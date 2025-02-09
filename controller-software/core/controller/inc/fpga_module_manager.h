#include <iostream>
#include <vector>
#include <memory>
#include "json.hpp"
#include "fpga_interface.h"
#include "fpga_instructions.h"
#include "node_core.h"


// include all drivers here

#include "serial_interface_card.h"
//#include "global_timers.h"
//#include "fanuc_encoders.h"
//#include "em_serial_controller.h"


#pragma once
using json = nlohmann::json;


class fpga_module_manager {
public:
    fpga_module_manager();
    ~fpga_module_manager();

    uint32_t load_config(std::string config_file);

    uint32_t set_fpga_interface(Fpga_Interface* fpga_interface);

    uint32_t initialize_fpga();

    uint32_t load_drivers();

    uint32_t compile_instructions();

    uint32_t save_ps_nodes(std::string file_path);   // save the fpga nodes to a json file

    uint32_t create_global_variables();

    void set_microseconds(const uint64_t* microseconds);

    uint32_t run_update();

    std::shared_ptr<base_driver> get_driver(uint32_t index);

    Node_Core* node_core = nullptr;

private:

    const uint64_t* microseconds = nullptr;

    json config;
    Fpga_Interface* fpga_interface;
    fpga_mem_layout mem_layout;

    void* PS_PL_control_ptr = nullptr;
    void* PL_PS_control_ptr = nullptr;
    void* PS_PL_data_ptr = nullptr;
    void* PL_PS_data_ptr = nullptr;
    void* PS_PL_dma_instructions_ptr = nullptr;

    // how much memory has been allocated to the drivers
    uint32_t allocated_PS_PL_address = 0;
    uint32_t allocated_PL_PS_address = 0;

    std::vector<uint64_t> fpga_instructions_old;
    bool instructions_modified = false;

    fpga_instructions fpga_instr = fpga_instructions();

    std::vector<std::shared_ptr<base_driver>> drivers;  // this will point to all drivers that are loaded

    uint32_t load_driver(json config, std::string module_name);

    template <typename T>
    bool load_json_value(const json& config, const std::string& value_name, T* dest); 

    uint32_t load_mem_layout();

    uint32_t set_memory_pointers(fpga_mem* mem);   // set the memory pointers for the driver

    uint32_t allocate_driver_memory(const fpga_mem* mem);    // allocate memory that the driver will use

    uint32_t write_instructions_to_fpga();  // write the instructions to the FPGA

};