#include "json.hpp"
#include <string>

#pragma once

using json = nlohmann::json;

/*
Helper class to load json address file

Goal is to make functions/objects that pair with the functions/objects used in the HDL code to initially create the address map
*/

class Register {
    public:
        void get_instruction_address(uint8_t* node, uint16_t* address);    // get the node and register address to be used in an instruction

    private:
        uint8_t node_index;
        uint32_t* raw_data_ptr; // data here can be any type, but we use uint32 for simplicity since it will always be 32 bits
};


template <typename T>
class Single_Register_Read : public Register {
public:
    operator T() const;     // Read the register
    const Single_Register_Read& operator[](int index) const; // Immutable access
    Single_Register_Read& operator[](int index);            // Mutable access

private:
    uint8_t register_width = 0;
    uint8_t bit_offset = 0;
};

template <typename T>
class Single_Register_Write : public Register{
    public:
    operator T() const;     // Read the register
    Single_Register_Write& operator=(const T& other);   // Write to the register
    const Single_Register_Write& operator[](int index) const; // Immutable access
    Single_Register_Write& operator[](int index);            // Mutable access

private:
    uint8_t register_width = 0;
    uint8_t bit_offset = 0;
};

template <typename T>
class Packed_Register_Read : public Register{
    public:
        operator T() const;     // Read the register
        Packed_Register_Read& operator=(const T& other);   // Write to the register
        const Packed_Register_Read& operator[](int index) const; // Immutable access
        Packed_Register_Read& operator[](int index);            // Mutable access

    private:
        uint8_t register_width = 0;
        uint8_t bit_offset = 0;
        uint32_t bit_mask = 0;
};

template <typename T>
class Packed_Register_Write : public Register{
    public:
        operator T() const;     // Read the register
        Packed_Register_Write& operator=(const T& other);   // Write to the register
        const Packed_Register_Write& operator[](int index) const; // Immutable access
        Packed_Register_Write& operator[](int index);            // Mutable access

    private:
        uint8_t register_width = 0;
        uint8_t bit_offset = 0;
        uint32_t bit_mask = 0;
};


class Register_Group {
    // basically just a wrapper around a vector of single registers
    public:
        const Register_Group& operator[](int index) const; // Immutable access
        Register_Group& operator[](int index);            // Mutable access

        std::vector<Register> registers;

    private:
        uint16_t group_size;
};


class Address_Map_Loader {
    public:
        Address_Map_Loader();
        ~Address_Map_Loader();

        void load_address_map(json* node_config);   // load the address map for a specific node

        template <typename T>
        Register get_register(std::string register_name);    // get a full register
        template <typename T>
        Register get_packed_register(std::string parent_register_name, std::string register_name);    // get a value from a packed register

        // Register get_register<T>(std::string register_name);    // get a full register
        // Register get_packed_register<T>(std::string parent_register_name, std::string register_name);    // get a value from a packed register

        // Register_Group get_registers_from_group<T>(std::string register_name, std::string group_name);    // get a register from a group
        // Register_Group get_packed_registers_from_group<T>(std::string parent_register_name, std::string register_name, std::string group_name);    // get a value from a packed register

    private:

        json* node_config;

        enum class variable_type : uint8_t {
            UINT8,
            UINT16,
            UINT32,
            INT8,
            INT16,
            INT32,
            //FLOAT,    // not implemented yet
            BOOL,
        };

        enum class variable_format : uint8_t {
            BOOL,
            SIGNED,
            UNSIGNED,
            //FLOAT,    // not implemented yet
            PACKED,
        };


        struct register_data{
            uint16_t address = 0;
            uint8_t count = 0;
            uint8_t width = 0;
            uint16_t group_index = 0;
            uint8_t starting_bit = 0;
            variable_format format;
            bool read = false;      // read and write registers are not curerntly supported
            bool write = false;
        };

        template <typename T>
        void verify_type_compatibility(register_data* config);    // make sure the type is compatible with the type in the json file

        uint32_t get_register_data_by_name(register_data* data, std::string register_name, std::string group_name);    // get the data for a register

        uint32_t get_packed_register_data_by_name(register_data* data, std::string sub_register_name, std::string register_name, std::string group_name);    // get the data for a register

        template <typename T>
        bool load_json_value(const json& config, const std::string& value_name, T* dest);    // helper function for loading values from the config file
        
};



// register groups should be indexed like arrays

// single registers should be created at init like this: reg = some_class.get_register<uint32_t>("register_name")
// packed registers should be created at init like this: reg = some_class.get_packed_register<bool>("parent_register_name", "register_name")
// then can be accessed like this: var = reg, reg = var

// grouped and registers with multiple instances should be created the same way as individual and packed registers
// but are instead accessed like this: var = reg[0], reg[0] = var

// instruction addresses can be obtained like this: reg.get_instruction_address(&node, &address), this will set the node and address to the correct values
// note that get_instruction_address will NOT work for packed registers, as instructions always move the entire 32bit register