#include <iostream>


#pragma once

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