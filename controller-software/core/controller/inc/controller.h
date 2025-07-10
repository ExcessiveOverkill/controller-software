#include <iostream>
#include <signal.h>
#include <time.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include "controller_api.h"
#include <atomic>
#include "fpga_interface.h"
#include "fpga_module_manager.h"

#pragma once


class Controller {
    private:
        uint64_t cycle_count = 0;   // count of FPGA cycles since startup

        bool quit = false;  // flag to exit the controller
        bool quit_on_fpga_update_fail = false;    // flag to exit the controller if the FPGA update fails

        uint32_t software_update_period_us = 1000;  // 1khz default
        uint32_t software_update_frequency = 1000;  // 1khz default

        // FPGA drivers
        Fpga_Interface fpga;
        fpga_module_manager fpga_manager;

        // API driver
        controller_api api;
        std::thread noncritical_thread;

        // Node core
        Node_Core node_core;
        
        uint32_t critical_calls();  // hard-realtime calls that must be completed each cycle

        void setup_main_thread();

        uint64_t microseconds = 0;
        void update_microseconds();

        void run();

        uint32_t load_fpga_config();
        std::string fpga_config_path = "";

        std::string fpga_driver_config_file = "";

        uint32_t save_default_configs();
        std::string temp_path = "";

        uint32_t load_user_configs();
        std::string user_node_config_path = "";
        std::string user_fpga_intructions_config_path = "";

        uint32_t start_nodejs();
        void quit_nodejs();


    public:
        Controller();

        uint32_t load_config(std::string file_path);  // load the main config file

        void start();

        ~Controller();
};