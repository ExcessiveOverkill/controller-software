#include "controller.h"

Controller::Controller(){
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

    fpga_manager.set_fpga_interface(&fpga);
    fpga_manager.set_microseconds(&microseconds);
    fpga_manager.node_core = &node_core;

    api.set_pause_flag(&pause_noncritical_thread);
    noncritical_thread = std::thread(noncritical_calls, this);   // TODO: this should be a low priority thread

}

void Controller::load_fpga_config(std::string config_file){
    
    if(fpga_manager.load_config(config_file)){
        std::cerr << "Failed to load FPGA config" << std::endl;
        quit = true;
    }

    if(fpga_manager.initialize_fpga()){
        std::cerr << "Failed to initialize FPGA" << std::endl;
        quit = true;
    }

    fpga_manager.load_drivers();

    fpga_manager.save_ps_nodes("/home/em-os/controller/nodes/fpga_nodes.json");

    // TODO: load user fpga copy instructions

    fpga_manager.compile_instructions();

    fpga.set_update_frequency(1000);    // TODO: get from a config file
    
}

void Controller::load_ps_nodes(std::string file_path){
    // TODO: implement
}

void Controller::setup_main_thread(){
    // Lock the current and future mapped memory to avoid page faults.
    // This ensures we do not get delayed by paging in real-time sections.
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "mlockall failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to lock memory.");
    }

    // Set the scheduler policy and priority.
    struct sched_param param;
    param.sched_priority = 90; // 1-99

    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        std::cerr << "sched_setscheduler failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to set scheduler policy and priority.");
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset); // pin to CPU #1

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "sched_setaffinity failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to set CPU affinity.");
    }
}

uint32_t Controller::critical_calls(){  // realtime calls that must be completed each cycle
    // realtime loop at a set frequency

    cycle_count++;

    // run low level fpga drivers
    fpga.cache_invalidate_all();    // make sure any cached memory gets updated
    fpga_manager.run_update();  // updates low level drivers
    
    // run node network
    node_core.run_update();
    
    fpga.cache_flush_all(); // write any changed data to FPGA memory

    return 0;
}

uint32_t Controller::noncritical_calls(Controller* controller){   // calls that can be delayed if needed
    // noncritical calls
    if(api.get_new_call() == 256){
        return 256;
    }

    if(api.run_calls() == 256){
        return 256;
    }

    return 0;
}

void Controller::run(){
    uint32_t ret = 0;

    uint64_t last_update_time = microseconds;

    while(!quit){

        // wait for update trigger
        if(fpga.wait_for_update() != 0){
            std::cerr << "Error: FPGA update failed" << std::endl;
            if(quit_on_fpga_update_fail){
                quit = true;
            }
            continue;
        }
        quit_on_fpga_update_fail = true;    // once we get a successful update, we can quit on failure
        

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
        
        update_microseconds();
        std::cout << "Update duration: " << (microseconds-last_update_time) << "us" << std::endl;

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

void Controller::update_microseconds(){
    microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}


controller_api Controller::api;

std::atomic<bool> Controller::pause_noncritical_thread;
std::condition_variable Controller::noncritical_cv;
std::mutex Controller::noncritical_mtx;