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


class Controller {
    private:
        uint64_t cycle_count = 0;   // count of FPGA cycles since startup

        bool quit = false;  // flag to exit the controller
        bool quit_on_fpga_update_fail = false;    // flag to exit the controller if the FPGA update fails

        static std::atomic<bool> pause_noncritical_thread; // flag to pause the noncritical thread
        static std::condition_variable noncritical_cv;
        static std::mutex noncritical_mtx;

        uint32_t noncritical_pause_timeout_us = 10; // timeout for noncritical thread to pause
        uint32_t software_update_period_us = 0;

        // FPGA drivers
        Fpga_Interface fpga;
        fpga_module_manager fpga_manager;

        // API driver
        static controller_api api;
        std::thread noncritical_thread;

        // Node core
        Node_Core node_core;
        
        uint32_t critical_calls();  // hard-realtime calls that must be completed each cycle

        static uint32_t noncritical_calls(Controller* controller);   // calls that can be delayed if needed

        void setup_main_thread();

        uint64_t microseconds = 0;
        void update_microseconds();


    public:
        Controller();

        void load_fpga_config(std::string config_file);
        void load_ps_nodes(std::string file_path);

        void run();

        ~Controller(){
        }
};