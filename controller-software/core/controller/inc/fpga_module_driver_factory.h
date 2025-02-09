
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <json.hpp>

#include "fpga_instructions.h"
#include "node_core.h"

using json = nlohmann::json;

// base driver for which all other module drivers will inherit from
class base_driver {
public:
    const uint64_t* microseconds = nullptr;    // pointer to the microseconds counter in the fpga manager, used for timing

    virtual uint32_t load_config(json* config, std::string module_name, Node_Core* node_core, fpga_instructions* fpga_instr) = 0; // configures the internal memory pointers and user nodes
    
    virtual uint32_t custom_load_config() = 0; // custom configuration for the driver

    virtual uint32_t run() = 0; // run the driver, called after each FPGA update

    fpga_mem base_mem;  // this is what is required for MINIMUM functionality of a driver


    // TESTING ONLY //
    int32_t* cmd_q_current_milliamps = nullptr;    // for testing only, this should be removed
    uint32_t* encoder_pos = nullptr;    // for testing only, this should be removed
    uint32_t* encoder_multiturn_count = nullptr;    // for testing only, this should be removed

protected:

    // helper function for loading values from the config file
    // WARNING: this may cause issues if you try to load a value as a different type than it is stored as
    template <typename T>
    bool load_json_value(const json& config, const std::string& value_name, T* dest); 

    uint8_t node_address = 255;   // address of the node in the FPGA

    Node_Core* node_core = nullptr; // pointer to the node core for the FPGA
    fpga_instructions* fpga_instr = nullptr; // pointer to the fpga instructions for the FPGA
    json* config = nullptr; // pointer to the json config for the driver
    Address_Map_Loader loader;  // loader for the address map
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
        
        std::cout << "Creating driver: " << typeName << std::endl;
    }
};
