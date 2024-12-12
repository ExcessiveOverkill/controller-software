#include "json.hpp"
#include <string>
#include "fpga_instructions.h"

#pragma once

using json = nlohmann::json;

/*
Helper class to load json address file

Goal is to make functions/objects that pair with the functions/objects used in the HDL code to initially create the address map
*/

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


struct register_json_data{
    uint16_t node_memory_address = 0;
    uint8_t count = 0;
    uint8_t width = 0;
    uint16_t group_index = 0;
    uint8_t starting_bit = 0;
    variable_format format;
    bool read = false;      // read and write registers are not curerntly supported
    bool write = false;
};

// struct mem_offsets{
//             // user address space (addresses directly accessible from software)
//             void* software_PS_PL_ptr = nullptr;
//             void* software_PL_PS_ptr = nullptr;
//             uint32_t software_ptr_offset = 0;

//             // hardware address space (node 0 memory)
//             uint32_t hardware_PS_PL_mem_offset = 0;
//             uint32_t hardware_PL_PS_mem_offset = 0;
//             uint32_t hardware_mem_offset = 0;
//         };



class Register {
    public:
        Register(json* json_data, const std::string register_name, uint16_t index, uint16_t parent_absolute_address);
        ~Register();

        template <typename T = uint32_t>
        Register* get_register(std::string register_name);  // get a sub-register, no index needed
        
        template <typename T>
        T* get_raw_data_ptr();    // get the raw data pointer

        virtual uint32_t create_copy_instructions(std::vector<uint64_t>*);    // create a copy instruction to copy the register to/from the PS/PL.


        // by default read/write functions throw errors unless they are overidden

        // Read the register
        #define READ_ERROR_DEFAULT "Unsupported conversion or cannot read from a write-only register"
        template <typename T = uint32_t>
        T get_value() const;

        // Write to the register
        #define WRITE_ERROR_DEFAULT "Unsupported conversion or cannot write to a read-only register"
        template <typename T = uint32_t>
        T set_value(const T& other);

        // write a single bit
        bool set_bit(bool value, uint8_t index);

        struct PL_data{
            std::string name;
            uint16_t address_offset = 0;
            uint16_t absolute_address = 0;
            uint32_t bank_size = 1;
            uint16_t width = 32;
            uint8_t starting_bit = 0;
            uint32_t bit_mask = -1;
            uint16_t index = 0;
            bool read = false;
            bool write = false;
            uint8_t node_index = 0;
        };

        // these are only set if the data is being synced with the PS
        // data here can be any type, but we use uint32 for simplicity since it will always be in 32 bit increments
        struct PS_data{
            uint32_t* software_data_ptr = nullptr;  // data in the PS memory
            uint16_t hardware_data_ptr = 0;     // data in the PL node 0 memory
        };

        PL_data pl_data;
        PS_data ps_data;

        bool is_sub_register = false;

        
    protected:
        fpga_mem* base_mem = nullptr;

        json* json_data = nullptr;

        void get_register_data(const std::string register_name);    // get the data for a register

        template <typename T>
        bool load_json_value(const json& config, const std::string& value_name, T* dest){
            // helper function for loading values from the config file
            if (!config.contains(value_name)) {
                std::cerr << "Error: Constant '" << value_name << "' not found in config." << std::endl;
                return false;
            }

            *dest = config[value_name].get<T>();
            return true;
        }



};


class Group {
    public:
    struct data{
        std::string name;
        uint16_t address_offset = 0;
        uint16_t absolute_address = 0;
        uint32_t alignment = 1;
        uint16_t count = 1;
        uint16_t index = 0;
    };

    Group(json* json_data, const std::string group_name, uint16_t index, uint16_t parent_absolute_address);

    template <typename T = uint32_t>
    Register* get_register(std::string register_name, uint16_t index);    // get a register in the group

    Group* get_group(std::string group_name, uint16_t index);    // get a group in the group
        
    data group_data;

    private:
        void get_group_data(const std::string group_name);    // get the data for a group

        json* json_data = nullptr;  // json data for the group

        template <typename T>
        bool load_json_value(const json& config, const std::string& value_name, T* dest){
            // helper function for loading values from the config file
            if (!config.contains(value_name)) {
                std::cerr << "Error: Constant '" << value_name << "' not found in config." << std::endl;
                return false;
            }

            *dest = config[value_name].get<T>();
            return true;
        }
};


class Address_Map_Loader {
    public:
        /*
        Load registers based on the json config generated by the HDL code
        */
        Address_Map_Loader();
        ~Address_Map_Loader();

        void setup(json* node_config, fpga_mem* base_mem, uint8_t node_index, std::vector<uint64_t>* instructions);   // setup the loader
        
        template <typename T = uint32_t>
        Register* get_register(std::string register_name, uint16_t index);    // get a full register, types are only for checking compatibility

        Group* get_group(std::string group_name, uint16_t index);    // get a group of registers

        void sync_with_PS(Register* reg);    // sync a register with the PS, only needed for parent registers (not sub-registers), must be called before sub-registers are created


    private:
        std::vector<uint64_t>* instructions = nullptr;

        json* node_config = nullptr;

        fpga_mem* base_mem = nullptr;

        void* software_PS_PL_ptr = nullptr;
        void* software_PL_PS_ptr = nullptr;

        uint32_t software_PS_PL_size = 0;
        uint32_t software_PL_PS_size = 0;

        uint8_t node_index = 0;

        Group* base_group = nullptr;

        //template <typename T>
        //void verify_type_compatibility(register_json_data* config);    // make sure the type is compatible with the type in the json file

        template <typename T>
        bool load_json_value(const json& config, const std::string& value_name, T* dest);    // helper function for loading values from the config file
        
};

template <typename T>
bool Address_Map_Loader::load_json_value(const json& config, const std::string& value_name, T* dest){
    // helper function for loading values from the config file
    if (!config.contains(value_name)) {
        std::cerr << "Error: Constant '" << value_name << "' not found in config." << std::endl;
        return false;
    }

    *dest = config[value_name].get<T>();
    return true;
}