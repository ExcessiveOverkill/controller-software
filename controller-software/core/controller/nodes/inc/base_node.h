#include "json.hpp"
#include <map>
#include "node_io.h"
#include "base_global_variable.h"

#pragma once

using json = nlohmann::json;

// TODO: add settings support to configure nodes

class base_node{
    protected:
        std::map<std::string, input> inputs;
        std::map<std::string, output> outputs;

        std::string type = "";

        bool marked_for_deletion = false;

    public:

        int execution_number = -256;

        base_node();
        uint32_t get_output_object(std::string output_name, output*& ptr);
        uint32_t connect_input(std::string input_name, std::shared_ptr<base_node> source_node, std::string source_output_name);
        uint32_t connect_input(std::string input_name, output* source_output);

        // only for set/get global var nodes
        virtual uint32_t set_global_var_input(std::string input_name, global_variable* variable);
        virtual uint32_t set_global_var_output(std::string output_name, global_variable* variable);

        std::string get_type(){return type;}


        uint32_t disconnect_input(std::string input_name);
        uint32_t reconnect_inputs();
        uint32_t configure_execution_number(bool* execution_number_modified);
        void mark_for_deletion();
        virtual uint32_t run() = 0;
        virtual uint32_t configure_settings(json* data);
        ~base_node();
};