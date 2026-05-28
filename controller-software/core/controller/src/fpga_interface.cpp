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
#include <dirent.h>
#include <fstream>


namespace {

bool enable_uio_irq(int fd) {
    uint32_t val = 1;
    return write(fd, &val, sizeof(val)) == static_cast<ssize_t>(sizeof(val));
}

bool read_uio_count(int fd, uint32_t& count) {
    return read(fd, &count, sizeof(count)) == static_cast<ssize_t>(sizeof(count));
}

bool drain_uio_pending(int fd, int max_reads = 32) {
    bool drained_any = false;
    for (int i = 0; i < max_reads; i++) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int ret = poll(&pfd, 1, 0);
        if (ret <= 0) {
            break;
        }
        if ((pfd.revents & POLLIN) == 0) {
            break;
        }

        uint32_t count = 0;
        if (!read_uio_count(fd, count)) {
            return false;
        }
        if (!enable_uio_irq(fd)) {
            return false;
        }
        drained_any = true;
    }

    // Leave IRQ unmasked for next event.
    return enable_uio_irq(fd) && (drained_any || true);
}

int find_irq_number_by_name(const std::string& irq_name) {
    DIR* dir = opendir("/proc/irq");
    if (dir == nullptr) {
        return -1;
    }

    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string dname(entry->d_name);
        if (dname.empty() || dname[0] < '0' || dname[0] > '9') {
            continue;
        }

        std::string probe = "/proc/irq/" + dname + "/" + irq_name;
        if (access(probe.c_str(), F_OK) == 0) {
            closedir(dir);
            return std::stoi(dname);
        }
    }

    closedir(dir);
    return -1;
}

void pin_irq_to_cpu0_best_effort(const std::string& irq_name) {
    int irq_num = find_irq_number_by_name(irq_name);
    if (irq_num < 0) {
        std::cerr << "Warning: unable to find IRQ number for " << irq_name << std::endl;
        return;
    }

    std::string affinity_path = "/proc/irq/" + std::to_string(irq_num) + "/smp_affinity_list";
    std::ofstream affinity_file(affinity_path);
    if (!affinity_file.is_open()) {
        std::cerr << "Warning: unable to open " << affinity_path << " to set IRQ affinity" << std::endl;
        return;
    }

    affinity_file << "0";
    if (!affinity_file.good()) {
        std::cerr << "Warning: failed writing IRQ affinity for " << irq_name << std::endl;
    }
}

}


//size_t page_size = sysconf(_SC_PAGESIZE);

Fpga_Interface::Fpga_Interface(){
}

Fpga_Interface::IrqCountSnapshot Fpga_Interface::get_irq_count_snapshot() const {
    IrqCountSnapshot snapshot;
    snapshot.run_count = last_running_irq_count;
    snapshot.done_count = last_done_irq_count;
    snapshot.run_delta = last_cycle_run_delta;
    snapshot.done_delta = last_cycle_done_delta;
    snapshot.baseline_valid = irq_delta_baseline_valid;
    snapshot.baseline_run_minus_done = irq_delta_baseline;
    snapshot.current_run_minus_done = static_cast<int32_t>(last_running_irq_count) - static_cast<int32_t>(last_done_irq_count);
    snapshot.delta_change_events = irq_delta_change_events;
    snapshot.increment_anomaly_events = irq_increment_anomaly_events;
    snapshot.last_increment_anomaly = last_irq_increment_anomaly;
    return snapshot;
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

    // Keep FPGA update IRQs local to CPU0 to reduce migration latency/jitter.
    pin_irq_to_cpu0_best_effort("fpga_mem_update_running_irq");
    pin_irq_to_cpu0_best_effort("fpga_mem_update_done_irq");

    error_if_nullptr();

    // TODO: get these values and offsets from a config file?
    fpga_main_trigger_counter = (uint16_t*)((char*)ocm_base_pointer + mem_layout.PS_to_PL_control_base_addr_offset);
    fpga_watchdog = (uint16_t*)((char*)ocm_base_pointer + mem_layout.PS_to_PL_control_base_addr_offset + 4);



    first_cycle = true;
    last_running_irq_count = 0;
    last_done_irq_count = 0;
    irq_delta_baseline_valid = false;
    irq_delta_last_observed_valid = false;
    irq_delta_change_events = 0;
    irq_prev_counts_valid = false;
    irq_increment_anomaly_events = 0;
    last_irq_increment_anomaly = false;
    last_cycle_run_delta = 0;
    last_cycle_done_delta = 0;

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
    enable_uio_irq(mem_update_running_fds[0].fd);
    enable_uio_irq(mem_update_done_fds[0].fd);

    // load bitstream
    int ret = system(("sudo fpgautil -b " + bitstreamPath).c_str());

    if(ret != 0) {
        std::cerr << "Failed to load bitstream" << std::endl;
        return 3;   // bitstream loading failed
    }

    // Drain stale startup events so the first control cycles start clean.
    if(!drain_uio_pending(mem_update_running_fds[0].fd) || !drain_uio_pending(mem_update_done_fds[0].fd)) {
        std::cerr << "Warning: failed to fully drain startup IRQ state" << std::endl;
    }

    return 0;
}

void Fpga_Interface::feed_watchdog() {
    // just need to set the watchdog register to a different value
    (*fpga_watchdog)++;
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
    uint32_t count;

    bool have_running = false;

    // Drain immediately pending running events. These can represent late-serviced
    // edges from the previous cycle and should not be treated as hard failure.
    ret = poll(mem_update_running_fds, 1, 0);
    while(ret > 0) {
        if(mem_update_running_fds->revents & POLLIN) {
            if(!read_uio_count(mem_update_running_fds[0].fd, count)) {
                return 12; // failed to read running IRQ count
            }
            last_running_irq_count = count;
            if(!enable_uio_irq(mem_update_running_fds[0].fd)) {
                return 13; // failed to re-enable running IRQ
            }
            have_running = true;
            ret = poll(mem_update_running_fds, 1, 0);
        }
        else{
            return 10;  // unknown event
        }
    }

    if(!have_running) {
        ret = poll(mem_update_running_fds, 1, 5); // wait for update start
        if(ret > 0) {
            if(mem_update_running_fds->revents & POLLIN) {
                if(!read_uio_count(mem_update_running_fds[0].fd, count)) {
                    return 12; // failed to read running IRQ count
                }
                last_running_irq_count = count;
                if(!enable_uio_irq(mem_update_running_fds[0].fd)) {
                    return 13; // failed to re-enable running IRQ
                }
                have_running = true;
            }
            else{
                return 10;  // unknown event
            }
        }
        else{
            std::cout << "Timeout waiting for FPGA update start" << std::endl;
            return 2;   // timeout
        }
    }

    // First cycle can observe ambiguous startup ordering; accept and move on.
    if(first_cycle) {
        first_cycle = false;
        return 0;
    }

    // Arm done IRQ, then wait for completion. During this window, tolerate extra
    // running events (e.g. trailing transitions) by consuming and continuing.
    if(!enable_uio_irq(mem_update_done_fds[0].fd)) {
        return 14; // failed to re-enable done IRQ
    }

    auto done_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    while(std::chrono::steady_clock::now() < done_deadline) {
        ret = poll(mem_update_done_fds, 1, 1);
        if(ret > 0) {
            if(mem_update_done_fds->revents & POLLIN) {
                if(!read_uio_count(mem_update_done_fds[0].fd, count)) {
                    return 15; // failed to read done IRQ count
                }
                last_done_irq_count = count;

                uint32_t run_delta = 0;
                uint32_t done_delta = 0;
                if(irq_prev_counts_valid) {
                    run_delta = last_running_irq_count - prev_running_irq_count;
                    done_delta = last_done_irq_count - prev_done_irq_count;
                    if(run_delta != 1 || done_delta != 1) {
                        irq_increment_anomaly_events++;
                        last_irq_increment_anomaly = true;
                        std::cerr << "IRQ increment anomaly: dr=" << run_delta
                                  << " dd=" << done_delta
                                  << " run=" << last_running_irq_count
                                  << " done=" << last_done_irq_count
                                  << " event=" << irq_increment_anomaly_events;
                        if(run_delta == done_delta && run_delta > 1) {
                            std::cerr << " (possible whole-cycle skip)";
                        }
                        std::cerr << std::endl;
                    }
                    else {
                        last_irq_increment_anomaly = false;
                    }
                }

                // Store deltas for snapshot before updating prev counts
                last_cycle_run_delta = run_delta;
                last_cycle_done_delta = done_delta;

                prev_running_irq_count = last_running_irq_count;
                prev_done_irq_count = last_done_irq_count;
                irq_prev_counts_valid = true;

                int32_t irq_delta = static_cast<int32_t>(last_running_irq_count) - static_cast<int32_t>(last_done_irq_count);
                if(!irq_delta_baseline_valid) {
                    irq_delta_baseline = irq_delta;
                    irq_delta_baseline_valid = true;
                    irq_delta_last_observed = irq_delta;
                    irq_delta_last_observed_valid = true;
                    std::cout << "IRQ count delta baseline set: run-done=" << irq_delta_baseline
                              << " (run=" << last_running_irq_count
                              << ", done=" << last_done_irq_count << ")" << std::endl;
                }
                else if(!irq_delta_last_observed_valid || irq_delta != irq_delta_last_observed) {
                    irq_delta_change_events++;
                    std::cerr << "IRQ count delta changed: run-done=" << irq_delta
                              << " (baseline=" << irq_delta_baseline
                              << ", run=" << last_running_irq_count
                              << ", done=" << last_done_irq_count
                              << ", event=" << irq_delta_change_events << ")" << std::endl;
                    irq_delta_last_observed = irq_delta;
                    irq_delta_last_observed_valid = true;
                }

                if(!enable_uio_irq(mem_update_running_fds[0].fd)) {
                    return 13; // failed to re-enable running IRQ
                }
                feed_watchdog();
                return 0;
            }
            return 10; // unknown event
        }

        // Consume any trailing running event and continue waiting for done.
        int run_ret = poll(mem_update_running_fds, 1, 0);
        while(run_ret > 0) {
            if(mem_update_running_fds->revents & POLLIN) {
                if(!read_uio_count(mem_update_running_fds[0].fd, count)) {
                    return 12;
                }
                last_running_irq_count = count;
                if(!enable_uio_irq(mem_update_running_fds[0].fd)) {
                    return 13;
                }
                run_ret = poll(mem_update_running_fds, 1, 0);
            }
            else{
                return 10;
            }
        }
    }

    std::cout << "Timeout waiting for FPGA update finish" << std::endl;
    return 3;   // timeout

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