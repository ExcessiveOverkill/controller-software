#include "node_core.h"
#include <filesystem>
#include <fstream>


Node_Core::Node_Core(){
}

uint32_t Node_Core::run_update(){
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

uint32_t Node_Core::save(std::string file, bool write_protected){
    // save entire node config to a json file
    
    // check if the file already exists and if it is write protected
    if(std::filesystem::exists(file)){
        // open and parse the file to check if it is write protected
        std::ifstream f(file);
        if (!f.is_open()) {
            std::cerr << "Error: Node config file already exists but is unable to be opened to check write permission" << std::endl;
            return 1;
        }

        try{
            json temp = json::parse(f);
            if(temp["info"]["write_protected"].get<bool>()){
                std::cerr << "Error: Node config file already exists and is write protected" << std::endl;
                return 3;
            }
        }
        catch(json::parse_error& e){
            std::cerr << "Error: Node config file exists but is unable to be parsed to check write permission" << std::endl;
            return 2;
        }
        f.close();
    }
    
    
    json j;

    // add info header
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_r(&now_time_t, &now_tm);
    std::stringstream dateStream;
    dateStream << std::put_time(&now_tm, "%Y-%m-%d");
    std::string dateStr = dateStream.str();
    std::stringstream timeStream;
    timeStream << std::put_time(&now_tm, "%H:%M:%S");
    std::string timeStr = timeStream.str();

    j["info"]["date"] = dateStr;
    j["info"]["time"] = timeStr;
    j["info"]["fpga_config"] = fpga_config_file;
    j["info"]["write_protected"] = write_protected;



    // add vars
    for(auto& var : global_variables){
        j["node_vars"][var.first] = var.second->export_json();
    }

    // TODO: add networks
    // add network configs
    // add nodes to networks
    // add connections


    // create directory if it does not exist and save file
    std::filesystem::path p(file);
    if (!std::filesystem::exists(p.parent_path())) {
        try {
            std::filesystem::create_directories(p.parent_path());
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error: could not create node config directory: " << e.what() << std::endl;
            return 5;
        }
    }
    std::ofstream o(file);
    if(!o.is_open()){
        std::cerr << "Error: Unable to open node config file for writing" << std::endl;
        return 4;
    }
    o << std::setw(4) << j << std::endl;
    o.close();

    std::cout << "Node config saved to: " << file << std::endl;

    return 0;
}

uint32_t Node_Core::load(std::string file){
    
    // load entire node config from a json file, this includes all networks, nodes, and connections, along with optional configs for every node and presets for variables
    /*

    {
    info: {
        date: "2021-01-01",
        time: "12:00:00",
        fpga_config: "fpga_config_file.json",
        write_protected: false
    },
    
    user_node_vars: {    // create global variables not already created by fpga drivers
        "var_name": {
            type: "uint32"
        }
    },

    node_var_values: {    // set any global variable values that are not configured to use an external data pointer
        "var_name": {
            value: 0
        }
    },

    networks: {
        "network_name": {
            type: "sync",
            enable: true,
            cycle_trigger_count: 1,
            timeout_usec: 10000,
            execution_index: 0,
            nodes: {
                "node_name": {
                    type: "node_type",
                    config: {
                        "config_name": "config_value"
                    }
                }
            },
            connections: {
                
            }
        }
    
    
    }

    */

    std::ifstream i(file);
    if(!i.is_open()){
        std::cerr << "Error: Unable to open node config file" << std::endl;
        return 1;
    }

    json j;
    try{
        i >> j;
    }
    catch(json::parse_error& e){
        std::cerr << "Error: unable to parse node config file" << std::endl;
        return 2;
    }
    i.close();

    // verify fpga_config matches
    std::string fpga_config;
    j["info"]["fpga_config"].get_to(fpga_config);
    if(fpga_config != fpga_config_file){
        std::cerr << "Error: FPGA config file does not match" << std::endl;
        return 3;
    }

    // create user global variables
    for(auto& var : j["user_node_vars"].items()){
        io_type type = get_io_type(var.value()["type"].get<std::string>());
        if(type == io_type::UNDEFINED){
            std::cerr << "Error: invalid type for global variable: " << var.value()["type"].get<std::string>() << std::endl;
            return 1;
        }
        if(create_global_variable(var.key(), type)){
            std::cerr << "Error: failed to create global variable" << std::endl;
            return 1;
        }
    }

    // set global variable values
    for(auto& var : j["node_var_values"].items()){
        
        if(global_variables.find(var.key()) == global_variables.end()){
            std::cerr << "Error: global variable not found: " << var.key() << std::endl;
            return 1;
        }

        // TODO: add more types

        switch(get_global_variable_type(var.key())){
            case io_type::UINT32:
                {
                    uint32_t value = var.value().get<uint32_t>();
                    if(set_global_variable_value(var.key(), &value)){
                        std::cerr << "Error: failed to set global variable value" << std::endl;
                        return 1;
                    }
                }
                break;
            case io_type::INT32:
                {
                    int32_t value = var.value().get<int32_t>();
                    if(set_global_variable_value(var.key(), &value)){
                        std::cerr << "Error: failed to set global variable value" << std::endl;
                        return 1;
                    }
                }
                break;
            case io_type::FLOAT:
                {
                    float value = var.value().get<float>();
                    if(set_global_variable_value(var.key(), &value)){
                        std::cerr << "Error: failed to set global variable value" << std::endl;
                        return 1;
                    }
                }
                break;
            case io_type::DOUBLE:
                {
                    double value = var.value().get<double>();
                    if(set_global_variable_value(var.key(), &value)){
                        std::cerr << "Error: failed to set global variable value" << std::endl;
                        return 1;
                    }
                }
                break;
            case io_type::BOOL:
                {
                    bool value = var.value().get<bool>();
                    if(set_global_variable_value(var.key(), &value)){
                        std::cerr << "Error: failed to set global variable value" << std::endl;
                        return 1;
                    }
                }
                break;
            default:
                std::cerr << "Error: unable to set variable value, invalid type" << std::endl;
                return 1;
        }

    }

    // create and configure networks
    for(auto& network : j["networks"].items()){
        std::string network_name = network.key();
        if(create_network(network_name)){
            std::cerr << "Error: failed to create network" << std::endl;
            return 1;
        }

        // add nodes
        for(auto& node : network.value()["nodes"].items()){
            std::string node_name = node.key();
            std::string node_type = node.value()["type"].get<std::string>();
            if(add_node(network_name, node_name, node_type)){
                std::cerr << "Error: failed to add node to network" << std::endl;
                return 1;
            }
            // configure node
            if(node.value().contains("config")){
                if(configure_node(network_name, node_name, &node.value()["config"])){
                    std::cerr << "Error: failed to configure node" << std::endl;
                    return 1;
                }
            }
        }

        // connect nodes
        for(auto& connection : network.value()["connections"].items()){
            std::string src_node = connection.value()["src_node"].get<std::string>();
            std::string src_port = connection.value()["src_port"].get<std::string>();
            std::string dst_node = connection.value()["dst_node"].get<std::string>();
            std::string dst_port = connection.value()["dst_port"].get<std::string>();
            if(connect_nodes(network_name, src_node, src_port, dst_node, dst_port)){
                std::cerr << "Error: failed to connect nodes" << std::endl;
                return 1;
            }
        }

        if(rebuild_execution_order(network_name)){
            std::cerr << "Error: failed to rebuild execution order" << std::endl;
            return 1;
        }

        // configure network
        if(configure_network(network_name, &network.value())){
            std::cerr << "Error: failed to configure network" << std::endl;
            return 1;
        }

    }

    
    return 0;
}

uint32_t Node_Core::set_fpga_config(std::string name){
    // set the fpga config file used
    // this is only used as additional info in the save file

    fpga_config_file = name;

    return 0;
}

uint32_t Node_Core::set_enable(bool enable_){
    enable = enable_;
    return 0;
}

uint32_t Node_Core::set_single_cycle_step(bool single_cycle_step_){
    single_cycle_step = single_cycle_step_;
    return 0;
}

uint32_t Node_Core::add_node(std::string network_name, std::string node_name, std::string node_type){
    // creates a new node with the given name and type

    if(networks.find(network_name) == networks.end()){
        std::cerr << "Error: network name not found" << std::endl;
        return 1;
    }

    if(networks[network_name]->add_node(node_type, node_name) != 0){
        std::cerr << "Error: failed to create node" << std::endl;
        return 3;
    }

    std::cout << "Created node: " << node_name << std::endl;

    return 0;
}

uint32_t Node_Core::remove_node(std::string network_name, std::string node_name){
    // removes a node with the given name

    if(networks.find(network_name) == networks.end()){
        std::cerr << "Error: network name not found" << std::endl;
        return 1;
    }

    if(networks[network_name]->remove_node(node_name) != 0){
        std::cerr << "Error: failed to remove node" << std::endl;
        return 3;
    }

    std::cout << "Removed node: " << node_name << std::endl;

    return 0;
}

uint32_t Node_Core::configure_node(std::string network_name, std::string node_name, json* data){
    // configures a node with the given name

    if(networks.find(network_name) == networks.end()){
        std::cerr << "Error: network name not found" << std::endl;
        return 1;
    }

    if(networks[network_name]->configure_node(node_name, data) != 0){
        std::cerr << "Error: failed to configure node" << std::endl;
        return 2;
    }

    //std::cout << "Configured node: " << node_name << std::endl;

    return 0;
}

uint32_t Node_Core::connect_nodes(std::string network_name, std::string source_node_name, std::string source_output_name, std::string target_node_name, std::string target_input_name){
    // connects an output to an input

    if(networks.find(network_name) == networks.end()){
        std::cerr << "Error: network name not found" << std::endl;
        return 1;
    }

    if(networks[network_name]->connect_nodes(source_node_name, source_output_name, target_node_name, target_input_name) != 0){
        std::cerr << "Failed to connect nodes: " << source_node_name << "." << source_output_name << " -> " << target_node_name << "." << target_input_name << std::endl;

        return 2;
    }

    std::cout << "Connected nodes: " << source_node_name << "." << source_output_name << " -> " << target_node_name << "." << target_input_name << std::endl;

    return 0;
}

uint32_t Node_Core::create_global_variable(std::string name, io_type type){
    // creates a new global variable with the given name and type

    if(global_variables.find(name) != global_variables.end()){
        std::cerr << "Error: global variable name already exists" << std::endl;
        return 1;
    }

    global_variables[name] = std::make_shared<global_variable>(name, type);

    std::cout << "Created global variable: " << name << std::endl;

    return 0;
}

uint32_t Node_Core::get_global_variable_data_ptr(std::string name, void** data){
    // gets the data pointer for the global variable with the given name

    if(global_variables.find(name) == global_variables.end()){
        std::cerr << "Error: global variable name not found: " << name << std::endl;
        return 1;
    }

    global_variables[name]->get_data_pointer(data);

    return 0;
}

uint32_t Node_Core::set_global_variable_data_ptr(std::string name, void* data){
    // sets the data pointer for the global variable with the given name
    // this will modify all pointers to the current data to use the new data pointer

    if(global_variables.find(name) == global_variables.end()){
        std::cerr << "Error: global variable name not found: " << name << std::endl;
        return 1;
    }

    global_variables[name]->set_data_pointer(data);

    return 0;
}

uint32_t Node_Core::set_global_variable_value(std::string name, void* value){
    // sets the value for the global variable with the given name

    if(global_variables.find(name) == global_variables.end()){
        std::cerr << "Error: global variable name not found: " << name << std::endl;
        return 1;
    }
    return global_variables[name]->set_value(value);
}

io_type Node_Core::get_global_variable_type(std::string name){
    // gets the type of the global variable with the given name

    if(global_variables.find(name) == global_variables.end()){
        std::cerr << "Error: global variable name not found: " << name << std::endl;
        return io_type::UNDEFINED;
    }

    return global_variables[name]->get_type();
}

uint32_t Node_Core::delete_global_variable(std::string name){
    // deletes a global variable with the given name

    if(global_variables.find(name) == global_variables.end()){
        std::cerr << "Error: global variable name not found: " << name << std::endl;
        return 1;
    }

    if(global_variables[name]->is_in_use()){
        std::cerr << "Error: global variable is still in use" << std::endl;
        return 2;
    }
    global_variables.erase(name);

    std::cout << "Deleted global variable: " << name << std::endl;

    return 0;
}

uint32_t Node_Core::rename_global_variable(std::string old_name, std::string new_name){
    // renames the global variable with the given old name to the new name

    if(global_variables.find(old_name) == global_variables.end()){
        std::cerr << "Error: global variable name not found: " << old_name << std::endl;
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

uint32_t Node_Core::create_network(std::string name){
    // creates a new network with the given name, will not run until configured

    if(networks.size() >= max_networks){
        std::cerr << "Error: maximum number of networks reached" << std::endl;
        return 1;
    }

    if(networks.find(name) != networks.end()){
        std::cerr << "Error: network name already exists" << std::endl;
        return 2;
    }

    networks[name] = std::make_shared<node_network>(&global_variables);

    network_execution_order.push_back(networks[name]);
    networks[name]->set_execution_order(network_execution_order.size()-1);

    std::cout << "Created network: " << name << std::endl;

    return 0;
}

uint32_t Node_Core::rename_network(std::string old_name, std::string new_name){
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

    // TODO: update execution order

    return 0;
}

uint32_t Node_Core::delete_network(std::string name){
    // deletes the network with the given name

    if(networks.find(name) == networks.end()){
        std::cerr << "Error: network name not found" << std::endl;
        return 1;
    }

    network_execution_order[networks[name]->get_execution_order()] = nullptr;
    networks.erase(name);

    std::cout << "Deleted network: " << name << std::endl;
    
    return 0;
}

uint32_t Node_Core::configure_network(std::string name, json* data){
    // configure the network with the given name

    // make sure the network exists
    if(networks.find(name) == networks.end()){
        std::cerr << "Error: network name not found" << std::endl;
        return 1;
    }

    // don't allow configuring the network while it is running or enabled
    if(!networks[name]->config_allowed()){
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
        //data->erase("enable");
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
        //data->erase("type");
    }
    
    if(data->contains("timeout_usec")){
        if(!data->at("timeout_usec").is_number()){
            std::cerr << "Error: timeout_usec is not a number" << std::endl;
        }
        else{
            networks[name]->set_timeout_usec(data->at("timeout_usec").get<uint32_t>());
        }
        //data->erase("timeout_usec");
    }

    if(data->contains("timeout_msec")){
        if(!data->at("timeout_msec").is_number()){
            std::cerr << "Error: timeout_msec is not a number" << std::endl;
        }
        else{
            networks[name]->set_timeout_msec(data->at("timeout_msec").get<uint32_t>());
        }
        //data->erase("timeout_msec");
    }

    if(data->contains("timeout_sec")){
        if(!data->at("timeout_sec").is_number()){
            std::cerr << "Error: timeout_sec is not a number" << std::endl;
        }
        else{
            networks[name]->set_timeout_sec(data->at("timeout_sec").get<uint32_t>());
        }
        //data->erase("timeout_sec");
    }

    if(data->contains("update_cycle_trigger_count")){
        if(!data->at("update_cycle_trigger_count").is_number()){
            std::cerr << "Error: update_cycle_trigger_count is not a number" << std::endl;
        }
        else{
            networks[name]->set_update_cycle_trigger_count(data->at("update_cycle_trigger_count").get<uint32_t>());
        }
        //data->erase("update_cycle_trigger_count");
    }

    if(data->contains("allowed_async_late_cycles")){
        if(!data->at("allowed_async_late_cycles").is_number()){
            std::cerr << "Error: allowed_async_late_cycles is not a number" << std::endl;
        }
        else{
            networks[name]->set_async_allowed_late_cycles(data->at("allowed_async_late_cycles").get<uint32_t>());
        }
        //data->erase("allowed_async_late_cycles");
    }

    if(data->contains("execution_order")){
        if(!data->at("execution_order").is_number()){
            std::cerr << "Error: execution_order is not a number" << std::endl;
        }
        else{
            uint32_t exec_num = data->at("execution_order").get<uint32_t>();

            // TODO: implement this properly

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
        //data->erase("execution_order");
    }


    // if (!data->empty()) {
    //     std::cerr << "Error: unknown configuration parameters in JSON data" << std::endl;
    //     return 3;
    // }

    return 0;
}

uint32_t Node_Core::rebuild_execution_order(std::string name){
    // rebuilds the execution order for the network with the given name

    if(networks.find(name) == networks.end()){
        std::cerr << "Error: network name not found" << std::endl;
        return 1;
    }

    if(networks[name]->rebuild_execution_order() != 0){
        std::cerr << "Error: failed to rebuild execution order" << std::endl;
        return 2;
    }

    std::cout << "Rebuilt execution order for network: " << name << std::endl;

    return 0;
}