
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <json.hpp>

#include "fpga_instructions.h"

using json = nlohmann::json;

// base driver for which all other module drivers will inherit from
class base_driver {
public:
    //virtual ~base_driver();

    // void set_data_memory_size(uint32_t size){   // needed to propperly offset instructions
    //     DATA_MEMORY_SIZE = size;
    // }

    const uint32_t* microseconds = nullptr;    // pointer to the microseconds counter in the fpga manager, used for timing

    virtual uint32_t load_config_pre(json config) = 0;  // determines the size of the memory that the driver will require
    virtual uint32_t load_config_post(json config) = 0; // configures the internal memory pointers
    virtual uint32_t get_base_instructions(std::vector<uint64_t>* instructions) = 0; // gets the base instructions for the driver, these will be run first
    virtual uint32_t run() = 0; // run the driver, called after each FPGA update

    struct fpga_mem{    // memory allocated to the driver by the FPGA manager
        uint32_t PS_PL_size = 0;    // must be in 4 byte increments
        void* PS_PL_ptr = nullptr;  // pointer in user space
        uint32_t PS_PL_mem_offset = 0;  // offset in the PS->PL memory, used for instructions

        uint32_t PL_PS_size = 0;    // must be in 4 byte increments
        void* PL_PS_ptr = nullptr;  // pointer in user space
        uint32_t PL_PS_mem_offset = 0;  // offset in the PS->PL memory, used for instructions
    };

    fpga_mem base_mem;  // this is what is required for MINIMUM functionality of a driver
    //fpga_mem extra_mem; // this is for any additional memory that the driver may require based on further configuration from node configs

protected:

    // helper function for loading values from the config file
    // WARNING: this may cause issues if you try to load a value as a different type than it is stored as
    template <typename T>
    bool load_json_value(const json& config, const std::string& value_name, T* dest); 

    uint8_t node_address = 255;   // address of the node in the FPGA
    //uint32_t DATA_MEMORY_SIZE = 0;  // size of the read/write (each) data memory in the FPGA in 32 bit words

};


template <typename T>
bool base_driver::load_json_value(const json& config, const std::string& value_name, T* dest){
    // helper function for loading values from the config file
    if (!config.contains(value_name)) {
        std::cerr << "Error: Constant '" << value_name << "' not found in config." << std::endl;
        return false;
    }

    *dest = config[value_name].get<T>();
    return true;
}






// Factory class for creating drivers
class Driver_Factory {
public:
    using Creator = std::function<std::shared_ptr<base_driver>()>;

    // Registers a creator function with a string key
    static void registerType(const std::string& typeName, Creator creator) {
        auto& map = getMap();
        if (map.find(typeName) != map.end()) {
            throw std::runtime_error("Type already registered: " + typeName);
        }
        map[typeName] = creator;
    }

    // Creates an object based on the string key
    static std::shared_ptr<base_driver> create_shared(const std::string& typeName) {
        auto& map = getMap();
        auto it = map.find(typeName);
        if (it != map.end()) {
            return (it->second)();
        }
        throw std::runtime_error("Type not registered: " + typeName);
    }

private:
    // Returns the static map
    static std::unordered_map<std::string, Creator>& getMap() {
        static std::unordered_map<std::string, Creator> map;
        return map;
    }
};

// Template class to register a type with the factory
template <typename T>
class Driver_Registrar {
public:
    Driver_Registrar(const std::string& typeName) {
        Driver_Factory::registerType(typeName, []() -> std::shared_ptr<base_driver> {
            return std::make_shared<T>();
        });
    }
};
