#include "fpga_interface.h"
#include <memory.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdexcept>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>


//size_t page_size = sysconf(_SC_PAGESIZE);

Fpga_Interface::Fpga_Interface(){
}


Fpga_Interface::~Fpga_Interface(){
    munmap(ocm_base_pointer, mem_layout.OCM_SIZE);
}

uint32_t Fpga_Interface::initialize(const fpga_mem_layout mem_layout, std::string bitstreamPath) {

    this->mem_layout = mem_layout;
   
    // unmap any existing memory
    if(ocm_base_pointer != nullptr){
        munmap(ocm_base_pointer, mem_layout.OCM_SIZE);
        ocm_base_pointer = nullptr;
    }


    // setup memory

    auto uioFd = open("/dev/uio0", O_RDWR);
    if(uioFd < 0) {
        throw std::runtime_error("Failed to open /dev/uio0");
    }

    ocm_base_pointer = mmap(nullptr, mem_layout.OCM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, uioFd, 0);

    if (ocm_base_pointer == MAP_FAILED) {
        throw std::runtime_error("Failed to mmap /dev/uio0");
    }

    // setup interrupts

    auto uioFd1 = open("/dev/uio1", O_RDWR);
    if(uioFd1 < 0) {
        throw std::runtime_error("Failed to open /dev/uio1");
    }

    auto uioFd2 = open("/dev/uio2", O_RDWR);
    if(uioFd2 < 0) {
        throw std::runtime_error("Failed to open /dev/uio2");
    }

    mem_update_running_fds[0].fd = uioFd1;
    mem_update_running_fds[0].events = POLLIN;

    mem_update_done_fds[0].fd = uioFd2;
    mem_update_done_fds[0].events = POLLIN;

    error_if_nullptr();

    fpga_main_trigger_counter = (uint16_t*)((char*)ocm_base_pointer + mem_layout.PS_to_PL_control_base_addr_offset);



    first_cycle = true;

    // make sure the bitstream file exists (some minor input validation)
    struct stat buffer;
    if (stat(bitstreamPath.c_str(), &buffer) != 0) {
        std::cerr << "Bitstream file does not exist" << std::endl;
        return 2;   // file does not exist
    }

    *fpga_main_trigger_counter = 0; // setting to zero will hault the FPGA
    cache_flush_all();
    // sleep for a bit to make sure the FPGA has time to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    
    // clear PS controlled memory
    memset((char*)ocm_base_pointer + mem_layout.PS_to_PL_control_base_addr_offset, 0, mem_layout.PS_to_PL_control_size);
    memset((char*)ocm_base_pointer + mem_layout.PS_to_PL_data_base_addr_offset, 0, mem_layout.PS_to_PL_data_size);
    memset((char*)ocm_base_pointer + mem_layout.PS_to_PL_dma_instructions_base_addr_offset, 0, mem_layout.PS_to_PL_dma_instructions_size);
    
    //memcpy((char*)ocm_base_pointer + PS_to_PL_data_base_addr_offset, new uint32_t(0x1234567), 4);
    //memcpy((char*)ocm_base_pointer + PS_to_PL_dma_instructions_base_addr_offset, new uint64_t(0x0002000004000000), 8);
    
    *fpga_main_trigger_counter = 0xFFFF; // set back to maximum for lowest update frequency

    cache_flush_all();

    // clear PL controlled memory
    memset((char*)ocm_base_pointer + mem_layout.PL_to_PS_data_base_addr_offset, 0, mem_layout.PL_to_PS_data_size);
    memset((char*)ocm_base_pointer + mem_layout.PL_to_PS_control_base_addr_offset, 0, mem_layout.PL_to_PS_control_size);
    cache_invalidate_all();

    // clear any pending interrupts
    uint32_t val = 1;
    write(mem_update_running_fds[0].fd, &val, sizeof(val));
    write(mem_update_done_fds[0].fd, &val, sizeof(val));

    // load bitstream
    int ret = system(("sudo fpgautil -b " + bitstreamPath).c_str());

    if(ret != 0) {
        std::cerr << "Failed to load bitstream" << std::endl;
        return 3;   // bitstream loading failed
    }

    return 0;
}

uint32_t Fpga_Interface::set_update_frequency(uint32_t frequency) {
    // set the frequency at which the FPGA will update
    // frequency is in Hz

    error_if_nullptr();

    if(frequency < 25e6 / 0xffff) {
        return 1;   // frequency too low
    }
    if(frequency > 20e3) {  // capped at 20 kHz
        return 2;   // frequency too high
    }

    *fpga_main_trigger_counter = 25e6 / frequency;

    return 0;
}

uint32_t Fpga_Interface::wait_for_update() {
    // wait for mem update to start
    int ret = 0;
    uint32_t val = 1;
    uint32_t count;

    ret = poll(mem_update_running_fds, 1, 0); // check if the update has already started, this is bad and means we missed the interrupt
    if(ret > 0) {
        if(mem_update_running_fds->revents & POLLIN) {
            read(mem_update_running_fds[0].fd, &count, sizeof(count)); // clear the interrupt
            write(mem_update_running_fds[0].fd, &val, sizeof(val)); // clear the interrupt
            if(first_cycle){
                first_cycle = false;
                return 0;   // first cycle, ok to miss the interrupt
            }
            std::cout << "FPGA interrupt missed" << std::endl;
            return 1;   // update already started
        }
        else{
            return 10;  // unknown event
        }
    }


    ret = poll(mem_update_running_fds, 1, 5); // Wait up to 5ms for the update to start
    if(ret > 0) {   // update detected
        if(mem_update_running_fds->revents & POLLIN) {
            read(mem_update_running_fds[0].fd, &count, sizeof(count)); // clear the interrupt
            write(mem_update_done_fds[0].fd, &val, sizeof(val)); // clear the done interrupt
            ret = poll(mem_update_done_fds, 1, 1); // wait up to 1ms for the update to finish
            read(mem_update_done_fds[0].fd, &count, sizeof(count)); // clear the interrupt
            write(mem_update_running_fds[0].fd, &val, sizeof(val)); // clear the start interrupt

            if(ret > 0) {   // update detected
                if(mem_update_running_fds->revents & POLLIN) {
                    first_cycle = false;
                    return 0;   // update finished
                }
                else{
                    return 10;  // unknown event
                }
            }
            else{   // update never finished
                std::cout << "Timeout waiting for FPGA update finish" << std::endl;
                return 3;   // timeout
            }
        }
        else{
            return 10;  // unknown event
        }
    }
    else{   // no update detected within 10ms, return error
        std::cout << "Timeout waiting for FPGA update start" << std::endl;
        return 2;   // timeout
    }

    return 11;  // how did we get here?

}

void* Fpga_Interface::get_PS_to_PL_control_pointer(uint32_t offset) {
    error_if_nullptr();
    return (char*)ocm_base_pointer + mem_layout.PS_to_PL_control_base_addr_offset + offset;
}

void* Fpga_Interface::get_PL_to_PS_control_pointer(uint32_t offset) {
    error_if_nullptr();
    return (char*)ocm_base_pointer + mem_layout.PL_to_PS_control_base_addr_offset + offset;
}

void* Fpga_Interface::get_PS_to_PL_data_pointer(uint32_t offset) {
    error_if_nullptr();
    return (char*)ocm_base_pointer + mem_layout.PS_to_PL_data_base_addr_offset + offset;
}

void* Fpga_Interface::get_PL_to_PS_data_pointer(uint32_t offset) {
    error_if_nullptr();
    return (char*)ocm_base_pointer + mem_layout.PL_to_PS_data_base_addr_offset + offset;
}

void* Fpga_Interface::get_PS_to_PL_dma_instructions_pointer(uint32_t offset) {
    error_if_nullptr();
    return (char*)ocm_base_pointer + mem_layout.PS_to_PL_dma_instructions_base_addr_offset + offset;
}

void Fpga_Interface::cache_flush(void* addr, uint32_t size) {
    __builtin___clear_cache((char*)addr, (char*)addr + size);
}

void Fpga_Interface::cache_invalidate(void* addr, uint32_t size) {
    __builtin___clear_cache((char*)addr, (char*)addr + size);
}

void Fpga_Interface::cache_flush_all() {
    cache_flush((char*)ocm_base_pointer + mem_layout.PS_to_PL_control_base_addr_offset, mem_layout.PS_to_PL_control_size);
    cache_flush((char*)ocm_base_pointer + mem_layout.PS_to_PL_data_base_addr_offset, mem_layout.PS_to_PL_data_size);
    cache_flush((char*)ocm_base_pointer + mem_layout.PS_to_PL_dma_instructions_base_addr_offset, mem_layout.PS_to_PL_dma_instructions_size);
}

void Fpga_Interface::cache_invalidate_all() {
    cache_invalidate((char*)ocm_base_pointer + mem_layout.PL_to_PS_data_base_addr_offset, mem_layout.PL_to_PS_data_size);
    cache_invalidate((char*)ocm_base_pointer + mem_layout.PL_to_PS_control_base_addr_offset, mem_layout.PL_to_PS_control_size);
}

void Fpga_Interface::error_if_nullptr(){
    if(ocm_base_pointer == nullptr){
        throw std::runtime_error("FPGA memory not initialized");
    }
}