#include "node_io.h"

output::output(io_type type_, void* data, int* source_execution_number_){
    type = type_;
    data_pointer = data;
    source_execution_number = source_execution_number_;
}

void output::mark_for_deletion(){
    delete_flag = true;
}

input::input(io_type type_, void* default_value_){
    type = type_;
    default_value = default_value_;

    if(default_value != nullptr){
        data_pointer = default_value;
        return;
    }

    // create default value based on type if none provided
    switch (type)
    {
    case io_type::BOOL:
        default_value = new bool(false);
        break;
    case io_type::UINT8:
        default_value = new uint8_t(0);
        break;
    case io_type::UINT16:
        default_value = new uint16_t(0);
        break;
    case io_type::UINT32:
        default_value = new uint32_t(0);
        break;
    case io_type::INT8:
        default_value = new int8_t(0);
        break;
    case io_type::INT16:
        default_value = new int16_t(0);
        break;
    case io_type::INT32:
        default_value = new int32_t(0);
        break;
    case io_type::FLOAT:
        default_value = new float(0.0);
        break;
    case io_type::DOUBLE:
        default_value = new double(0.0);
        break;
    case io_type::EM_SERIAL_DEVICE:
        default_value = nullptr;   // no default
        break;
    }

    data_pointer = default_value;

}

uint32_t input::set_default(){
    // if(default_value == nullptr){
    //     std::cerr << "Error: default value is null" << std::endl;
    //     return 1;   // default value is null
    // }
    source_execution_number = &default_execution_number;
    data_pointer = default_value;
    source_output = nullptr;
    return 0;
}

uint32_t input::set_input_data(output* source){
    uint32_t ret = 0;
    if(source == nullptr){
        std::cerr << "Error: source is null" << std::endl;
        ret = 1;   // source is null
        if(set_default()){
            ret = 6;   // error setting input source and setting default value
        }
    }

    else if(source->type != type){
        std::cerr << "Error: type mismatch: " << get_io_type_string(source->type) <<  " -> " << get_io_type_string(type) << std::endl;
        ret = 2;   // type mismatch
    }

    else if(source->data_pointer == nullptr){
        std::cerr << "Error: data pointer is null" << std::endl;
        ret = 3;   // data pointer is null
    }

    else if(source->delete_flag){
        std::cerr << "Error: source output is marked for deletion" << std::endl;
        ret = 4;   // source output is marked for deletion
    }

    else if(source->source_execution_number == nullptr){
        std::cerr << "Error: source execution number is null" << std::endl;
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

uint32_t input::reconnect(){
    return set_input_data(source_output);
}