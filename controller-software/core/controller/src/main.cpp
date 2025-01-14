//#include "api_drivers/controller_api.h"
//#include "nodes/node_core.h"
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
//#include "controller.h"
#include "fpga_interface.h"
#include "fpga_module_manager.h"


int main() {

    // TODO: clean up this thread control stuff
    // Lock the current and future mapped memory to avoid page faults.
    // This ensures we do not get delayed by paging in real-time sections.
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "mlockall failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // Set the scheduler policy and priority.
    struct sched_param param;
    param.sched_priority = 90; // pick something appropriate, 1-99

    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        std::cerr << "sched_setscheduler failed: " << strerror(errno) << std::endl;
        return 1;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset); // pin to CPU #1

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "sched_setaffinity failed: " << strerror(errno) << std::endl;
    }


    //Node_Core core;
    //Controller controller;

    fpga_module_manager fpga_manager;
    Fpga_Interface fpga;
    fpga_manager.set_fpga_interface(&fpga);

    fpga_manager.load_config("/home/em-os/controller/config/fpga_configs/controller_config.json");


    if(fpga_manager.initialize_fpga()){
        std::cerr << "Failed to initialize FPGA" << std::endl;
        return 1;
    }

    fpga_manager.load_drivers();

    // load node config

    // drivers should somehow specify what variables are available to the controller (both to the PS and PL), and make configuration settings available to the user

    // use node config to create global variables from fpga module drivers

    // for now the global variables created are just hardcoded in the drivers
    fpga_manager.create_global_variables();


    // TESTING: get encoder pos and drive cmd current variables from driver
    int32_t* cmd_current_milliamps = nullptr;
    uint32_t* encoder_pos = nullptr;
    uint32_t* encoder_pos_multiturn = nullptr;

    auto servo_drive_driver = fpga_manager.get_driver(3);
    auto encoder_driver = fpga_manager.get_driver(1);

    if(servo_drive_driver != nullptr){
        cmd_current_milliamps = servo_drive_driver->cmd_q_current_milliamps;
    }
    if(encoder_driver != nullptr){
        encoder_pos = encoder_driver->encoder_pos;
        encoder_pos_multiturn = encoder_driver->encoder_multiturn_count;
    }

    fpga.set_update_frequency(1000);

    uint32_t count = 0;
    uint32_t ret = 0;

    const double max_current = 4.0;
    const double max_velocity = 2.0;

    double home_pos = 0.0;

    double cmd_pos = 0.5;
    double cmd_vel = 0.0;
    double fbk_pos = 0.0;
    double fbk_vel = 0.0;

    double cmd_current = 0.0;

    double pos_p = 20.0;
    double pos_i = 0.0;
    double pos_i_limit = 0.0;

    double vel_p = 10.0;
    double vel_i = 0.0;
    double vel_i_limit = 0.0;

    double pos_integrator = 0.0;
    double vel_integrator = 0.0;

    cmd_pos = home_pos;


    while(1){
        ret = fpga.wait_for_update();
        if(ret != 0){
            std::cout << "FPGA update failed" << std::endl;
            //return 1;
        }
        fpga.cache_invalidate_all();    // make sure any cached memory gets updated

        fpga_manager.run_update();  // updates low level drivers


        if(home_pos == 0.0){
            home_pos = static_cast<double>(*encoder_pos) / 0xffffffff;
            home_pos += static_cast<double>(*encoder_pos_multiturn);     // TODO: make this handle multiturn rollover
            cmd_pos = home_pos;
        }
        else{

            if(count == 0){
                cmd_pos += .5;
            }

            // Read feedback position and velocity
            double old_fbk_pos = fbk_pos;
            fbk_pos = static_cast<double>(*encoder_pos) / 0xffffffff;
            fbk_pos += static_cast<double>(*encoder_pos_multiturn);     // TODO: make this handle multiturn rollover


            fbk_vel = (fbk_pos - old_fbk_pos) * 1000.0; // 1 ms update rate

            // Position control
            double pos_error = cmd_pos - fbk_pos;
            pos_integrator += pos_error;
            if (pos_integrator > pos_i_limit) pos_integrator = pos_i_limit;
            if (pos_integrator < -pos_i_limit) pos_integrator = -pos_i_limit;
            cmd_vel = pos_p * pos_error + pos_i * pos_integrator;


            // limit velocity
            cmd_vel = std::min(std::max(cmd_vel, -max_velocity), max_velocity);


            // Velocity control
            double vel_error = cmd_vel - fbk_vel;
            vel_integrator += vel_error;
            if (vel_integrator > vel_i_limit) vel_integrator = vel_i_limit;
            if (vel_integrator < -vel_i_limit) vel_integrator = -vel_i_limit;
            cmd_current = vel_p * vel_error + vel_i * vel_integrator;


            // Limit current
            cmd_current = std::min(std::max(cmd_current, -max_current), max_current);

            // Write command current to the servo drive
            *cmd_current_milliamps = static_cast<int32_t>(cmd_current * 1000.0);
        }


        // TODO: other realtime updates happen here



        count++;
        if(count >= 1000){
            std::cout << "FPGA update 1000x passed" << std::endl;
            count = 0;
        }

        fpga.cache_flush_all(); // write any changed data to FPGA memory
    }

    //controller.run();

    // startup internal API
    // controller_api api;

    // node_network* net = new node_network();

    // net->set_type(node_network::update_type::ASYNC);
    // net->set_timeout_usec(100);
    // //net->set_timeout_sec(2);

    // unsigned int cycle = 0;

    // net->add_node(new bool_constant(), "constant");
    // net->add_node(new logic_not(), "not_0");
    // net->add_node(new logic_not(), "not_1");
    // net->add_node(new logic_not(), "not_2");
    // net->add_node(new logic_not(), "not_3");
    // net->add_node(new logic_and(), "and_0");
    // net->add_node(new bool_print_cout(), "bool_print");
    // net->add_node(new nothing_delay(), "delay_0");

    // output* out = nullptr;

    // net->connect_nodes("constant", "output", "delay_0", "input");
    // net->connect_nodes("delay_0", "output", "not_0", "input");
    // net->connect_nodes("not_0", "output", "not_1", "input");
    // net->connect_nodes("not_1", "output", "not_2", "input");
    // net->connect_nodes("not_2", "output", "bool_print", "input");

    // //net->nodes["not_2"]->get_output_object("output", out);

    // //net->nodes[1]->connect_input("input", net->nodes[4], "output"); // connect first not to fourth not (creates circular dependency)

    // //std::cout << "output: " << *reinterpret_cast<bool*>(out->data_pointer) << std::endl;

    // net->rebuild_execution_order();    
    // net->run(&cycle);
    // cycle++;

    // std::this_thread::sleep_for(std::chrono::seconds(2));

    // net->run(&cycle);
    // cycle++;
    
    // std::this_thread::sleep_for(std::chrono::seconds(2));

    // // net->connect_nodes("not_2", "output", "and_0", "input_A");
    // // net->connect_nodes("not_0", "output", "and_0", "input_B");
    // // net->connect_nodes("and_0", "output", "bool_print", "input");

    // // net->rebuild_execution_order();
    // net->run(&cycle);


    return 0;
}