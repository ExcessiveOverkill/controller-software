#include <iostream>
#include <string>
#include "register_helper.h"


Register::Register(json* json_data, const std::string register_name, uint16_t register_index, uint16_t parent_absolute_address, std::string prefix_name){
    // this json object must contain the register object
    if (!json_data->contains(register_name)){
        throw std::runtime_error("Register not found");
    }

    this->json_data = new json((*json_data)[register_name]);

    get_register_data(register_name);
    pl_data.index = register_index;
    pl_data.absolute_address = parent_absolute_address + pl_data.address_offset + register_index;

    pl_data.bit_mask = (1 << pl_data.width) - 1;    // mask is not shifted to the correct position, it is just the correct width

    full_name = prefix_name + "." + pl_data.name + ":" + std::to_string(pl_data.index);
}

Register::~Register(){
}

Address_Map_Loader::Address_Map_Loader(){
}

Address_Map_Loader::~Address_Map_Loader(){
}

void Address_Map_Loader::setup(std::string name, json* config, fpga_mem* base_mem){
    this->module_name = name;
    this->config = config;
    this->base_mem = base_mem;

    node_index = (*config)[module_name]["node_address"].get<uint8_t>();

    json data = (*config)[module_name]["node"];

    base_group = new Group(&data, "base_group", 1, 0, "");
    base_group->full_name = module_name + ":" + std::to_string(module_index);    // override the base group name to just be the module name
}

Register* Address_Map_Loader::get_register_by_full_name(std::string full_name){
    // get a register by the full name
    
    // TODO: implement
    
    return nullptr;
}

uint8_t Address_Map_Loader::get_node_index(){
    // get the node index
    return node_index;
}

Dynamic_Register::Dynamic_Register(Address_Map_Loader* loader, Register* reg){
    this->instructions = loader->instructions;
    loader->sync_with_PS(reg);
    this->reg = reg;
    read = reg->pl_data.read;
    write = reg->pl_data.write;
    instruction_index = instructions->size()-1;   // save the current instruction index so we know which to modify
    instruction = instructions->at(instruction_index);
    enable_sync(false); // disable syncing by default
}


template <typename T>
T* Register::get_raw_data_ptr(){
    return (T*)ps_data.software_data_ptr;
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

    if(pl_data.read){
        //instructions->push_back(create_instruction_COPY(pl_data.node_index, pl_data.absolute_address, 0, ps_data.hardware_data_ptr));
    }
    else if(pl_data.write){
        //instructions->push_back(create_instruction_COPY(0, ps_data.hardware_data_ptr, pl_data.node_index, pl_data.absolute_address));
    }
    else{
        throw std::runtime_error("Register must be read or write");
    }

    return 0;
}

// uint32_t Register::create_copy_instruction(fpga_instructions::copy** cpy){
//     // create a copy instruction to copy the register to/from the PS/PL.

//     if(pl_data.read){
//         *cpy = new fpga_instructions::copy(pl_data.node_index, pl_data.absolute_address, 0, ps_data.hardware_data_ptr);
//     }
//     else if(pl_data.write){
//         *cpy = new fpga_instructions::copy(0, ps_data.hardware_data_ptr, pl_data.node_index, pl_data.absolute_address);
//     }
//     else{
//         throw std::runtime_error("Register must be read or write");
//     }

//     return 0;
// }

void Address_Map_Loader::sync_with_PS(Register* reg){
    // sync the register with the PS, only needed for parent registers (not sub-registers), must be called before sub-registers are created

    if(reg->is_sub_register){
        throw std::runtime_error("Sync with PS can only be called on parent registers");
    }

    if(reg->pl_data.read){
        reg->ps_data.software_data_ptr = reinterpret_cast<uint32_t*>(base_mem->software_PL_PS_ptr) + base_mem->software_PL_PS_size / 4; // /4 to convert to 32 bit words
        reg->ps_data.hardware_data_ptr = base_mem->hardware_PL_PS_mem_offset + base_mem->hardware_PL_PS_size;
        base_mem->software_PL_PS_size += 4;  // increment by 4 bytes
        base_mem->hardware_PL_PS_size += 1;  // increment by 1 32 bit word
    }
    else if(reg->pl_data.write){
        reg->ps_data.software_data_ptr = reinterpret_cast<uint32_t*>(base_mem->software_PS_PL_ptr) + base_mem->software_PS_PL_size / 4; // /4 to convert to 32 bit words
        reg->ps_data.hardware_data_ptr = base_mem->hardware_PS_PL_mem_offset + base_mem->hardware_PS_PL_size;
        base_mem->software_PS_PL_size += 4;  // increment by 4 bytes
        base_mem->hardware_PS_PL_size += 1;  // increment by 1 32 bit word
    }
    else{
        throw std::runtime_error("Register must be read or write");
    }

    reg->pl_data.node_index = node_index;
    reg->create_copy_instructions(instructions);
    
    return;
}

// fpga_instructions::copy* Address_Map_Loader::sync_with_PS_new(Register* reg){
//     // sync the register with the PS, only needed for parent registers (not sub-registers), must be called before sub-registers are created

//     if(reg->is_sub_register){
//         throw std::runtime_error("Sync with PS can only be called on parent registers");
//     }

//     if(reg->pl_data.read){
//         reg->ps_data.software_data_ptr = reinterpret_cast<uint32_t*>(base_mem->software_PL_PS_ptr) + base_mem->software_PL_PS_size / 4; // /4 to convert to 32 bit words
//         reg->ps_data.hardware_data_ptr = base_mem->hardware_PL_PS_mem_offset + base_mem->hardware_PL_PS_size;
//         base_mem->software_PL_PS_size += 4;  // increment by 4 bytes
//         base_mem->hardware_PL_PS_size += 1;  // increment by 1 32 bit word
//     }
//     else if(reg->pl_data.write){
//         reg->ps_data.software_data_ptr = reinterpret_cast<uint32_t*>(base_mem->software_PS_PL_ptr) + base_mem->software_PS_PL_size / 4; // /4 to convert to 32 bit words
//         reg->ps_data.hardware_data_ptr = base_mem->hardware_PS_PL_mem_offset + base_mem->hardware_PS_PL_size;
//         base_mem->software_PS_PL_size += 4;  // increment by 4 bytes
//         base_mem->hardware_PS_PL_size += 1;  // increment by 1 32 bit word
//     }
//     else{
//         throw std::runtime_error("Register must be read or write");
//     }

//     reg->pl_data.node_index = node_index;

//     fpga_instructions::copy* cpy = nullptr;
//     reg->create_copy_instruction(&cpy);
//     fpga_instr->add(cpy);
    
//     return cpy;
// }

void Register::get_register_data(const std::string register_name){
    // get the data for a group

    std::string register_name_get = "";
    bool ret = true;
        
    pl_data.name = register_name;
    ret &= load_json_value(*json_data, "address_offset", &pl_data.address_offset);
    ret &= load_json_value(*json_data, "bank_size", &pl_data.bank_size);
    ret &= load_json_value(*json_data, "width", &pl_data.width);
    ret &= load_json_value(*json_data, "starting_bit", &pl_data.starting_bit);
    
    // std::string data_type;
    // ret &= load_json_value(*json_data, "type", &data_type);
    // if(data_type == "bool"){
    //     pl_data.var_format = variable_format::BOOL;
    // }
    // else if(data_type == "signed"){
    //     pl_data.var_format = variable_format::SIGNED;
    // }
    // else if(data_type == "unsigned"){
    //     pl_data.var_format = variable_format::UNSIGNED;
    // }
    // else{
    //     throw std::runtime_error("Invalid data type");
    // }

    std::string rw;
    ret &= load_json_value(*json_data, "rw", &rw);
    if(rw == "r"){
        pl_data.read = true;
    }
    else if(rw == "w"){
        pl_data.write = true;
    }
    else{
        throw std::runtime_error("Invalid read/write value");
    }

    if(!ret){
        throw std::runtime_error("Failed to load register data");
    }

    return;
}

Group::Group(json* json_data, const std::string group_name, uint16_t index, uint16_t parent_absolute_address, std::string prefix_name){
    // this json object must contain the group object
    if (!json_data->contains(group_name)){
        throw std::runtime_error("Group not found");
    }
    this->json_data = new json((*json_data)[group_name]);

    get_group_data(group_name);
    group_data.index = index;
    group_data.absolute_address = parent_absolute_address + group_data.address_offset + (group_data.alignment * index);

    full_name = prefix_name + "." + group_name + ":" + std::to_string(group_data.index);
}

void Group::get_group_data(const std::string group_name){
    // get the data for a group

    std::string group_name_get = "";
    bool ret = true;

    group_data.name = group_name;
    ret &= load_json_value(*json_data, "address_offset", &group_data.address_offset);
    ret &= load_json_value(*json_data, "alignment", &group_data.alignment);
    ret &= load_json_value(*json_data, "count", &group_data.count);

    if(!ret){
        throw std::runtime_error("Failed to load group data");
    }
    return;    
}

template <typename T>
Register* Group::get_register(std::string register_name, uint16_t index){
    // get a register in the group
    json data = (*json_data)["registers"];

    return new Register(&data, register_name, index, group_data.absolute_address, full_name);
}

Group* Group::get_group(std::string group_name, uint16_t index){
    // get a group of registers
    json data = (*json_data)["groups"];

    return new Group(&data, group_name, index, group_data.absolute_address, full_name);
}

template <typename T>
Register* Register::get_register(std::string register_name){
    // get a sub-register, no index needed
    json data = (*json_data)["sub_registers"];
    auto reg = new Register(&data, register_name, 0, 0, full_name);
    reg->is_sub_register = true;
    reg->ps_data.software_data_ptr = ps_data.software_data_ptr;
    reg->ps_data.hardware_data_ptr = ps_data.hardware_data_ptr;
    return reg;
}
template Register* Register::get_register<uint32_t>(std::string register_name);
template Register* Register::get_register<uint16_t>(std::string register_name);
template Register* Register::get_register<uint8_t>(std::string register_name);
template Register* Register::get_register<int32_t>(std::string register_name);
template Register* Register::get_register<int16_t>(std::string register_name);
template Register* Register::get_register<int8_t>(std::string register_name);
template Register* Register::get_register<bool>(std::string register_name);

template <typename T>
Register* Address_Map_Loader::get_register(std::string register_name, uint16_t index){
    // get a full register

    // allowed return types are automatically found by the bit width of the register and the type of the variable defined in the json file
    // specifying a larger matching type is acceptable, but specifying a smaller type will throw an error (uint8 -> uint16 is fine, uint16 -> uint8 is not)
    
    auto reg = base_group->get_register<T>(register_name, index);
    return reg;
}
template Register* Address_Map_Loader::get_register<uint8_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<uint16_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<uint32_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<int8_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<int16_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<int32_t>(std::string register_name, uint16_t index);
template Register* Address_Map_Loader::get_register<bool>(std::string register_name, uint16_t index);

// TODO: re-implement this
/*
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
*/

Group* Address_Map_Loader::get_group(std::string group_name, uint16_t index){
    // get a group of registers

    return base_group->get_group(group_name, index);
}

void Dynamic_Register::set_register(Register* reg){
    // set the register to be used, NOTE: an update cycle must be run for the data to update
    if(reg->is_sub_register){
        throw std::runtime_error("Dynamic register cannot be a sub-register");
    }

    if(reg->pl_data.read && write){
        throw std::runtime_error("Dynamic register cannot switch between read and write");
    }
    if(reg->pl_data.write && read){
        throw std::runtime_error("Dynamic register cannot switch between read and write");
    }

    // if(this->reg->pl_data.node_index != reg->pl_data.node_index){    // TODO: fix this, currently regs are assigned a node only on a sync call
    //     throw std::runtime_error("Dynamic register cannot switch between nodes");
    // }

    // modify the instruction to use the new register
    // TODO: might want to have dynamic instructions always be the last/first instructions in the list so we don't need to update multiple instruction blocks (if we have too many instructions)
    if(read){
        //instruction = create_instruction_COPY(this->reg->pl_data.node_index, reg->pl_data.absolute_address, 0, this->reg->ps_data.hardware_data_ptr);
    }
    else{
        //instruction = create_instruction_COPY(0, this->reg->ps_data.hardware_data_ptr, this->reg->pl_data.node_index, reg->pl_data.absolute_address);
    }
    instructions->at(instruction_index) = instruction;
}

Register* Dynamic_Register::get_register(){
    // get the register being used (will be the one used to initialize the dynamic register, but with the instruction modified)
    return reg;
}

void Dynamic_Register::enable_sync(bool enable){
    // enable/disable syncing with the PS
    if(enable){
        instructions->at(instruction_index) = instruction;
    }
    else{
        //instructions->at(instruction_index) = create_instruction_NOP();
    }
}

template <typename T>
T Register::get_value() const{
    // get the value of the register

    return (*(ps_data.software_data_ptr) >> pl_data.starting_bit) & pl_data.bit_mask;
}
template uint32_t Register::get_value<uint32_t>() const;
template uint16_t Register::get_value<uint16_t>() const;
template uint8_t Register::get_value<uint8_t>() const;
template int32_t Register::get_value<int32_t>() const;
template int16_t Register::get_value<int16_t>() const;
template int8_t Register::get_value<int8_t>() const;
template bool Register::get_value<bool>() const;

template <typename T>
T Register::set_value(const T& value){
    // set the value of the register

    *(ps_data.software_data_ptr) &= ~(pl_data.bit_mask << pl_data.starting_bit);  // clear the relavent bits
    *(ps_data.software_data_ptr) |= (value & pl_data.bit_mask) << pl_data.starting_bit;  // set the new bits
    return value;
}
template uint32_t Register::set_value<uint32_t>(const uint32_t& value);
template uint16_t Register::set_value<uint16_t>(const uint16_t& value);
template uint8_t Register::set_value<uint8_t>(const uint8_t& value);
template int32_t Register::set_value<int32_t>(const int32_t& value);
template int16_t Register::set_value<int16_t>(const int16_t& value);
template int8_t Register::set_value<int8_t>(const int8_t& value);
template bool Register::set_value<bool>(const bool& value);

bool Register::set_bit(bool value, uint8_t index){
    // set a single bit in the register

    *(ps_data.software_data_ptr) &= ~((pl_data.bit_mask & (1<<index)) << pl_data.starting_bit);
    *(ps_data.software_data_ptr) |= (value << (pl_data.starting_bit + index));
    return value;
}
