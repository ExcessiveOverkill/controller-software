#include "fpga_module_driver_factory.h"

uint32_t base_driver::load_config(json* config, std::string module_name, Node_Core* node_core, fpga_instructions* fpga_instr, json* user_driver_config){
    this->node_core = node_core;
    this->fpga_instr = fpga_instr;
    this->config = config;

    loader.setup(module_name, config, &base_mem);

    node_address = loader.get_node_index();

    node_var_prefix = "node_vars.drivers." + module_name;

    if(custom_load_config(user_driver_config) != 0){
        std::cerr << "Failed to load custom config" << std::endl;
        return 1;
    }

    return 0;
}

void base_driver::cpy_source(Register* reg){
    cpy.set_source(reg->pl_data.full_name);
    copy_reg = reg;
}

void base_driver::cpy_destination(Register* reg){
    cpy.set_destination(reg->pl_data.full_name);
    copy_reg = reg;
}

void base_driver::cpy_source(std::string var_name){
    std::string full_name = node_var_prefix + "." + var_name;

    if(copy_reg == nullptr){
        throw std::runtime_error("Register src/dst must be set before node variable");
    }
    
    node_core->create_global_variable(full_name, io_type::UINT32);
    
    node_core->get_global_variable_data_ptr(full_name, (void**)&(copy_reg->ps_data.software_data_ptr));

    // add subregister pointers to node variable
    for(auto& var_ptr : copy_reg->sub_register_var_ptrs){
        node_core->get_global_variable_data_ptr(full_name, var_ptr);
    }
    
    cpy.set_source(full_name);
}

void base_driver::add_node_var(std::string var_name, io_type type, void** data_ptr){
    node_core->create_global_variable(node_var_prefix + "." + var_name, type);
    node_core->get_global_variable_data_ptr(node_var_prefix + "." + var_name, data_ptr);
}

void base_driver::cpy_destination(std::string var_name){
    std::string full_name = node_var_prefix + "." + var_name;

    if(copy_reg == nullptr){
        throw std::runtime_error("Register src/dst must be set before node variable");
    }

    node_core->create_global_variable(full_name, io_type::UINT32);
    
    node_core->get_global_variable_data_ptr(full_name, (void**)&(copy_reg->ps_data.software_data_ptr));

    // add subregister pointers to node variable
    for(auto& var_ptr : copy_reg->sub_register_var_ptrs){
        node_core->get_global_variable_data_ptr(full_name, var_ptr);
    }

    cpy.set_destination(full_name);
}

void base_driver::cpy_dynamic_source(std::string var_name, uint32_t** data_ptr){
    std::string full_name = node_var_prefix + "." + var_name;

    node_core->create_global_variable(full_name, io_type::UINT32);
    node_core->get_global_variable_data_ptr(full_name, (void**)data_ptr);
    
    cpy.set_register_index_source(full_name);
}

void base_driver::cpy_add_instruction(){
    fpga_instr->add(cpy);
    cpy.clear_register_index_source();
    copy_reg = nullptr;
}

void base_driver::cpy_time_reference(uint32_t time_reference){
    cpy.set_time_reference(time_reference);
}

void base_driver::cpy_time_reference(fpga_instructions::copy* ref_instruction, bool write_priority){
    cpy.set_time_reference(ref_instruction, write_priority);
}

void base_driver::cpy_execution_window(int32_t earliest, int32_t latest){
    cpy.set_execution_window(earliest, latest);
}

void base_driver::cpy_write_priority(){
    cpy.set_write_priority();
}

void base_driver::cpy_read_priority(){
    cpy.set_read_priority();
}

