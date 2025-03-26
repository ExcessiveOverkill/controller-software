#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

#include <iostream>

class shared_mem{
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

