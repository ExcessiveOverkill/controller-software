#include "node_network.h"
#include <map>
#include "global_variables/base_global_variable.h"

class Node_Core {
    private:
        // set some limits to keep things from getting out of hand
        const uint32_t max_networks = 20;   // maximum number of networks allowed
        const uint32_t max_nodes_per_network = 20;  // maximum number of nodes allowed per network
        // TODO: add limits for global variable counts

        std::map<std::string, std::shared_ptr<node_network>> networks;
        std::vector<std::shared_ptr<node_network>> network_execution_order;

        std::map<std::string, std::shared_ptr<global_variable>> global_variables;

        uint32_t cycle = 0;

        bool enable = false;    // enable running the networks
        bool single_cycle_step = false;   // run one cycle at a time
        //bool single_network_step = false; // run one network at a time (not yet implemented)

    public:
        Node_Core(){
            // net.set_type(node_network::update_type::ASYNC);
            // net.set_timeout_usec(100);
            // //net->set_timeout_sec(2);

            // net.add_node(new bool_constant(), "constant");
            // net.add_node(new logic_not(), "not_0");
            // net.add_node(new logic_not(), "not_1");
            // net.add_node(new logic_not(), "not_2");
            // net.add_node(new logic_not(), "not_3");
            // net.add_node(new logic_and(), "and_0");
            // net.add_node(new bool_print_cout(), "bool_print");
            // net.add_node(new nothing_delay(), "delay_0");

            // output* out = nullptr;

            // net.connect_nodes("constant", "output", "delay_0", "input");
            // net.connect_nodes("delay_0", "output", "not_0", "input");
            // net.connect_nodes("not_0", "output", "not_1", "input");
            // net.connect_nodes("not_1", "output", "not_2", "input");
            // net.connect_nodes("not_2", "output", "bool_print", "input");

            // //net->nodes["not_2"]->get_output_object("output", out);

            // //net->nodes[1]->connect_input("input", net->nodes[4], "output"); // connect first not to fourth not (creates circular dependency)

            // //std::cout << "output: " << *reinterpret_cast<bool*>(out->data_pointer) << std::endl;

            // net.rebuild_execution_order();    
            // net.run(&cycle);
            // cycle++;

            // std::this_thread::sleep_for(std::chrono::seconds(2));

            // net.run(&cycle);
            // cycle++;

            // std::this_thread::sleep_for(std::chrono::seconds(2));

            // // net->connect_nodes("not_2", "output", "and_0", "input_A");
            // // net->connect_nodes("not_0", "output", "and_0", "input_B");
            // // net->connect_nodes("and_0", "output", "bool_print", "input");

            // // net->rebuild_execution_order();
            // net.run(&cycle);
        
        }
        uint32_t run_update(){
            // run all networks in order

            if(!enable){    // do not run if not enabled
                return 0;
            }

            for(auto& net : network_execution_order){
                net->run(&cycle);
            }

            cycle++;

            if(single_cycle_step){
                enable = false;
            }

            return 0;
        }

        uint32_t set_enable(bool enable_){
            enable = enable_;
            return 0;
        }

        uint32_t set_single_cycle_step(bool single_cycle_step_){
            single_cycle_step = single_cycle_step_;
            return 0;
        }

        uint32_t add_node(std::string network_name, std::string node_name, std::string node_type){
            // creates a new node with the given name and type

            if(networks.find(network_name) == networks.end()){
                std::cerr << "Error: network name not found" << std::endl;
                return 1;
            }

            if(networks[network_name]->add_node(node_type, node_name) != 0){
                std::cerr << "Error: failed to create node" << std::endl;
                return 3;
            }

            return 0;
        }

        uint32_t remove_node(std::string network_name, std::string node_name){
            // removes a node with the given name

            if(networks.find(network_name) == networks.end()){
                std::cerr << "Error: network name not found" << std::endl;
                return 1;
            }

            if(networks[network_name]->remove_node(node_name) != 0){
                std::cerr << "Error: failed to remove node" << std::endl;
                return 3;
            }

            return 0;
        }

        uint32_t create_global_variable(std::string name, io_type type){
            // creates a new global variable with the given name and type

            if(global_variables.find(name) != global_variables.end()){
                std::cerr << "Error: global variable name already exists" << std::endl;
                return 1;
            }

            global_variables[name] = std::make_shared<global_variable>(type);

            return 0;
        }

        uint32_t delete_global_variable(std::string name){
            // deletes a global variable with the given name

            if(global_variables.find(name) == global_variables.end()){
                std::cerr << "Error: global variable name not found" << std::endl;
                return 1;
            }

            if(global_variables[name]->is_in_use()){
                std::cerr << "Error: global variable is still in use" << std::endl;
                return 2;
            }
            global_variables.erase(name);
        }

        uint32_t rename_global_variable(std::string old_name, std::string new_name){
            // renames the global variable with the given old name to the new name

            if(global_variables.find(old_name) == global_variables.end()){
                std::cerr << "Error: global variable name not found" << std::endl;
                return 1;
            }

            if(global_variables.find(new_name) != global_variables.end()){
                std::cerr << "Error: new global variable name already exists" << std::endl;
                return 2;
            }

            global_variables[new_name] = global_variables[old_name];
            //global_variables[new_name]->set_name(new_name);
            global_variables.erase(old_name);

            return 0;
        }

        uint32_t create_network(std::string name){
            // creates a new network with the given name, will not run until configured

            if(networks.size() >= max_networks){
                std::cerr << "Error: maximum number of networks reached" << std::endl;
                return 1;
            }

            if(networks.find(name) != networks.end()){
                std::cerr << "Error: network name already exists" << std::endl;
                return 2;
            }

            networks[name] = std::make_shared<node_network>();

            return 0;
        }

        uint32_t rename_network(std::string old_name, std::string new_name){
            // renames the network with the given old name to the new name, execution order will be maintained since the network is a shared pointer

            if(networks.find(old_name) == networks.end()){
                std::cerr << "Error: network name not found" << std::endl;
                return 1;
            }

            if(networks.find(new_name) != networks.end()){
                std::cerr << "Error: new network name already exists" << std::endl;
                return 2;
            }

            networks[new_name] = networks[old_name];
            networks.erase(old_name);

            return 0;
        }

        uint32_t delete_network(std::string name){
            // deletes the network with the given name

            if(networks.find(name) == networks.end()){
                std::cerr << "Error: network name not found" << std::endl;
                return 1;
            }

            network_execution_order[networks[name]->get_execution_order()] = nullptr;
            networks.erase(name);
            
            return 0;
        }

        uint32_t configure_network(std::string name, json* data){
            // configure the network with the given name

            // make sure the network exists
            if(networks.find(name) == networks.end()){
                std::cerr << "Error: network name not found" << std::endl;
                return 1;
            }

            // don't allow configuring the network while it is running or enabled
            if(networks[name]->config_allowed()){
                std::cerr << "Error: cannot configure network while it is running" << std::endl;
                return 2;
            }

            // parse JSON data and configure the network

            if(data->contains("enable")){
                if(!data->at("enable").is_boolean()){
                    std::cerr << "Error: enable is not a boolean" << std::endl;
                }
                else{
                    networks[name]->set_enable(data->at("enable").get<bool>());
                }
                data->erase("enable");
            }

            if(data->contains("type")){
                if(!data->at("type").is_string()){
                    std::cerr << "Error: network type is not a string" << std::endl;
                }
                else if(data->at("type").get<std::string>().compare("sync") == 0){
                    networks[name]->set_type(node_network::update_type::SYNC);
                }
                else if(data->at("type").get<std::string>().compare("async") == 0){
                    networks[name]->set_type(node_network::update_type::ASYNC);
                }
                else{
                    std::cerr << "Error: unknown network type" << std::endl;
                }
                data->erase("type");
            }
            
            if(data->contains("timeout_usec")){
                if(!data->at("timeout_usec").is_number()){
                    std::cerr << "Error: timeout_usec is not a number" << std::endl;
                }
                else{
                    networks[name]->set_timeout_usec(data->at("timeout_usec").get<uint32_t>());
                }
                data->erase("timeout_usec");
            }

            if(data->contains("timeout_msec")){
                if(!data->at("timeout_msec").is_number()){
                    std::cerr << "Error: timeout_msec is not a number" << std::endl;
                }
                else{
                    networks[name]->set_timeout_msec(data->at("timeout_msec").get<uint32_t>());
                }
                data->erase("timeout_msec");
            }

            if(data->contains("timeout_sec")){
                if(!data->at("timeout_sec").is_number()){
                    std::cerr << "Error: timeout_sec is not a number" << std::endl;
                }
                else{
                    networks[name]->set_timeout_sec(data->at("timeout_sec").get<uint32_t>());
                }
                data->erase("timeout_sec");
            }

            if(data->contains("update_cycle_trigger_count")){
                if(!data->at("update_cycle_trigger_count").is_number()){
                    std::cerr << "Error: update_cycle_trigger_count is not a number" << std::endl;
                }
                else{
                    networks[name]->set_update_cycle_trigger_count(data->at("update_cycle_trigger_count").get<uint32_t>());
                }
                data->erase("update_cycle_trigger_count");
            }

            if(data->contains("allowed_async_late_cycles")){
                if(!data->at("allowed_async_late_cycles").is_number()){
                    std::cerr << "Error: allowed_async_late_cycles is not a number" << std::endl;
                }
                else{
                    networks[name]->set_async_allowed_late_cycles(data->at("allowed_async_late_cycles").get<uint32_t>());
                }
                data->erase("allowed_async_late_cycles");
            }

            if(data->contains("execution_order")){
                if(!data->at("execution_order").is_number()){
                    std::cerr << "Error: execution_order is not a number" << std::endl;
                }
                else{
                    uint32_t exec_num = data->at("execution_order").get<uint32_t>();

                    if(exec_num >= network_execution_order.size()){
                        std::cerr << "Error: execution_order is out of range" << std::endl;
                    }

                    else{
                        // update execution order vector
                        if(network_execution_order[exec_num] == nullptr){  // execution number is available
                            network_execution_order[exec_num] = networks[name];
                            // update selected network
                            networks[name]->set_execution_order(exec_num);
                        }
                        else{   // number is taken
                            std::cerr << "Error: execution order number is already taken" << std::endl;
                        }
                    }
                }
                data->erase("execution_order");
            }


            if (!data->empty()) {
                std::cerr << "Error: unknown configuration parameters in JSON data" << std::endl;
                return 3;
            }

            return 0;
        }
        

};
