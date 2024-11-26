#include "register_helper.h"

Register::Register(){
}

Register::~Register(){
}

Register::operator=(const Register& other){
    // copy the register
}

Register::operator T() const{
    // read the register
}

Address_Map_Loader::Address_Map_Loader(){
}

Address_Map_Loader::~Address_Map_Loader(){
}

void Address_Map_Loader::load_address_map(json* node_config){
    this->node_config = node_config;
}

template <typename T>
void Address_Map_Loader::verify_type_compatibility(register_data* config){
    // make sure the type is compatible with the type in the json file

    // TODO: load this
    uint8_t width;

    // TODO: load this
    variable_format format;


    variable_type type;

    if(width == 0){
        throw std::runtime_error("Width of register is zero");
    }
    else if(format == variable_format::BOOL){
        type = variable_type::BOOL;
    }
    else if(format == variable_format::PACKED){
        throw std::runtime_error("Packed registers cannot be directly accessed");
    }
    else if(width <= 8){
        type = format==variable_format::SIGNED ? variable_type::INT8 : variable_type::UINT8;
    }
    else if(width <= 16){
        type = format==variable_format::SIGNED ? variable_type::INT16 : variable_type::UINT16;
    }
    else if(width <= 32){
        type = format==variable_format::SIGNED ? variable_type::INT32 : variable_type::UINT32;
    }
    else{
        throw std::runtime_error("Width of register is too large (over 32 bits)");
    }


    // make sure whatever type the user specified is the same as (or compatible with) the type in the json file
    if(std::is_same<T, uint8_t>::value){
        switch (type){
        case variable_type::UINT8:
            break;
        default:
            throw std::runtime_error("Type specified is not compatible with the type in the json file");
            break;
        }
    }
    else if(std::is_same<T, uint16_t>::value){
        switch (type){
        case variable_type::UINT8:
        case variable_type::UINT16:
            break;
        default:
            throw std::runtime_error("Type specified is not compatible with the type in the json file");
            break;
        }
    }
    else if(std::is_same<T, uint32_t>::value){
        switch (type){
        case variable_type::UINT8:
        case variable_type::UINT16:
        case variable_type::UINT32:
            break;
        default:
            throw std::runtime_error("Type specified is not compatible with the type in the json file");
            break;
        }
    }
    else if(std::is_same<T, int8_t>::value){
        switch (type){
        case variable_type::INT8:
            break;
        default:
            throw std::runtime_error("Type specified is not compatible with the type in the json file");
            break;
        }
    }
    else if(std::is_same<T, int16_t>::value){
        switch (type){
        case variable_type::INT8:
        case variable_type::INT16:
            break;
        default:
            throw std::runtime_error("Type specified is not compatible with the type in the json file");
            break;
        }
    }
    else if(std::is_same<T, int32_t>::value){
        switch (type){
        case variable_type::INT8:
        case variable_type::INT16:
        case variable_type::INT32:
            break;
        default:
            throw std::runtime_error("Type specified is not compatible with the type in the json file");
            break;
        }
    }
    else if(std::is_same<T, bool>::value){
        switch (type){
        case variable_type::BOOL:
            break;
        default:
            throw std::runtime_error("Type specified is not compatible with the type in the json file");
            break;
        }
    }
    else{
        // unsupported type, throw error
        throw std::runtime_error("Unknown type for convertion");
    }
}

uint32_t Address_Map_Loader::get_register_data_by_name(register_data* data, std::string register_name, std::string group_name){
    // get the data for a register

    std::string group_name_get = "";
    std::string register_name_get = "";
    bool ret = true;

    // loop through all registers to find a match
    for(auto& reg : node_config["address_map"]){
        if(!(load_json_value(reg, "name", &register_name) && register_name_get == register_name)){  // name mismatch
            continue;
        }
        if(!(load_json_value(reg, "group_name", &group_name) && group_name_get == group_name)){ // group mismatch
            continue;
        }

        data->address = reg;
        ret &= load_json_value(reg, "count", &data->count);
        ret &= load_json_value(reg, "width", &data->width);
        ret &= load_json_value(reg, "group_index", &data->group_index);
        ret &= load_json_value(reg, "format", &data->format);
        ret &= load_json_value(reg, "starting_bit", &data->starting_bit);
        str::string read_write;
        ret &= load_json_value(reg, "read", &read_write);
        if(read_write.contains("READ")){
            data->read = true;
        }
        else if(read_write.contains("WRITE")){
            data->write = true;
        }
        else{
            ret = false;
        }
        if(data->read && data->write){
            std::cerr << "Error: Register cannot be both read and write" << std::endl;
            ret = false;
        }
        
        if(!ret){
            std::cerr << "Error: Failed to load register data" << std::endl;
            return 1;
        }
        return 0;
    }

}


Register Address_Map_Loader::get_register<T>(std::string register_name){
    // get a full register

    // return type is automatically determined by the bit width of the register and the type of the variable defined in the json file
    // specifying a larger matching type is acceptable, but specifying a smaller type will throw an error (uint8 -> uint16 is fine, uint16 -> uint8 is not)
    verify_type_compatibility<T>(node_config);

    // TODO: create the register object and configure it
    
}


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