#include <iostream>
#include <string>
#include "register_helper.h"


Register::Register(){
}

Register::~Register(){
}

Address_Map_Loader::Address_Map_Loader(){
}

Address_Map_Loader::~Address_Map_Loader(){
}

void Address_Map_Loader::setup(json* node_config, fpga_mem* base_mem, uint8_t node_index, std::vector<uint64_t>* instructions){
    this->node_config = node_config;
    this->base_mem = base_mem;
    this->node_index = node_index;
    this->instructions = instructions;
    if(instructions != nullptr){
        prepare_for_use = true;
    }
}

void Register::set_software_data_ptr(uint32_t* software_data_ptr){
    this->software_data_ptr = software_data_ptr;
}

void Register::set_hardware_data_ptr(uint16_t hardware_data_ptr){
    this->hardware_data_ptr = hardware_data_ptr;
}

uint32_t* Register::get_software_data_ptr(){
    return software_data_ptr;
}

uint16_t Register::get_hardware_data_ptr(){
    return hardware_data_ptr;
}

Register::reg_type Register::get_type(){
    return type;
}

uint16_t Register::get_count(){
    return count;
}

//template <typename T>
void Register::configure(register_json_data* data, uint8_t node_index, fpga_mem* mem){

    bool read = 0;
    bool write = 0;

    // configure base data pointer
    if(data->read){
        read = true;
        software_data_ptr = reinterpret_cast<uint32_t*>(mem->software_PL_PS_ptr) + mem->software_PL_PS_size / 4; // /4 to convert to 32 bit words
        hardware_data_ptr = mem->hardware_PL_PS_mem_offset + mem->hardware_PL_PS_size;
        mem->software_PL_PS_size += 4;  // increment by 4 bytes
        mem->hardware_PL_PS_size += 1;  // increment by 1 32 bit word
    }
    if(data->write){
        write = true;
        software_data_ptr = reinterpret_cast<uint32_t*>(mem->software_PS_PL_ptr) + mem->software_PS_PL_size / 4; // /4 to convert to 32 bit words
        hardware_data_ptr = mem->hardware_PS_PL_mem_offset + mem->hardware_PS_PL_size;
        mem->software_PS_PL_size += 4;  // increment by 4 bytes
        mem->hardware_PS_PL_size += 1;  // increment by 1 32 bit word
    }
    if(read && write){
        throw std::runtime_error("Read and write registers are not currently supported");
    }
    if(!read && !write){
        throw std::runtime_error("Register must be read or write");
    }

    configure(data, node_index);

}

//template <typename T>
void Register::configure(register_json_data* data, uint8_t node_index){

    // not syncing with the PS, configure only what is needed to create future instructions

    width = data->width;
    bit_offset = data->starting_bit;
    bit_mask = (1 << width) - 1;    // mask is not shifted to the correct position, it is just the correct width
    this->node_index = node_index;
    node_memory_address = data->node_memory_address;
    count = data->count;

}

template <typename T>
T* Register::get_raw_data_ptr(){
    return (T*)software_data_ptr;
}
template uint32_t* Register::get_raw_data_ptr<uint32_t>();
template uint16_t* Register::get_raw_data_ptr<uint16_t>();
template uint8_t* Register::get_raw_data_ptr<uint8_t>();
template int32_t* Register::get_raw_data_ptr<int32_t>();
template int16_t* Register::get_raw_data_ptr<int16_t>();
template int8_t* Register::get_raw_data_ptr<int8_t>();
template bool* Register::get_raw_data_ptr<bool>();

uint32_t Register::create_copy_instructions(std::vector<uint64_t>* instructions){
    // create a copy instruction to copy the register to/from the PS/PL.

    std::cout << "No create_copy_instruction function override defined, no instruction will be created" << std::endl;

    return 1;
}

template <typename T>
Register* Address_Map_Loader::get_register(std::string register_name, uint16_t index){
    // get a full register

    // allowed return types are automatically found by the bit width of the register and the type of the variable defined in the json file
    // specifying a larger matching type is acceptable, but specifying a smaller type will throw an error (uint8 -> uint16 is fine, uint16 -> uint8 is not)
    
    register_json_data data;
    
    get_register_data_by_name(&data, register_name, "");
    
    verify_type_compatibility<T>(&data);

    if(index >= data.count){
        throw std::runtime_error("Index out of range");
    }

    Register* reg;

    if(data.read){
        reg = new Single_Register_Read();
    }
    else{
        reg = new Single_Register_Write();
    }

    data.node_memory_address += index;  // increment the memory address by the index

    if(prepare_for_use){
        reg->configure(&data, node_index, base_mem);  // link to PS memory
        reg->create_copy_instructions(instructions);
    }
    else{
        reg->configure(&data, node_index);  // don't link to PS memory
    }
    
    return reg;
}
template Register* Address_Map_Loader::get_register<uint8_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<uint16_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<uint32_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<int8_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<int16_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<int32_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<bool>(std::string register_name, uint16_t index);


Register* Address_Map_Loader::get_packed_register_parent(std::string parent_register_name){
    // get a packed register parent

    register_json_data data;
    
    get_register_data_by_name(&data, parent_register_name, "");
    
    if(data.format != variable_format::PACKED){
        throw std::runtime_error("Register is not a packed register");
    }

    Register* reg;

    if(data.read){
        reg = new Single_Register_Read();
    }
    else{
        reg = new Single_Register_Write();
    }

    if(prepare_for_use){
        reg->configure(&data, node_index, base_mem);  // link to PS memory
        reg->create_copy_instructions(instructions);
    }
    else{
        reg->configure(&data, node_index);  // don't link to PS memory
    }
    
    return reg;
}

template <typename T>
Register* Address_Map_Loader::get_sub_register_from_parent(Register* parent, std::string parent_register_name, std::string register_name){
    // get part of a packed register
    // sub registers inherit the physical memory addresses of the parent register

    // return type is automatically determined by the bit width of the register and the type of the variable defined in the json file
    // specifying a larger matching type is acceptable, but specifying a smaller type will throw an error (uint8 -> uint16 is fine, uint16 -> uint8 is not)
    
    register_json_data data;
    
    get_packed_register_data_by_name(&data, register_name, parent_register_name, "");
    
    verify_type_compatibility<T>(&data);

    Register* reg;

    // TODO: check to make sure read/write is compatible with parent register, as of now the sub-register read/write is ignored

    switch (parent->get_type()){
    case Register::reg_type::SINGLE_READ:
        reg = new Single_Register_Read();
        break;
    case Register::reg_type::SINGLE_WRITE:
        reg = new Single_Register_Write();
        break;
    default:
        throw std::runtime_error("Sub-register must be a single read or write register");
        break;
    }

    reg->configure(&data, node_index);  // no need to automatically link to PS memory, as the parent register already has (if required)

    reg->set_software_data_ptr(parent->get_software_data_ptr());
    reg->set_hardware_data_ptr(parent->get_hardware_data_ptr());

    return reg;
}
template Register* Address_Map_Loader::get_sub_register_from_parent<uint8_t>(Register* parent, std::string parent_register_name, std::string register_name);
template Register* Address_Map_Loader::get_sub_register_from_parent<uint16_t>(Register* parent, std::string parent_register_name, std::string register_name);
template Register* Address_Map_Loader::get_sub_register_from_parent<uint32_t>(Register* parent, std::string parent_register_name, std::string register_name);
template Register* Address_Map_Loader::get_sub_register_from_parent<int8_t>(Register* parent, std::string parent_register_name, std::string register_name);
template Register* Address_Map_Loader::get_sub_register_from_parent<int16_t>(Register* parent, std::string parent_register_name, std::string register_name);
template Register* Address_Map_Loader::get_sub_register_from_parent<int32_t>(Register* parent, std::string parent_register_name, std::string register_name);
template Register* Address_Map_Loader::get_sub_register_from_parent<bool>(Register* parent, std::string parent_register_name, std::string register_name);


template <typename T>
void Address_Map_Loader::verify_type_compatibility(register_json_data* config){
    // make sure the type is compatible with the type in the json file

    variable_type type;

    if(config->width == 0){
        throw std::runtime_error("Width of register is zero");
    }
    else if(config->format == variable_format::BOOL){
        type = variable_type::BOOL;
    }
    else if(config->format == variable_format::PACKED){
        throw std::runtime_error("Packed registers cannot be directly accessed");
    }
    else if(config->width <= 8){
        type = config->format==variable_format::SIGNED ? variable_type::INT8 : variable_type::UINT8;
    }
    else if(config->width <= 16){
        type = config->format==variable_format::SIGNED ? variable_type::INT16 : variable_type::UINT16;
    }
    else if(config->width <= 32){
        type = config->format==variable_format::SIGNED ? variable_type::INT32 : variable_type::UINT32;
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
        throw std::runtime_error("Unknown type for conversion");
    }
}

uint32_t Address_Map_Loader::get_register_data_by_name(register_json_data* data, const std::string register_name, const std::string group_name){
    // get the data for a register

    std::string group_name_get = "";
    std::string register_name_get = "";
    bool ret = true;

    // loop through all registers to find a match
    for(auto& reg : (*node_config)["node"]["address_map"]){
        
        if(!(load_json_value(reg, "name", &register_name_get) && register_name_get == register_name)){  // name mismatch
            continue;
        }

        if(!(load_json_value(reg, "group_name", &group_name_get) && group_name_get == group_name)){ // group mismatch
            continue;
        }

        ret &= load_json_value(reg, "address", &data->node_memory_address);
        ret &= load_json_value(reg, "count", &data->count);
        ret &= load_json_value(reg, "width", &data->width);
        ret &= load_json_value(reg, "group_index", &data->group_index);
        ret &= load_json_value(reg, "startingBit", &data->starting_bit);

        std::string format;
        ret &= load_json_value(reg, "dataType", &format);
        if(format.find("BOOL") != std::string::npos){
            data->format = variable_format::BOOL;
        }
        else if(format.find("PACKED") != std::string::npos){
            data->format = variable_format::PACKED;
        }
        else if(format.find("UNSIGNED") != std::string::npos){  // note that this check must be before the SIGNED check, since "UNSIGNED" contains "SIGNED"
            data->format = variable_format::UNSIGNED;
        }
        else if(format.find("SIGNED") != std::string::npos){
            data->format = variable_format::SIGNED;
        }
        else{
            ret = false;
        }
        
        std::string read_write;
        ret &= load_json_value(reg, "readWrite", &read_write);
        if(read_write.find("READ") != std::string::npos){
            data->read = true;
        }
        else if(read_write.find("WRITE") != std::string::npos){
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

        std::cout << "Configuring register: " << register_name << " at node memory: " << data->node_memory_address << std::endl;
        return 0;
    }

    std::cerr << "Error: Register not found" << std::endl;
    return 1;

}

uint32_t Address_Map_Loader::get_packed_register_data_by_name(register_json_data* data, const std::string sub_register_name, const std::string register_name, const std::string group_name){
    // get the data for a register

    std::string group_name_get = "";
    std::string register_name_get = "";
    bool ret = true;

    json packed_registers;
    bool parent_register_found = false;

    // loop through all registers to find a match
    for(auto& reg : (*node_config)["node"]["address_map"]){
        
        if(!(load_json_value(reg, "name", &register_name_get) && register_name_get == register_name)){  // name mismatch
            continue;
        }

        if(!(load_json_value(reg, "group_name", &group_name_get) && group_name_get == group_name)){ // group mismatch
            continue;
        }

        if(!(load_json_value(reg, "dataType", &register_name_get) && register_name_get.find("PACKED") != std::string::npos)){ // not a packed register
            std::cerr << "Error: Specified register is not a packed type" << std::endl;
            return 1;
        }

        // parent register found, now find the sub register
        packed_registers = reg["packedRegisters"];
        parent_register_found = true;
        break;
    }

    if(!parent_register_found){
        std::cerr << "Error: Parent register not found" << std::endl;
        return 1;
    }

    // loop through all packed registers to find a match
    for(auto& reg : packed_registers){
        if(!(load_json_value(reg, "name", &register_name_get) && register_name_get == sub_register_name)){  // name mismatch
            continue;
        }

        ret &= load_json_value(reg, "address", &data->node_memory_address);
        ret &= load_json_value(reg, "count", &data->count);
        ret &= load_json_value(reg, "width", &data->width);
        ret &= load_json_value(reg, "group_index", &data->group_index);
        ret &= load_json_value(reg, "startingBit", &data->starting_bit);

        std::string format;
        ret &= load_json_value(reg, "dataType", &format);
        if(format.find("BOOL") != std::string::npos){
            data->format = variable_format::BOOL;
        }
        else if(format.find("PACKED") != std::string::npos){
            //data->format = variable_format::PACKED;
            std::cerr << "Error: Nested packed registers not supported" << std::endl;
            return 1;
        }
        // note that we must check for unsigned before signed since "unsigned" contains "signed"
        else if(format.find("UNSIGNED") != std::string::npos){
            data->format = variable_format::UNSIGNED;
        }
        else if(format.find("SIGNED") != std::string::npos){
            data->format = variable_format::SIGNED;
        }
        else{
            ret = false;
        }
        
        std::string read_write;
        ret &= load_json_value(reg, "readWrite", &read_write);
        if(read_write.find("READ") != std::string::npos){
            data->read = true;
        }
        else if(read_write.find("WRITE") != std::string::npos){
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

        std::cout << "Configured sub-register: " << sub_register_name << " at offset: " << data->node_memory_address << ":" << data->starting_bit << std::endl;
        return 0;
    }

    std::cerr << "Error: Sub-register not found" << std::endl;
    return 1;

}


//template <typename T>
uint32_t Single_Register_Read::create_copy_instructions(std::vector<uint64_t>* instructions){
    instructions->push_back(create_instruction_COPY(node_index, node_memory_address, 0, hardware_data_ptr));
    return 0;
}

//template <typename T>
// T Single_Register_Write<T>::operator=(const T& other){
//     // Write to the register
//     *software_data_ptr &= ~(bit_mask << bit_offset);  // clear the relavent bits
//     *software_data_ptr |= (other & bit_mask) << bit_offset;  // set the new bits
//     return other;
// }


//template <typename T>
uint32_t Single_Register_Write::create_copy_instructions(std::vector<uint64_t>* instructions){
    instructions->push_back(create_instruction_COPY(0, hardware_data_ptr, node_index, node_memory_address));
    return 0;
}
