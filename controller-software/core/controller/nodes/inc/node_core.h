#pragma once

#include <map>
#include <chrono>
#include <fstream>

// #ifndef NODE_CORE_H
// #define NODE_CORE_H
#include "node_network.h"
#include "base_global_variable.h"

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

        std::string fpga_config_file = "";   // file used to configure the fpga

    public:
        Node_Core();

        uint32_t run_update();

        uint32_t save(std::string file, bool write_protected = false);

        uint32_t load(std::string file);

        uint32_t set_fpga_config(std::string name);

        uint32_t set_enable(bool enable_);

        uint32_t set_single_cycle_step(bool single_cycle_step_);

        uint32_t add_node(std::string network_name, std::string node_name, std::string node_type);

        uint32_t remove_node(std::string network_name, std::string node_name);

        uint32_t configure_node(std::string network_name, std::string node_name, json* data);

        uint32_t connect_nodes(std::string network_name, std::string source_node_name, std::string source_output_name, std::string target_node_name, std::string target_input_name);

        //uint32_t connect_varaiable_get(std::string target_node_name);    // only for get_global_variable nodes

        //uint32_t connect_varaiable_set(std::string target_node_name);    // only for set_global_variable nodes

        uint32_t create_global_variable(std::string name, io_type type);

        uint32_t get_global_variable_data_ptr(std::string name, void** data);

        uint32_t set_global_variable_data_ptr(std::string name, void* data);

        uint32_t set_global_variable_value(std::string name, void* value);

        io_type get_global_variable_type(std::string name);

        uint32_t delete_global_variable(std::string name);

        uint32_t rename_global_variable(std::string old_name, std::string new_name);

        uint32_t create_network(std::string name);

        uint32_t rename_network(std::string old_name, std::string new_name);

        uint32_t delete_network(std::string name);

        uint32_t configure_network(std::string name, json* data);

        uint32_t rebuild_execution_order(std::string name);
        
};

//#endif