#pragma once

#include <iostream>
#include <map>

// All data types that can be used in the node editor, this includes even custom objects
enum io_type{
    UNDEFINED,
    UINT8,
    UINT16,
    UINT32,
    INT8,
    INT16,
    INT32,
    DOUBLE,
    BOOL,
    FLOAT,
    EM_SERIAL_DEVICE
};

static const std::map<io_type, size_t> io_type_size = {
    {io_type::UINT8, sizeof(uint8_t)},
    {io_type::INT8, sizeof(int8_t)},
    {io_type::UINT16, sizeof(uint16_t)},
    {io_type::INT16, sizeof(int16_t)},
    {io_type::UINT32, sizeof(uint32_t)},
    {io_type::INT32, sizeof(int32_t)},
    {io_type::DOUBLE, sizeof(double)},
    {io_type::BOOL, sizeof(bool)},
    {io_type::FLOAT, sizeof(float)}
};

static io_type get_io_type(std::string type){

    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    
    if(type.find("uint8") != std::string::npos){
        return io_type::UINT8;
    }
    else if(type.find("int8") != std::string::npos){
        return io_type::INT8;
    }
    else if(type.find("uint16") != std::string::npos){
        return io_type::UINT16;
    }
    else if(type.find("int16") != std::string::npos){
        return io_type::INT16;
    }
    else if(type.find("uint32") != std::string::npos){
        return io_type::UINT32;
    }
    else if(type.find("int32") != std::string::npos){
        return io_type::INT32;
    }
    else if(type == "double"){
        return io_type::DOUBLE;
    }
    else if(type == "bool"){
        return io_type::BOOL;
    }
    else if(type == "float"){
        return io_type::FLOAT;
    }
    else if(type == "em_serial_device"){
        return io_type::EM_SERIAL_DEVICE;
    }
    else{
        return io_type::UNDEFINED;
    }
}

static std::string get_io_type_string(io_type type){
    switch(type){
        case io_type::UINT8: return "uint8";
        case io_type::INT8: return "int8";
        case io_type::UINT16: return "uint16";
        case io_type::INT16: return "int16";
        case io_type::UINT32: return "uint32";
        case io_type::INT32: return "int32";
        case io_type::DOUBLE: return "double";
        case io_type::BOOL: return "bool";
        case io_type::FLOAT: return "float";
        case io_type::EM_SERIAL_DEVICE: return "em_serial_device";
        default: return "undefined";
    }
}

class output{
    public:
        io_type type = UNDEFINED;

        int* source_execution_number = nullptr;

        bool delete_flag = false;

        void* data_pointer = nullptr;

        output() = default;

        output(io_type type_, void* data, int* source_execution_number_);

        void mark_for_deletion();

        io_type get_type(){return type;};
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

        input(io_type type_, void* default_value_);

        uint32_t set_default();

        // set where the input should retreive its data from
        uint32_t set_input_data(output* source);

        uint32_t reconnect();

        io_type get_type(){return type;};

};