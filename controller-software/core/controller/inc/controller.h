#include <iostream>
#include <signal.h>
#include <time.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <condition_variable>
#include "controller_api.h"
#include <atomic>
#include "fpga_interface.h"


class Controller {
    private:

        bool quit = false;  // flag to exit the controller

        static std::atomic<bool> pause_noncritical_thread; // flag to pause the noncritical thread
        static std::condition_variable noncritical_cv;
        static std::mutex noncritical_mtx;

        uint32_t noncritical_pause_timeout_us = 10; // timeout for noncritical thread to pause

        // FPGA driver
        Fpga_Interface fpga;

        // API driver
        static controller_api api;
        std::thread noncritical_thread;

        // node core
        Node_Core node_core;
        
        uint32_t critical_calls(){  // realtime calls that must be completed each cycle
            // realtime loop at a set frequency

            // run low level fpga drivers



            // update global node variables from FPGA mem
            
            // run node network
            node_core.run_update();
            
            // update FPGA mem from global node variables
            // signal to FPGA that update is done
            return 0;
        }

        static uint32_t noncritical_calls(Controller* controller){   // calls that can be delayed if needed
            // noncritical calls
            if(api.get_new_call() == 256){
                return 256;
            }

            if(api.run_calls() == 256){
                return 256;
            }

            return 0;
        }

    public:
        Controller(){
            /*
            initialize:
                node core
                API driver
                FPGA interface (OCM)

                load configs (main config, node networks, motion layout, electrical layout, FPGA bitstream)
                configure FPGA bitstream
                configure low level device drivers
                configure FPGA DMA
                configure devices
                configure node networks

                setup update flag timer (fall back if FPGA is not triggering updates)
                
            */

            fpga.initialize("/home/em-os/controller/config/fpga_configs/bit_files/bitfile.bit.bin");
            fpga.set_update_frequency(1000);

            api.set_pause_flag(&pause_noncritical_thread);
            noncritical_thread = std::thread(noncritical_calls, this);   // TODO: this should be a low priority thread

        }

        void run(){
            
            
            uint32_t ret = 0;
            while(!quit){

                // wait for update trigger
                if(fpga.wait_for_update() != 0){
                    std::cerr << "Error: FPGA update failed" << std::endl;
                    quit = true;
                    break;
                }

                // Signal noncritical thread to pause
                pause_noncritical_thread.store(true);

                // Wait for noncritical thread to pause
                auto wait_start = std::chrono::steady_clock::now();
                bool paused = false;
                while (std::chrono::steady_clock::now() - wait_start < std::chrono::microseconds(noncritical_pause_timeout_us)) {
                    if (!pause_noncritical_thread.load() || noncritical_thread.joinable()) {
                        paused = true;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }

                if (!paused) {
                    std::cerr << "Non-critical thread did not pause in time, running critical update anyway" << std::endl;
                }

                void critical_calls();

                update_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - last_trigger).count();
                remaining_update_time_us = software_update_period_us - update_duration_us;
                std::cout << "Update duration: " << update_duration_us << "us" << std::endl;

                // if api handler is done, join then restart (might make this run at a lower update rate than the base frequency)
                if(noncritical_thread.joinable()){  
                    noncritical_thread.join();
                    noncritical_thread = std::thread(noncritical_calls, this);
                }
                // Resume worker thread
                pause_noncritical_thread.store(false);
                noncritical_cv.notify_one();
              
            }

        }

        uint32_t set_software_update_period_us(uint32_t period_us){
            if(period_us > max_update_period_us || period_us < min_update_period_us){
                std::cerr << "Error: update period out of range" << std::endl;
                return 1;
            }
            software_update_period_us = period_us;
            return 0;
        }

        ~Controller(){
        }
};


controller_api Controller::api;

std::atomic<bool> Controller::pause_noncritical_thread;
std::condition_variable Controller::noncritical_cv;
std::mutex Controller::noncritical_mtx;
