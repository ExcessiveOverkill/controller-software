#include "json.hpp"
#include <map>
#include "node_io.h"

#pragma once

using json = nlohmann::json;

// TODO: add settings support to configure nodes

class base_node{
    protected:
        std::map<std::string, input> inputs;
        std::map<std::string, output> outputs;

        bool marked_for_deletion = false;

    public:

        int execution_number = -256;

        base_node(){
            // do nothing
        }

        unsigned int get_output_object(std::string output_name, output*& ptr){
            if(outputs.find(output_name) == outputs.end()){
                return 1;   // output name not found
            }
            ptr = &outputs[output_name];
            return 0;
        }

        unsigned int connect_input(std::string input_name, std::shared_ptr<base_node> source_node, std::string source_output_name){
            if(inputs.find(input_name) == inputs.end()){
                return 1;   // internal input name not found
            }

            output* output_pointer = nullptr;
            if(source_node->get_output_object(source_output_name, output_pointer) != 0){
                return 2;   // source output name not found
            }

            if (inputs[input_name].set_input_data(output_pointer) != 0){
                return 3;   // error setting input data
            }

            return 0;
        }

        unsigned int connect_input(std::string input_name, output* source_output){
            if(inputs.find(input_name) == inputs.end()){
                return 1;   // internal input name not found
            }

            if (inputs[input_name].set_input_data(source_output) != 0){
                return 2;   // error setting input data
            }

            return 0;
        }

        unsigned int disconnect_input(std::string input_name){
            if(inputs.find(input_name) == inputs.end()){
                return 1;   // internal input name not found
            }

            if (inputs[input_name].set_default() != 0){
                return 2;   // error setting default value
            }

            return 0;
        }

        // fix any inputs pointing to invalid ouitputs by switching to the default value
        unsigned int reconnect_inputs(){
            for (auto& pair : inputs) {
                pair.second.reconnect();
            }
            return 0;
        }

        unsigned int configure_execution_number(bool* execution_number_modified){
            if(reconnect_inputs() != 0){
                return 1;   // error reconnecting inputs
            }

            // find the highest execution number of all inputs
            int highest_execution_input = -256;
            for (auto& pair : inputs) {
                if(*pair.second.source_execution_number > highest_execution_input){
                    highest_execution_input = *pair.second.source_execution_number;
                }
            }

            // update the execution number if needed and signal it was modified
            if(execution_number != highest_execution_input + 1){
                *execution_number_modified = true;
                execution_number = highest_execution_input + 1;
            }
            
            return 0;
        }

        // mark all outputs for deletion
        void mark_for_deletion(){
            for (auto& pair : outputs) {
                pair.second.mark_for_deletion();
            }
            marked_for_deletion = true;
        }

        // run node
        virtual unsigned int run() = 0;

        // configure internal settings
        virtual unsigned int configure_settings(json* data){
            return 1;   // no settings to configure
        }

        ~base_node(){
            // do nothing
        }
};