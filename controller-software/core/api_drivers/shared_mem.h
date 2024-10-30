#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

#include <iostream>

#pragma once

class shared_mem
{
private:
        const unsigned int data_buffer_size = 256 * 4; // size of shared memory buffer for data (64 32-bit values)
        const unsigned int control_buffer_size = 32 * 4 * 4; // size of shared memory buffer for control info (able to hold 32 API calls)

        void* web_to_controller_data_mem = nullptr;
        void* web_to_controller_control_mem = nullptr;
        void* controller_to_web_data_mem = nullptr;
        void* controller_to_web_control_mem = nullptr;

        void* persistent_web_mem = nullptr;

        int web_to_controller_data_map;
        int web_to_controller_control_map;
        int controller_to_web_data_map;
        int controller_to_web_control_map;

        int persistent_web_map;

        unsigned int create_shared_mem(unsigned int size, void *&mem, int &shm_fd, std::string shm_name, int prot_flags = PROT_READ | PROT_WRITE);
        unsigned int open_shared_mem(unsigned int size, void *&mem, int &shm_fd, std::string shm_name, int prot_flags = PROT_READ | PROT_WRITE);

public:

    void* get_web_to_controller_data_mem() const { return web_to_controller_data_mem; }
    void* get_web_to_controller_control_mem() const { return web_to_controller_control_mem; }
    void* get_controller_to_web_data_mem() const { return controller_to_web_data_mem; }
    void* get_controller_to_web_control_mem() const { return controller_to_web_control_mem; }

    void* get_persistent_web_mem() const { return persistent_web_mem; }

    ~shared_mem();

    unsigned int controller_create_shared_mem();
    unsigned int web_create_shared_mem();

    unsigned int get_data_buffer_size() { return data_buffer_size; }
    unsigned int get_control_buffer_size() { return control_buffer_size; }

    void unmap_shared_mem();
    void close_shared_mem();
};

unsigned int shared_mem::open_shared_mem(unsigned int size, void *&mem, int &shm_fd, std::string shm_name, int prot_flags) {
    
    // Ensure name starts with '/'
    if (shm_name.empty() || shm_name[0] != '/') {
        shm_name = "/" + shm_name;
    }

    // Open the existing shared memory object
    shm_fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
    if (shm_fd == -1) {
        std::cerr << "Could not open shared memory object: " << strerror(errno) << std::endl;
        return 1;
    }

    // Map the shared memory object into the process's address space
    mem = mmap(NULL, size, prot_flags, MAP_SHARED, shm_fd, 0);
    if (mem == MAP_FAILED) {
        std::cerr << "Could not map shared memory object: " << strerror(errno) << std::endl;
        close(shm_fd);
        return 1;
    }

    // // set memory to all zero if we have write access
    // if(prot_flags & PROT_WRITE){
    //     memset(mem, 0, size);
    // }

    return 0;
}

unsigned int shared_mem::create_shared_mem(unsigned int size, void *&mem, int &shm_fd, std::string shm_name, int prot_flags) {

    // Ensure name starts with '/'
    if (shm_name.empty() || shm_name[0] != '/') {
        shm_name = "/" + shm_name;
    }

    // Open or create the shared memory object
    shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        std::cerr << "Could not open shared memory object: " << strerror(errno) << std::endl;
        return 1;
    }

    // Set the size of the shared memory object
    if (ftruncate(shm_fd, size) == -1) {
        std::cerr << "Could not set size of shared memory object: " << strerror(errno) << std::endl;
        close(shm_fd);
        shm_unlink(shm_name.c_str());
        return 1;
    }

    // Map the shared memory object into the process's address space
    mem = mmap(NULL, size, prot_flags, MAP_SHARED, shm_fd, 0);
    if (mem == MAP_FAILED) {
        std::cerr << "Could not map shared memory object: " << strerror(errno) << std::endl;
        close(shm_fd);
        shm_unlink(shm_name.c_str());
        return 1;
    }

    // set memory to all zero if we have write access
    if(prot_flags & PROT_WRITE){
        memset(mem, 0, size);
    }

    return 0;
}

unsigned int shared_mem::controller_create_shared_mem(){   // called from the controller side
    if(create_shared_mem(data_buffer_size, web_to_controller_data_mem, web_to_controller_data_map, "web_to_controller_data_mem", PROT_READ | PROT_WRITE) != 0){ // read and write so we can clear the buffer on startup
        return 1;
    }
    if(create_shared_mem(control_buffer_size, web_to_controller_control_mem, web_to_controller_control_map, "web_to_controller_control_mem", PROT_READ | PROT_WRITE) != 0){ // read and write so we can clear the buffer on startup
        return 1;
    }
    if(create_shared_mem(data_buffer_size, controller_to_web_data_mem, controller_to_web_data_map, "controller_to_web_data_mem", PROT_WRITE) != 0){
        return 1;
    }
    if(create_shared_mem(control_buffer_size, controller_to_web_control_mem, controller_to_web_control_map, "controller_to_web_control_mem", PROT_WRITE) != 0){
        return 1;
    }
    if(create_shared_mem(sizeof(uint32_t)*4, persistent_web_mem, persistent_web_map, "persistent_web_mem", PROT_READ | PROT_WRITE) != 0){   
        return 1;
    }
    return 0;
}

unsigned int shared_mem::web_create_shared_mem(){   // called from the web side
    if(open_shared_mem(data_buffer_size, web_to_controller_data_mem, web_to_controller_data_map, "web_to_controller_data_mem", PROT_WRITE) != 0){
        return 1;
    }
    if(open_shared_mem(control_buffer_size, web_to_controller_control_mem, web_to_controller_control_map, "web_to_controller_control_mem", PROT_WRITE) != 0){
        return 1;
    }
    if(open_shared_mem(data_buffer_size, controller_to_web_data_mem, controller_to_web_data_map, "controller_to_web_data_mem", PROT_READ) != 0){
        return 1;
    }
    if(open_shared_mem(control_buffer_size, controller_to_web_control_mem, controller_to_web_control_map, "controller_to_web_control_mem", PROT_READ) != 0){
        return 1;
    }
    if(open_shared_mem(sizeof(uint32_t)*4, persistent_web_mem, persistent_web_map, "persistent_web_mem", PROT_READ | PROT_WRITE) != 0){   
        return 1;
    }
    return 0;
}

void shared_mem::unmap_shared_mem(){
    munmap(web_to_controller_data_mem, data_buffer_size);
    munmap(controller_to_web_data_mem, data_buffer_size);
    munmap(web_to_controller_control_mem, control_buffer_size);
    munmap(controller_to_web_control_mem, control_buffer_size);
    munmap(persistent_web_mem, sizeof(uint32_t)*4);
}

void shared_mem::close_shared_mem(){
    close(web_to_controller_data_map);
    close(controller_to_web_data_map);
    close(web_to_controller_control_map);
    close(controller_to_web_control_map);
    close(persistent_web_map);
}

shared_mem::~shared_mem(){
}
