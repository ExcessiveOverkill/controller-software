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
        Register();
        ~Register();

        //template <typename T>
        void configure(register_json_data* data, uint8_t node_index, fpga_mem* mem);    // configure the register based on the data from the json file

        //template <typename T>
        void configure(register_json_data* data, uint8_t node_index);    // configure for register not synced with PS
        
        template <typename T>
        T* get_raw_data_ptr();    // get the raw data pointer

        virtual uint32_t create_copy_instructions(std::vector<uint64_t>*);    // create a copy instruction to copy the register to/from the PS/PL.


        // by default read/write functions throw errors unless they are overidden

        // Read the register
        #define READ_ERROR_DEFAULT "Unsupported conversion or cannot read from a write-only register"
        virtual uint32_t get_value() const{ throw std::runtime_error(READ_ERROR_DEFAULT);};
        // TODO: support other types

        // Write to the register
        #define WRITE_ERROR_DEFAULT "Unsupported conversion or cannot write to a read-only register"
        virtual uint32_t set_value(const uint32_t& other) { throw std::runtime_error(WRITE_ERROR_DEFAULT);};
        // TODO: support other types

        // write a single bit
        virtual bool set_bit(bool value, uint8_t index) { throw std::runtime_error(WRITE_ERROR_DEFAULT);};

        // these should only be used to set different pointers post-configure
        void set_software_data_ptr(uint32_t* software_data_ptr);    // set the pointer to the software data
        void set_hardware_data_ptr(uint16_t hardware_data_ptr);    // set the pointer to the hardware data

        // if raw access is needed, these will give the raw pointers
        uint32_t* get_software_data_ptr();    // get the software data pointer
        uint16_t get_hardware_data_ptr();    // get the hardware data pointer

        uint16_t get_count();    // get the count of the register defined in the json file

        enum class reg_type{
            NONE,
            SINGLE_READ,
            SINGLE_WRITE,
            GROUP
        };

        reg_type get_type();    // get the type of the register

        
    protected:
        uint8_t width = 0;
        uint8_t bit_offset = 0;
        uint32_t bit_mask = 0;  // mask is not shifted to the correct position, it is just the correct width
        uint16_t count = 1;

        uint8_t node_index; // index of the node in the FPGA dma chain
        uint16_t node_memory_address;  // address of the data in the node in the FPGA

        fpga_mem* base_mem = nullptr;


        // these are only set if the data is being synced with the PS
        // data here can be any type, but we use uint32 for simplicity since it will always be in 32 bit increments
        
        // data in the PS memory
        uint32_t* software_data_ptr = nullptr;
        
        // data in the PL node 0 memory
        uint16_t hardware_data_ptr = 0;

        reg_type type = reg_type::NONE;



};


class Single_Register_Read : public Register {
public:
    Single_Register_Read() {
        type = reg_type::SINGLE_READ;
    }

    uint32_t get_value() const override{
        return (*software_data_ptr >> bit_offset) & bit_mask;
    }

    uint32_t create_copy_instructions(std::vector<uint64_t>* instructions) override;

private:
};

class Single_Register_Write : public Register{
    public:
    Single_Register_Write() {
        type = reg_type::SINGLE_WRITE;
    }

    uint32_t set_value(const uint32_t& other) override{
        *software_data_ptr &= ~(bit_mask << bit_offset);  // clear the relavent bits
        *software_data_ptr |= (other & bit_mask) << bit_offset;  // set the new bits
        return other;
    }

    bool set_bit(bool value, uint8_t index) override{
        *software_data_ptr &= ~((bit_mask & (1<<index)) << bit_offset);
        *software_data_ptr |= (value << (bit_offset + index));
        return value;
    }

    uint32_t create_copy_instructions(std::vector<uint64_t>* instructions) override;

private:
};

class Register_Group {
    // basically just a wrapper around a vector of single registers
    public:
        //Register_Group();

        const Register_Group& operator[](int index) const; // Immutable access
        Register_Group& operator[](int index);            // Mutable access

        std::vector<Register> registers;

    private:
        uint16_t group_size;
};


class Address_Map_Loader {
    private:
        struct packed_register{
            std::string name;
            uint8_t bit_offset;
            uint8_t width;
        };
    
    
    public:
        /*
        Load registers based on the json config generated by the HDL code
        */
        Address_Map_Loader();
        ~Address_Map_Loader();

        void setup(json* node_config, fpga_mem* base_mem, uint8_t node_index, std::vector<uint64_t>* instructions);   // setup the loader

        // void load_address_map(json* node_config);   // load the address map for a specific node
        // void set_software_side_PS_PL_ptr(void* software_PS_PL_ptr);    // set the pointer to the PS to PL memory
        // void set_software_side_PL_PS_ptr(void* software_PL_PS_ptr);    // set the pointer to the PL to PS memory
        // void set_node_index(uint8_t index);    // set the node index

        //void prepare_registers_for_use(std::vector<uint64_t>* instructions);    // call this to configure the loader to setup the registers for immediate use in the driver

        template <typename T>
        Register* get_register(std::string register_name, uint16_t index);    // get a full register, types are only for checking compatibility
        
        Register* get_packed_register_parent(std::string parent_register_name);    // get a packed register parent
        
        template <typename T>
        Register* get_sub_register_from_parent(Register* parent, std::string parent_register_name, std::string register_name);    // get a value from a packed register, types are only for checking compatibility

        // Register_Group get_registers_from_group<T>(std::string register_name, std::string group_name);    // get a register from a group
        // Register_Group get_packed_registers_from_group<T>(std::string parent_register_name, std::string register_name, std::string group_name);    // get a value from a packed register


    private:

        bool prepare_for_use = false;
        std::vector<uint64_t>* instructions = nullptr;

        json* node_config = nullptr;

        fpga_mem* base_mem = nullptr;

        void* software_PS_PL_ptr = nullptr;
        void* software_PL_PS_ptr = nullptr;

        uint32_t software_PS_PL_size = 0;
        uint32_t software_PL_PS_size = 0;

        uint8_t node_index = 0;

        template <typename T>
        void verify_type_compatibility(register_json_data* config);    // make sure the type is compatible with the type in the json file

        uint32_t get_register_data_by_name(register_json_data* data, const std::string register_name, const std::string group_name);    // get the data for a register

        uint32_t get_packed_register_data_by_name(register_json_data* data, const std::string sub_register_name, const std::string register_name, const std::string group_name);    // get the data for a register

        template <typename T>
        bool load_json_value(const json& config, const std::string& value_name, T* dest);    // helper function for loading values from the config file
        
};



// register groups should be indexed like arrays

// single registers should be created at init like this: reg = some_class.get_register<uint32_t>("register_name")
// packed registers should be created at init like this: reg = some_class.get_packed_register<bool>("parent_register_name", "register_name")
// then can be accessed like this: var = reg, reg = var

// grouped and registers with multiple instances should be created the same way as individual and packed registers
// but are instead accessed like this: var = reg[0], reg[0] = var

// instruction addresses can be obtained like this: reg.get_instruction_address(&node, &node_memory_address), this will set the node and address to the correct values
// note that get_instruction_address will NOT work for packed registers, as instructions always move the entire 32bit register






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