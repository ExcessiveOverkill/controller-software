#include "controller.h"

Controller::Controller(){

    quit_nodejs();  // make sure nodejs is not running

    fpga_manager.set_fpga_interface(&fpga);
    fpga_manager.set_microseconds(&microseconds);
    fpga_manager.node_core = &node_core;

    api.setup(&node_core);

}

uint32_t Controller::load_config(std::string file_path){

    quit = true;

    // load the main config file that points to other configs

    std::ifstream f(file_path);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open " << file_path << std::endl;
        return 1;
    }

    try{
        json temp = json::parse(f);
        temp["fpga_config_path"].get_to(fpga_config_path);
        temp["fpga_driver_config_file"].get_to(fpga_driver_config_file);
        temp["fpga_instructions_file"].get_to(user_fpga_intructions_config_path);
        temp["node_config_file"].get_to(user_node_config_path);
        temp["temp_path"].get_to(temp_path);
        temp["software_update_frequency_hz"].get_to(software_update_frequency);
    }
    catch(json::parse_error& e){
        std::cerr << "Error: unable to parse main config file" << std::endl;
        return 2;
    }
    f.close();

    if(load_fpga_config()){
        std::cerr << "Failed to load FPGA config" << std::endl;
        return 2;
    }

    if(save_default_configs()){
        std::cerr << "Failed to save default configs" << std::endl;
        return 3;
    }

    if(load_user_configs()){
        std::cerr << "Failed to load user configs" << std::endl;
        return 4;
    }

    fpga_manager.set_update_frequency(software_update_frequency);

    if(fpga_manager.compile_instructions()){
        std::cerr << "Failed to compile fpga instructions" << std::endl;
        return 5;
    }

    quit = false;

    return 0;

}

uint32_t Controller::load_fpga_config(){
    
    if(fpga_manager.load_config(fpga_config_path)){
        std::cerr << "Failed to load FPGA config" << std::endl;
        return 1;
    }

    node_core.set_fpga_config(fpga_config_path);

    if(fpga_manager.initialize_fpga()){
        std::cerr << "Failed to initialize FPGA" << std::endl;
        return 2;
    }


    uint32_t ret = fpga_manager.load_drivers(fpga_driver_config_file);
    if(ret != 0 && ret != 1){
        std::cerr << " Failed to load FPGA drivers" << std::endl;
        return 3;
    }

    return 0;
}

uint32_t Controller::save_default_configs(){

    // this file will show all node objects created by the FPGA drivers
    // these objects will be created and loaded before any user node configs
    if(node_core.save(temp_path + "/default_node_config.json")){
        std::cerr << "Failed to save default node config" << std::endl;
        return 1;
    }
    
    // this file contains all instructions required by the FPGA drivers
    // they are NOT loaded unless there is no user config file, the user config needs to include all of them for full functionality
    if(fpga_manager.save_instructions(temp_path + "/default_fpga_instructions.json")){
        std::cerr << "Failed to save default fpga instructions" << std::endl;
        return 2;
    }

    return 0;
}

uint32_t Controller::load_user_configs(){
    
    // load user node configs
    if(node_core.load(user_node_config_path)){
        std::cerr << "Failed to load user node config" << std::endl;
        return 1;
    }

    // load user fpga configs
    if(fpga_manager.load_instructions(user_fpga_intructions_config_path)){
        std::cerr << "Failed to load user fpga instructions, attempting to load defaults" << std::endl;
        if(fpga_manager.load_instructions(temp_path + "/default_fpga_instructions.json")){
            std::cerr << "Failed to load default fpga instructions" << std::endl;
            return 2;
        }
    }

    return 0;
}

uint32_t Controller::start_nodejs(){
    auto ret = system("taskset -c 1 chrt -o 0 node controller/web/server/app.js > /dev/null 2>&1 &");
    if(ret != 0){
        std::cerr << "Error: failed to start nodejs webserver" << std::endl;
        return 1;
    }
    std::cout << "Nodejs webserver started" << std::endl;
    return 0;
}

void Controller::quit_nodejs(){
    system("pkill node");
}

void Controller::start(){

    if(quit){
        std::cerr << "Error: Controller could not start" << std::endl;
        return;
    }

    if(start_nodejs()){
        std::cerr << "Error: Failed to start nodejs" << std::endl;
        return;
    }

    // test node network
    /*
    node_core.create_network("test_network");
    json data = R"({
    "enable": false,
    "type": "sync",
    "timeout_usec": 10000,
    "update_cycle_trigger_count": 50   
    })"_json;
    node_core.configure_network("test_network", &data);

    node_core.add_node("test_network", "float_cmds", "api_cmd_float");
    node_core.add_node("test_network", "print_float", "float_print_cout");

    data = R"({
    "config": [
        {
            "name": "output_0",
            "min": -1.0,
            "max": 1.0,
            "default_value": 0.0,
            "timeout": 0.0
        }
    ]
    })"_json;
    
    node_core.configure_node("test_network", "float_cmds", &data);

    node_core.connect_nodes("test_network", "float_cmds", "output_0", "print_float", "input");

    node_core.rebuild_execution_order("test_network");

    data = R"({
    "enable": true
    })"_json;
    node_core.configure_network("test_network", &data);

    */

    node_core.set_enable(true);



    software_update_period_us = 1e6 / software_update_frequency;

    // Enter RT mode immediately before the control loop to avoid scheduling
    // non-critical startup work under FIFO policy.
    setup_main_thread();

    std::cout << "Controller started" << std::endl;

    run();  // main loop

    std::cout << "Controller stopped" << std::endl;

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
    param.sched_priority = 95; // leave headroom for IRQ threads

    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        std::cerr << "sched_setscheduler failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to set scheduler policy and priority.");
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset); // pin to CPU #0 — matches FPGA IRQ affinity

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "sched_setaffinity failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to set CPU affinity.");
    }

    // Verify the scheduling policy and priority.
    int policy;
    int ret = pthread_getschedparam(pthread_self(), &policy, &param);
    if (ret != 0) {
        std::cerr << "pthread_getschedparam failed: " << strerror(ret) << std::endl;
        throw std::runtime_error("Failed to get scheduling parameters.");
    }

    std::cout << "Main thread scheduling policy: "
              << (policy == SCHED_FIFO ? "SCHED_FIFO" :
                  policy == SCHED_RR   ? "SCHED_RR" :
                                        "SCHED_OTHER")
              << ", priority: " << param.sched_priority << std::endl;
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

void Controller::run(){
    uint64_t last_update_time = microseconds;

    while(!quit){

        // wait for update trigger
        uint32_t ret = fpga.wait_for_update();
        if(ret != 0){
            std::cerr << "Error: FPGA update failed" << std::endl;
            if(quit_on_fpga_update_fail){
                quit = true;
            }
            if(ret == 2 || ret == 3){
                // faults due to FPGA not running
                std::cerr << "FPGA update failure likely due to FPGA not running" << std::endl;
                quit = true;
            }
            continue;
        }
        //quit_on_fpga_update_fail = true;    // once we get a successful update, we can quit on failure

        critical_calls();


        // TODO: check how much time we have to run these first
        api.get_new_call();
        api.run_calls();

        update_microseconds();
        uint32_t update_duration = microseconds - last_update_time;
        last_update_time = microseconds;
        if(update_duration > software_update_period_us*2 + software_update_period_us / 10){   // if the update took longer tham +10%
            Fpga_Interface::IrqCountSnapshot irq_snapshot = fpga.get_irq_count_snapshot();
            std::cerr << "Update duration: " << update_duration << "us"
                      << "  run=" << irq_snapshot.run_count
                      << " done=" << irq_snapshot.done_count
                      << " dr=" << irq_snapshot.run_delta
                      << " dd=" << irq_snapshot.done_delta
                      << " delta=" << irq_snapshot.current_run_minus_done;
            if(irq_snapshot.baseline_valid) {
                std::cerr << " baseline=" << irq_snapshot.baseline_run_minus_done;
            }
            if(irq_snapshot.last_increment_anomaly) {
                std::cerr << "  irq_increment_anomaly_event=" << irq_snapshot.increment_anomaly_events;
            }
            std::cerr << std::endl;
        }
    }

}

void Controller::update_microseconds(){
    microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

Controller::~Controller(){
    quit = true;
    quit_nodejs();
}