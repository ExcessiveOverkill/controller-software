#include "fpga_module_manager.h"
#include "fpga_module_driver_factory.h"
#include <fstream>
#include <chrono>

fpga_module_manager::fpga_module_manager(){
    config = json::object();
}

fpga_module_manager::~fpga_module_manager(){
}

uint32_t fpga_module_manager::load_config(std::string config_file){
    // load drivers and config based on json config file

    // Read JSON file
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open JSON configuration file." << std::endl;
        return 1;
    }

    config = json::parse(file);
    file.close();
    return 0;
}

uint32_t fpga_module_manager::load_mem_layout(){
    // get memory layout from config file
    if (!config.contains("controller")) {
        std::cerr << "Error: controller not found in config." << std::endl;
        return 1;
    }

    json controller_json = config["controller"];

    if (!controller_json.contains("driver_settings")) {
        std::cerr << "Error: driver_settings not found in config." << std::endl;
        return 1;
    }

    json driver_settings_json = controller_json["driver_settings"];

    bool success = true;

    success &= load_json_value(driver_settings_json, "OCM_BASE_ADDR", &mem_layout.OCM_BASE_ADDR);
    success &= load_json_value(driver_settings_json, "OCM_SIZE", &mem_layout.OCM_SIZE);
    success &= load_json_value(driver_settings_json, "PS_TO_PL_CONTROL_OFFSET", &mem_layout.PS_to_PL_control_base_addr_offset);
    success &= load_json_value(driver_settings_json, "PS_TO_PL_CONTROL_SIZE", &mem_layout.PS_to_PL_control_size);
    success &= load_json_value(driver_settings_json, "PL_TO_PS_CONTROL_OFFSET", &mem_layout.PL_to_PS_control_base_addr_offset);
    success &= load_json_value(driver_settings_json, "PL_TO_PS_CONTROL_SIZE", &mem_layout.PL_to_PS_control_size);
    success &= load_json_value(driver_settings_json, "PS_TO_PL_DATA_OFFSET", &mem_layout.PS_to_PL_data_base_addr_offset);
    success &= load_json_value(driver_settings_json, "PS_TO_PL_DATA_SIZE", &mem_layout.PS_to_PL_data_size);
    success &= load_json_value(driver_settings_json, "PL_TO_PS_DATA_OFFSET", &mem_layout.PL_to_PS_data_base_addr_offset);
    success &= load_json_value(driver_settings_json, "PL_TO_PS_DATA_SIZE", &mem_layout.PL_to_PS_data_size);
    success &= load_json_value(driver_settings_json, "PS_TO_PL_DMA_INSTRUCTION_OFFSET", &mem_layout.PS_to_PL_dma_instructions_base_addr_offset);
    success &= load_json_value(driver_settings_json, "PS_TO_PL_DMA_INSTRUCTION_SIZE", &mem_layout.PS_to_PL_dma_instructions_size);
    success &= load_json_value(driver_settings_json, "DATA_MEMORY_SIZE", &mem_layout.data_memory_size);

    if (!success) {
        std::cerr << "Error: Failed to load driver data from config." << std::endl;
        return 1;
    }
    return 0;
}


uint32_t fpga_module_manager::set_fpga_interface(Fpga_Interface* fpga_interface){
    // set the fpga interface
    this->fpga_interface = fpga_interface;
    return 0;
}

uint32_t fpga_module_manager::initialize_fpga(){

    // load memory layout
    if(load_mem_layout() != 0){
        std::cerr << "Failed to load memory layout" << std::endl;
        return 1;
    }

    // clear pointers
    PS_PL_control_ptr = nullptr;
    PL_PS_control_ptr = nullptr;
    PS_PL_data_ptr = nullptr;
    PL_PS_data_ptr = nullptr;
    PS_PL_dma_instructions_ptr = nullptr;


    // initialize the fpga
    // TODO: this path should come from the config file somewhere
    uint32_t ret = fpga_interface->initialize(mem_layout, "/home/em-os/controller/config/fpga_configs/bit_files/bitfile.bit.bin");
    if(ret != 0){
        std::cout << "FPGA initialization failed" << std::endl;
        return 2;
    }

    // copy memory pointers for later use
    PS_PL_control_ptr = fpga_interface->get_PS_to_PL_control_pointer(0);
    PL_PS_control_ptr = fpga_interface->get_PL_to_PS_control_pointer(0);
    PS_PL_data_ptr = fpga_interface->get_PS_to_PL_data_pointer(0);
    PL_PS_data_ptr = fpga_interface->get_PL_to_PS_data_pointer(0);
    PS_PL_dma_instructions_ptr = fpga_interface->get_PS_to_PL_dma_instructions_pointer(0);

    return 0;
}

uint32_t fpga_module_manager::load_drivers(){
    // load and configure drivers based on json config file

    bool missing_driver = false;

    uint8_t node_count = 2;

    for (json::iterator it = config.begin(); it != config.end(); ++it) {
        json node_config = it.value();
        std::string node_name = it.key();
        std::cout << "Loading node: " << node_name << std::endl;

        if(!node_config.contains("node")){    // skip if its not a node
            continue;
        }
        node_count++;
        uint32_t ret = load_driver(config, node_name);
        if(ret != 0){
            missing_driver = true;
        }
    }

    // add some NOP instructions to make sure the final send instructions are completed before the axi transfer starts
    for(int i = 0; i < (node_count+2)*4; i++){
        fpga_instructions_old.push_back(create_instruction_NOP());
    }

    if(missing_driver){
        std::cerr << "Failed to load one or more FPGA node drivers" << std::endl;
        return 1;
    }

    return 0;
}

std::shared_ptr<base_driver> fpga_module_manager::get_driver(uint32_t index){
    if(index >= drivers.size()){
        return nullptr;
    }
    // get driver by index
    return drivers[index];
}

uint32_t fpga_module_manager::run_update(){
    // run the update sequence for the FPGA

    // run  the drivers
    for(auto driver : drivers){
        driver->run();
    }

    // write the instructions to the FPGA
    write_instructions_to_fpga();   // TODO: handle the instructions being larger than the write block size, optimize order, handle read/write sequence delays, put all dynamic instructions in one block

    return 0;
}

uint32_t fpga_module_manager::write_instructions_to_fpga(){
    // write the instructions to the FPGA memory
    // TODO: add support for multiple instruction blocks (when instruction count becomes very large)
    uint32_t index = 0;
    for(auto instruction : fpga_instructions_old){
        reinterpret_cast<uint64_t*>(PS_PL_dma_instructions_ptr)[index] = instruction;
        index++;
    }

    return 0;
}

uint32_t fpga_module_manager::load_driver(json config, std::string module_name){
    // load driver based on json config file
    json compatible_drivers = config[module_name]["node"]["compatible_drivers"];

    bool driver_found = false;

    for (const auto& driver : compatible_drivers) {
        try{
            std::shared_ptr<base_driver> new_driver = Driver_Factory::create_shared(driver.get<std::string>());
            drivers.push_back(new_driver);
            driver_found = true;

            std::cout << "Loaded fpga module driver: " << driver.get<std::string>() << std::endl;
        }

        catch (const std::runtime_error& e) {
            if(std::string(e.what()).find("Type not registered") != std::string::npos){
                // driver not found, try the next one
                continue;
            }
        }
    }

    if (!driver_found) {
        std::cerr << "Error: No compatible drivers found for node: " << module_name << ", it will not be accessible" << std::endl;
        return 1;
    }

    // configure driver
    drivers.back()->microseconds = microseconds;   // used for syncronized timing

    if(set_memory_pointers(&drivers.back()->base_mem) != 0){
        std::cerr << "Failed to set memory pointers" << std::endl;
        drivers.pop_back();
        return 2;
    }

    if(drivers.back()->load_config(&config, module_name, node_core, &fpga_instr) != 0){
        std::cerr << "Failed to load driver post config" << std::endl;
        drivers.pop_back();
        return 3;
    }

    // allocate memory for the driver
    if(allocate_driver_memory(&drivers.back()->base_mem) != 0){
        std::cerr << "Failed to allocate memory for driver" << std::endl;
        drivers.pop_back();
        return 4;
    }

    return 0;
}

uint32_t fpga_module_manager::save_ps_nodes(std::string file_path){
    // save the fpga driver PS nodes to a json file so the user can see what is available

    return 1;   // not implemented yet
}

void fpga_module_manager::set_microseconds(const uint64_t* microseconds){
    // set the microseconds pointer for the drivers
    this->microseconds = microseconds;
}

template <typename T>
bool fpga_module_manager::load_json_value(const json& config, const std::string& value_name, T* dest){
    // helper function for loading values from the config file
    if (!config.contains(value_name)) {
        std::cerr << "Error: Constant '" << value_name << "' not found in config." << std::endl;
        return false;
    }

    *dest = config[value_name].get<T>();
    return true;
}

uint32_t fpga_module_manager::set_memory_pointers(fpga_mem* mem){
    // set the memory pointers for the driver

    mem->hardware_PS_PL_mem_offset = allocated_PS_PL_address / 4 + mem_layout.data_memory_size;  // top half is PS to PL
    mem->hardware_PL_PS_mem_offset = allocated_PL_PS_address / 4;    // bottom half is PL to PS

    mem->software_PS_PL_ptr = (char*)PS_PL_data_ptr + allocated_PS_PL_address;
    mem->software_PL_PS_ptr = (char*)PL_PS_data_ptr + allocated_PL_PS_address;

    return 0;
}

uint32_t fpga_module_manager::allocate_driver_memory(const fpga_mem* mem){
    // allocate memory for the driver (this is called after the driver has been configured)

    if(mem->software_PS_PL_size % 4 != 0 || mem->software_PL_PS_size % 4 != 0){
        std::cerr << "Error: Memory sizes must be in 4 byte increments" << std::endl;
        return 1;
    }

    if(mem->software_PS_PL_size + allocated_PS_PL_address >= mem_layout.PS_to_PL_control_size){
        std::cerr << "Error: Not enough PS->PL memory for driver" << std::endl;
        return 2;
    }

    if(mem->software_PL_PS_size + allocated_PL_PS_address >= mem_layout.PL_to_PS_control_size){
        std::cerr << "Error: Not enough PL->PS memory for driver" << std::endl;
        return 3;
    }

    allocated_PS_PL_address += mem->software_PS_PL_size;
    allocated_PL_PS_address += mem->software_PL_PS_size;
    
    return 0;
}


uint32_t fpga_module_manager::create_global_variables(){
    // create global variables from fpga module drivers
    // for now the global variables created are just hardcoded in the drivers


    return 0;
}


uint32_t fpga_module_manager::compile_instructions(){
    // compile the instructions for the FPGA
    fpga_instr.compile();
    return 0;
}
