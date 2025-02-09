#include "json.hpp"
#include <string>

#pragma once

using json = nlohmann::json;

/*
Helper class to load json address file

Goal is to make functions/objects that pair with the functions/objects used in the HDL code to initially create the address map
*/

struct fpga_mem{    // memory allocated to the driver by the FPGA manager
    uint32_t software_PS_PL_size = 0;    // must be in 4 byte increments
    uint32_t hardware_PS_PL_size = 0; // in 32 bit words
    void* software_PS_PL_ptr = nullptr;  // pointer in user space
    uint32_t hardware_PS_PL_mem_offset = 0;  // offset in the PS->PL memory, used for instructions

    uint32_t software_PL_PS_size = 0;    // must be in 4 byte increments
    uint32_t hardware_PL_PS_size = 0; // in 32 bit words
    void* software_PL_PS_ptr = nullptr;  // pointer in user space
    uint32_t hardware_PL_PS_mem_offset = 0;  // offset in the PS->PL memory, used for instructions
};

class Register {
    public:
        Register(json* json_data, const std::string register_name, uint16_t index, uint16_t parent_absolute_address, std::string prefix_name);
        ~Register();

        template <typename T = uint32_t>
        Register* get_register(std::string register_name);  // get a sub-register, no index needed
        
        template <typename T>
        T* get_raw_data_ptr();    // get the raw data pointer

        uint32_t create_copy_instructions(std::vector<uint64_t>*);    // create a copy instruction to copy the register to/from the PS/PL.

        //uint32_t create_copy_instruction(fpga_instructions::copy** cpy);    // create a copy instruction to copy the register to/from the PS/PL.

        // Read the register
        template <typename T = uint32_t>
        T get_value() const;

        // Write to the register
        template <typename T = uint32_t>
        T set_value(const T& other);

        // write a single bit
        bool set_bit(bool value, uint8_t index);

        enum class variable_format : uint8_t {
            BOOL,
            SIGNED,
            UNSIGNED,
            //FLOAT,    // not implemented yet
            PACKED,
        };

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
            variable_format var_format = variable_format::UNSIGNED;
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

        std::string full_name;

        
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

        Group(json* json_data, const std::string group_name, uint16_t index, uint16_t parent_absolute_address, std::string prefix_name);

        template <typename T = uint32_t>
        Register* get_register(std::string register_name, uint16_t index);    // get a register in the group

        Group* get_group(std::string group_name, uint16_t index);    // get a group in the group
            
        data group_data;

        std::string full_name;

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

        void setup(std::string name, json* config, fpga_mem* base_mem);   // setup the loader

        template <typename T = uint32_t>
        Register* get_register(std::string register_name, uint16_t index);    // get a full register, types are only for checking compatibility

        Group* get_group(std::string group_name, uint16_t index);    // get a group of registers

        void sync_with_PS(Register* reg);    // sync a register with the PS, only needed for parent registers (not sub-registers), must be called before sub-registers are created

        Register* get_register_by_full_name(std::string full_name);    // get a register by the full name

        uint8_t get_node_index();    // get the node index

        std::vector<uint64_t>* instructions = nullptr;

    private:

        json* config = nullptr;

        fpga_mem* base_mem = nullptr;

        void* software_PS_PL_ptr = nullptr;
        void* software_PL_PS_ptr = nullptr;

        uint32_t software_PS_PL_size = 0;
        uint32_t software_PL_PS_size = 0;

        uint8_t node_index = 255;

        Group* base_group = nullptr;

        //fpga_instructions* fpga_instr = nullptr;

        template <typename T>
        bool load_json_value(const json& config, const std::string& value_name, T* dest);    // helper function for loading values from the config file
        
        std::string module_name = "";
        uint8_t module_index = 0;   // index of the module if there are multiple instances of the same module in the fpga
};

class Dynamic_Register{
    // class to allow changing the target PL location on the fly
    // uses only one address in the PS and PL memory, but adjusts the DMA instruction to change what node/BRAM address is being used

    public:
        Dynamic_Register(Address_Map_Loader* loader, Register* reg);

        void set_register(Register* reg);    // set the register to be used, NOTE: an update cycle must be run for the data to update
        Register* get_register();    // get the register being used (will be the one used to initialize the dynamic register, but with the instruction modified)
    
        void enable_sync(bool enable);    // enable/disable syncing with the PS

    private:
        std::vector<uint64_t>* instructions = nullptr;
        uint32_t instruction_index = 0;
        Register* reg = nullptr;
        bool read = false;
        bool write = false;
        uint16_t ps_hardware_data_ptr = 0;
        uint64_t instruction = 0;

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