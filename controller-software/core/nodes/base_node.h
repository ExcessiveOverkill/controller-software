#include "../json.hpp"
#include <map>

#pragma once

using json = nlohmann::json;

enum io_type{
    UNDEFINED,
    UINT32,
    INT32,
    DOUBLE,
    BOOL
};

class output{
    public:
        io_type type = UNDEFINED;

        int* source_execution_number = nullptr;

        bool delete_flag = false;

        void* data_pointer = nullptr;

        output() = default;

        output(io_type type_, void* data, int* source_execution_number_){
            type = type_;
            data_pointer = data;
            source_execution_number = source_execution_number_;
        }

        void mark_for_deletion(){
            delete_flag = true;
        }

        ~output(){
            // do nothing
        }
};

class input{
    private:
        io_type type = UNDEFINED;

        int default_execution_number = -256;

        void* default_value = nullptr;

        output* source_output = nullptr;

        
        
    public:

        int* source_execution_number = nullptr;

        void* data_pointer = nullptr;
        
        input() = default;

        input(io_type type_, void* default_value_){
            type = type_;
            default_value = default_value_;
        }

        unsigned int set_default(){
            if(default_value == nullptr){
                std::cerr << "Error: default value is null" << std::endl;
                return 1;   // default value is null
            }
            source_execution_number = &default_execution_number;
            data_pointer = default_value;
            source_output = nullptr;
            return 0;
        }

        // set where the input should retreive its data from
        unsigned int set_input_data(output* source){
            unsigned int ret = 0;
            if(source == nullptr){
                ret = 1;   // source is null
                if(set_default()){
                    ret = 6;   // error setting input source and setting default value
                }
            }

            else if(source->type != type){
                ret = 2;   // type mismatch
            }

            else if(source->data_pointer == nullptr){
                ret = 3;   // data pointer is null
            }

            else if(source->delete_flag){
                ret = 4;   // source output is marked for deletion
            }

            else if(source->source_execution_number == nullptr){
                ret = 5;   // source execution number is null
            }
            
            if(ret){
                return ret;
            }

            else{
                source_output = source;
                data_pointer = source->data_pointer;
                source_execution_number = source->source_execution_number;
            }
            return 0;
        }

        unsigned int reconnect(){
            return set_input_data(source_output);
        }

        ~input(){
            // do nothing
        }
};

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

        unsigned int connect_input(std::string input_name, base_node* source_node, std::string source_output_name){
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