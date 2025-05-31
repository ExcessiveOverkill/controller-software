#include "fpga_instructions.h"
#include <chrono>
#include <fstream>
#include "register_helper.h"

void fpga_instructions::setup(json* fpga_config, Node_Core* node_core){
    this->fpga_config = fpga_config;
    this->node_core = node_core;

    settings.total_nodes = (*fpga_config)["controller"]["driver_settings"]["NODE_COUNT"].get<uint8_t>();
    settings.inter_node_cycles = (*fpga_config)["controller"]["driver_settings"]["INTER_NODE_CYCLES"].get<uint8_t>();
    settings.intra_node_cycles = (*fpga_config)["controller"]["driver_settings"]["INTRA_NODE_CYCLES"].get<uint8_t>();
    settings.dma_cycles = (*fpga_config)["controller"]["driver_settings"]["DMA_CYCLES"].get<uint8_t>();
}

uint32_t fpga_instructions::copy::set_time_reference(uint32_t time_reference){
    this->time_reference = time_reference;
    return 0;
}

uint32_t fpga_instructions::copy::set_time_reference(copy* ref_instruction, bool write_priority){
    this->ref_instruction = ref_instruction;
    if(write_priority){
        ref_instruction_write_priority = true;
    }
    else{
        ref_instruction_read_priority = true;
    }
    return 0;
}

uint32_t fpga_instructions::copy::set_execution_window(int32_t earliest, int32_t latest){
    if(earliest > latest){
        throw std::invalid_argument("earliest execution time must be before latest execution time");
    }
    time_window = latest - earliest;
    time_window++;
    this->earliest_execution = earliest;
    this->latest_execution = latest;
    return 0;
}

uint32_t fpga_instructions::copy::set_write_priority(){
    this->write_priority = true;
    this->read_priority = false;
    return 0;
}

uint32_t fpga_instructions::copy::set_read_priority(){
    this->read_priority = true;
    this->write_priority = false;
    return 0;
}

uint32_t fpga_instructions::copy::load_from_json(json* json_data){
    /*
    {
    "instruction_index": 0,
    "source": "full.source.name",
    "destination": "full.destination.name",
    "time_reference_cycle": -1,
    "time_reference_instruction": -1,
    "time_reference_instruction_edge": "write", // what edge of the dependent reference instruction copy we're basing our timing on
    "execution_window_earliest": 0,
    "execution_window_latest": 0,
    "edge": "write",
    "dynamic_select_source": "full.dynamic.source.name"
    }
     */

    // ensure all required fields are present
    if(
        !json_data->contains("instruction_index") ||
        !json_data->contains("source") ||
        !json_data->contains("destination") ||
        !json_data->contains("time_reference_cycle") || 
        !json_data->contains("time_reference_instruction") || 
        !json_data->contains("time_reference_instruction_edge") || 
        !json_data->contains("execution_window_earliest") || 
        !json_data->contains("execution_window_latest") || 
        !json_data->contains("edge") ||
        !json_data->contains("dynamic_select_source")){

        std::cerr << "Error: JSON data missing required fields" << std::endl;
        return 1;
    }

    // load fields
    (*json_data)["instruction_index"].get_to(instruction_index);
    (*json_data)["source"].get_to(source_name);
    (*json_data)["destination"].get_to(destination_name);
    uint32_t time_ref = -1;
    uint32_t time_ref_instruction = -1;
    (*json_data)["time_reference_cycle"].get_to(time_ref);
    (*json_data)["time_reference_instruction"].get_to(time_ref_instruction);
    std::string time_reference_instruction_edge;
    (*json_data)["time_reference_instruction_edge"].get_to(time_reference_instruction_edge);
    int32_t earliest = 0;
    int32_t latest = 0;
    (*json_data)["execution_window_earliest"].get_to(earliest);
    (*json_data)["execution_window_latest"].get_to(latest);
    std::string edge;
    (*json_data)["edge"].get_to(edge);
    std::string dynamic_select_source;
    (*json_data)["dynamic_select_source"].get_to(dynamic_select_source);

    set_execution_window(earliest, latest);

    if(edge == "write"){
        set_write_priority();
    }
    else if(edge == "read"){
        set_read_priority();
    }
    else{
        std::cerr << "Error: Invalid instruction edge" << std::endl;
        return 1;
    }

    if(time_ref != -1){
        set_time_reference(time_ref);
    }
    else if(time_ref_instruction != -1){
        if(time_reference_instruction_edge == "write"){
            ref_instruction_write_priority = true;
        }
        else if(time_reference_instruction_edge == "read"){
            ref_instruction_read_priority = true;
        }
        else{
            std::cerr << "Error: Invalid time reference instruction edge" << std::endl;
            return 1;
        }
        ref_instruction = nullptr;    // must set this pointer later
    }
    else{
        std::cerr << "Error: No time reference" << std::endl;
        return 1;
    }

    if(dynamic_select_source != ""){
        set_register_index_source(dynamic_select_source);
    }

    return 0;
}

uint32_t fpga_instructions::copy::save_to_json(json* json_data){
    /*
    {
    "instruction_index": 0,
    "source": "full.source.name",
    "destination": "full.destination.name",
    "time_reference_cycle": -1,
    "time_reference_instruction": -1,
    "time_reference_instruction_edge": "write",
    "execution_window_earliest": 0,
    "execution_window_latest": 0,
    "edge": "write",
    "dynamic_select_source": "full.dynamic.source.name"
    }
     */

    if(ref_instruction != nullptr){
        time_reference_instruction = ref_instruction->instruction_index;
    }

    // save fields
    (*json_data)["instruction_index"] = instruction_index;
    (*json_data)["source"] = source_name;
    (*json_data)["destination"] = destination_name;
    (*json_data)["time_reference_cycle"] = time_reference;
    (*json_data)["time_reference_instruction"] = -1;    // TODO: must set this pointer later
    (*json_data)["time_reference_instruction_edge"] = "write";    // TODO: must set this pointer later
    (*json_data)["execution_window_earliest"] = earliest_execution;
    (*json_data)["execution_window_latest"] = latest_execution;
    if(write_priority){
        (*json_data)["edge"] = "write";
    }
    else{
        (*json_data)["edge"] = "read";
    }
    (*json_data)["dynamic_select_source"] = register_index_source_name;

    return 0;
}

uint32_t fpga_instructions::save(std::string file_path, std::string fpga_config_file, bool write_protected){
    // check if the file already exists and if it is write protected
    if(std::filesystem::exists(file_path)){
        // open and parse the file to check if it is write protected
        std::ifstream f(file_path);
        if (!f.is_open()) {
            std::cerr << "Error: FPGA instruction config file already exists but is unable to be opened to check write permission" << std::endl;
            return 1;
        }

        try{
            json temp = json::parse(f);
            if(temp["info"]["write_protected"].get<bool>()){
                std::cerr << "Error: FPGA instruction config file already exists and is write protected" << std::endl;
                return 3;
            }
        }
        catch(json::parse_error& e){
            std::cerr << "Error: FPGA instruction config file exists but is unable to be parsed to check write permission" << std::endl;
            return 2;
        }
        f.close();
    }
    
    
    json j;

    // add info header
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_r(&now_time_t, &now_tm);
    std::stringstream dateStream;
    dateStream << std::put_time(&now_tm, "%Y-%m-%d");
    std::string dateStr = dateStream.str();
    std::stringstream timeStream;
    timeStream << std::put_time(&now_tm, "%H:%M:%S");
    std::string timeStr = timeStream.str();

    j["info"]["date"] = dateStr;
    j["info"]["time"] = timeStr;
    j["info"]["fpga_config"] = fpga_config_file;
    j["info"]["write_protected"] = write_protected;

    for(auto instruction : instructions){
        json instruction_json;
        instruction.save_to_json(&instruction_json);
        j["instructions"].push_back(instruction_json);
    }

     std::filesystem::path p(file_path);
    if (!std::filesystem::exists(p.parent_path())) {
        try {
            std::filesystem::create_directories(p.parent_path());
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error: could not create FPGA instruction config directory: " << e.what() << std::endl;
            return 5;
        }
    }

    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open FPGA instruction config file." << std::endl;
        return 4;
    }

    file << j.dump(4);
    file.close();

    std::cout << "FPGA instruction config saved to: " << file_path << std::endl;

    return 0;
}

uint32_t fpga_instructions::load(std::string file_path, std::string fpga_config_file){
    // load instructions from a json file

    // clear all existing instructions
    instructions.clear();
    condensed_instructions.clear();
    running_instruction_index = 0;

    // load instructions from the file
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << file_path << std::endl;
        return 1;
    }

    json j = json::parse(file);

    std::string fpga_config;

    j["info"]["fpga_config"].get_to(fpga_config);

    if(fpga_config != fpga_config_file){
        std::cerr << "Error: FPGA instruction config file does not match FPGA config file" << std::endl;
        return 2;
    }

    // load instructions
    for(auto instruction : j["instructions"]){
        copy new_instruction;
        new_instruction.load_from_json(&instruction);
        add(new_instruction);
    }
    std::vector<uint32_t> instruction_indexes;

    // link reference instructions
    for(auto instruction : instructions){
        
        for(auto index : instruction_indexes){
            if(instruction.instruction_index == index){
                std::cerr << "Error: duplicate instruction index" << std::endl;
                return 3;
            }
        }

        instruction_indexes.push_back(instruction.instruction_index);
        if(instruction.time_reference_instruction != -1){

            if(instruction.ref_instruction != nullptr){
                std::cerr << "Error: instruction already has a reference instruction" << std::endl;
                return 4;
            }

            for(auto ref_instruction : instructions){
                if(ref_instruction.instruction_index == instruction.time_reference_instruction){
                    instruction.ref_instruction = &ref_instruction;
                    break;
                }
            }

            if(instruction.ref_instruction == nullptr){
                std::cerr << "Error: referenced instruction not found" << std::endl;
                return 5;
            }
        }
    }

    return 0;
}

uint32_t fpga_instructions::copy::set_destination(std::string name){

    if(name == ""){
        throw std::invalid_argument("name is empty");
    }

    if(name.find('.') == std::string::npos){
        throw std::invalid_argument("name is not a full name");
    }

    if(name.substr(0, name.find('.')) == "fpga"){
        // TODO: test load register here
    }
    else if(name.substr(0, name.find('.')) == "node_vars"){
        // TODO: test load node variable here
    }
    else{
        throw std::invalid_argument("name is not a valid type");
    }

    destination_name = name;
    return 0;
}

uint32_t fpga_instructions::copy::set_source(std::string name){

    if(name == ""){
        throw std::invalid_argument("name is empty");
    }

    if(name.find('.') == std::string::npos){
        throw std::invalid_argument("name is not a full name");
    }

    if(name.substr(0, name.find('.')) == "fpga"){
        // TODO: test load register here
    }
    else if(name.substr(0, name.find('.')) == "node_vars"){
        // TODO: test load node variable here
    }
    else{
        throw std::invalid_argument("name is not a valid type");
    }

    source_name = name;
    return 0;
}

uint32_t fpga_instructions::copy::set_register_index_source(std::string name){
    if(name == ""){
        throw std::invalid_argument("name is empty");
    }

    if(name.find('.') == std::string::npos){
        throw std::invalid_argument("name is not a full name");
    }

    if(name.substr(0, name.find('.')) == "node_vars"){
        // TODO: test load node variable here
    }
    else{
        throw std::invalid_argument("name is not a valid type");
    }

    register_index_source_name = name;
    is_dynamic = true;
    return 0;
}

void fpga_instructions::copy::clear_register_index_source(){
    register_index_source_name = "";
    is_dynamic = false;
}

uint32_t fpga_instructions::add(copy instruction){
    if(instruction.instruction_index == -1){
        instruction.instruction_index = running_instruction_index++;
    }
    else if(running_instruction_index > instruction.instruction_index){
        running_instruction_index = instruction.instruction_index + 1;
    }
    instructions.push_back(instruction);
    return 0;
}

void fpga_instructions::set_update_frequency(uint32_t frequency){
    full_update_counts = 100e6 / frequency; // DMA cycles in one update period
}

uint32_t fpga_instructions::compile(){
    /*
    cycle through all instructions
    instructions with the smallest window of execution are placed first (providing they are not dependent on an un-placed instruction)
    */

    // get register and node variable data
    for(auto it = instructions.begin(); it != instructions.end(); ++it){
        copy* instruction = &(*it);

        Register* source_reg = nullptr;
        Register* destination_reg = nullptr;

        std::string source_node_var = "";
        std::string destination_node_var = "";
        std::string register_index_source_node_var = "";

        if(instruction->time_reference == -2){  // -2 means use the end of the update period as the time reference
            instruction->time_reference = full_update_counts - 2500;    // ~25us before the end of the update period to ensure there is time for the AXI transfer (~20us) to complete
        }

        if(instruction->source_name == ""){
            std::cerr << "Error: instruction source not set" << std::endl;
            return 1;
        }
        if(instruction->destination_name == ""){
            std::cerr << "Error: instruction destination not set" << std::endl;
            return 1;
        }

        // create source
        if(instruction->source_name.substr(0, instruction->source_name.find('.')) == "fpga"){ // register
            if(get_register_from_full_name(instruction->source_name, fpga_config, base_mem, &source_reg) != 0){
                std::cerr << "Error: failed to get register from full name" << std::endl;
                return 1;
            }
            if(!source_reg->pl_data.read){
                std::cerr << "Error: FPGA instruction source register is not readable" << std::endl;
                return 1;
            }
        }
        else if(instruction->source_name.substr(0, instruction->source_name.find('.')) == "node_vars"){ // node variable
            source_node_var = instruction->source_name;
        }
        else{
            std::cerr << "Error: invalid instruction source" << std::endl;
            return 1;
        }

        // create destination
        if(instruction->destination_name.substr(0, instruction->destination_name.find('.')) == "fpga"){ // register
            if(get_register_from_full_name(instruction->destination_name, fpga_config, base_mem, &destination_reg) != 0){
                std::cerr << "Error: failed to get register from full name" << std::endl;
                return 1;
            }
            if(!destination_reg->pl_data.write){
                std::cerr << "Error: FPGA instruction destination register is not writable" << std::endl;
                return 1;
            }
        }
        else if(instruction->destination_name.substr(0, instruction->destination_name.find('.')) == "node_vars"){ // node variable
            destination_node_var = instruction->destination_name;
        }
        else{
            std::cerr << "Error: invalid instruction destination" << std::endl;
            return 1;
        }

        register_index_source_node_var = instruction->register_index_source_name;

        if(source_reg != nullptr && destination_reg != nullptr){
            // data transfer only in fpga, no need to sync with a node variable
            if(register_index_source_node_var != ""){
                throw std::invalid_argument("dynamic register index cannot be used with fpga to fpga transfer");
            }
            instruction->src_addr = source_reg->pl_data.absolute_address;
            instruction->src_node = source_reg->pl_data.node_index;
            instruction->dst_addr = destination_reg->pl_data.absolute_address;
            instruction->dst_node = destination_reg->pl_data.node_index;

            delete source_reg;
            delete destination_reg;
        }
        else if(source_reg != nullptr){
            // copy from fpga reg to node var
            sync_with_ps(source_reg, base_mem);
            if(node_core->set_global_variable_data_ptr(destination_node_var, source_reg->ps_data.software_data_ptr)){
                std::cerr << "Error: failed to set global variable data pointer" << std::endl;
                return 1;
            }
            instruction->src_addr = source_reg->pl_data.absolute_address;
            instruction->src_node = source_reg->pl_data.node_index;
            instruction->dst_addr = source_reg->ps_data.hardware_data_ptr;
            instruction->dst_node = 0;

            instruction->dynamic_reg_starting_address = source_reg->pl_data.absolute_address;

            delete source_reg;
        }
        else if(destination_reg != nullptr){
            // copy from node var to fpga reg
            sync_with_ps(destination_reg, base_mem);
            if(node_core->set_global_variable_data_ptr(source_node_var, destination_reg->ps_data.software_data_ptr)){
                std::cerr << "Error: failed to set global variable data pointer" << std::endl;
                return 1;
            }
            instruction->src_addr = destination_reg->ps_data.hardware_data_ptr;
            instruction->src_node = 0;
            instruction->dst_addr = destination_reg->pl_data.absolute_address;
            instruction->dst_node = destination_reg->pl_data.node_index;

            instruction->dynamic_reg_starting_address = destination_reg->pl_data.absolute_address;

            delete destination_reg;
        }
        else{
            // this should never happen
            std::cerr << "Error: invalid source and destination, cannot copy from node_var to node_var" << std::endl;
            return 1;
        }

        if(register_index_source_node_var != ""){
            //uint32_t* dynamic_reg_index = &(instruction->dynamic_reg_index);
            //node_core->set_global_variable_data_ptr(register_index_source_node_var, dynamic_reg_index);
            node_core->get_global_variable_data_ptr(register_index_source_node_var, (void**)&(instruction->dynamic_reg_index_ptr));
        }

    }

    settings.full_cycles = (settings.intra_node_cycles * settings.total_nodes) + (settings.inter_node_cycles * settings.total_nodes) + settings.dma_cycles;

    uint32_t smallest_window = 0;
    
    uint32_t instruction_count = instructions.size();   // setting the size here keeps us from ever attempting to place placeholder instructions

    while(1){
        uint32_t next_smallest_window = -1;

        for(uint32_t i = 0; i < instruction_count; i++){
            copy* instruction = &(instructions[i]);

            if(instruction->placed){
                continue;
            }
            
            if(instruction->ref_instruction != nullptr && !instruction->ref_instruction->placed){
                continue;
            }

            if(instruction->time_window < next_smallest_window && instruction->time_window != smallest_window){
                next_smallest_window = instruction->time_window;
            }

            if(instruction->time_window <= smallest_window){
                if(place_instruction(i) != 0){
                    std::cerr << "Error: failed to place instruction" << std::endl;
                    return 1;
                }
            }
        }

        if(next_smallest_window == -1){
            break;
        }

        smallest_window = next_smallest_window;

    }

    std::cout << "instructions placed" << std::endl;

    for(auto instruction : instructions){
        if(instruction.block_placeholder){
            continue;
        }
        std::cout << instruction.dma_execution_cycle << "\t" << instruction.source_name << " -> " << instruction.destination_name << std::endl;
    }

    condense_instructions();

    std::cout << "instructions condensed" << std::endl;

    // test print instructions
    // for(auto instruction : condensed_instructions){
    //     std::cout << instruction << std::endl;
    // }

    return 0;
}

uint32_t fpga_instructions::place_instruction(uint32_t index){

    int32_t earliest_cycle = 0;
    int32_t latest_cycle = 0;

    copy* instruction = &(instructions[index]);

    if(instruction->ref_instruction != nullptr){
        if(!instruction->ref_instruction->placed){
            throw std::invalid_argument("instruction is dependent on an un-placed instruction");
            return 1;
        }

        if(instruction->write_priority){    // use the write cycle time of the reference instruction
            earliest_cycle = instruction->ref_instruction->write_cycle + instruction->earliest_execution;
            latest_cycle = instruction->ref_instruction->write_cycle + instruction->latest_execution;
        }
        else{   // use the read cycle time of the reference instruction
            earliest_cycle = instruction->ref_instruction->read_cycle + instruction->earliest_execution;
            latest_cycle = instruction->ref_instruction->read_cycle + instruction->latest_execution;
        }
    }
    else{
        earliest_cycle = instruction->time_reference + instruction->earliest_execution;
        latest_cycle = instruction->time_reference + instruction->latest_execution;
    }

    if(latest_cycle < 0){
        throw std::invalid_argument("latest cycle is before 0");
        return 1;
    }

    if(!instruction->write_priority && !instruction->read_priority){
        throw std::invalid_argument("instruction has no read or write priority");
        return 1;
    }
    if(instruction->write_priority && instruction->read_priority){
        throw std::invalid_argument("instruction has both read and write priority");
        return 1;
    }


    for(uint32_t i = earliest_cycle; i <= latest_cycle; i++){
        int32_t send_time = i - settings.dma_cycles;
        uint8_t src_node = instruction->src_node;
        uint8_t dst_node = instruction->dst_node;


        if(instruction->write_priority){
            send_time -= (settings.intra_node_cycles * dst_node);
            send_time -= (settings.inter_node_cycles * dst_node);

        }
        else{
            send_time -= (settings.intra_node_cycles * src_node);
            send_time -= (settings.inter_node_cycles * src_node);
        }

        if(send_time < 0){
            continue;
        }

        // handle wrapping around the DMA for certain cases
        if(dst_node <= src_node){
            if(int32_t(send_time - settings.full_cycles) < 0 || !check_dma_index_available(send_time) || !check_dma_index_available(send_time - settings.full_cycles)){
                continue;
            }

            // add a placeholder instruction to handle the blocking wrap-around
            copy cpy;
            cpy.dma_execution_cycle = send_time;
            cpy.block_placeholder = true;
            cpy.blocking_instruction = instruction;
            cpy.placed = true;
            add(cpy);

            instruction = &(instructions[index]);   // get the pointer again incase the vector was resized

            send_time -= settings.full_cycles;
            instruction->dma_execution_cycle = send_time;
            instruction->write_cycle = settings.full_cycles;
        }
        else{
            if(!check_dma_index_available(send_time)){
                continue;
            }

            instruction->dma_execution_cycle = send_time;
            instruction->write_cycle = 0;
        }

        instruction->read_cycle = instruction->dma_execution_cycle + settings.dma_cycles + settings.intra_node_cycles * src_node + settings.inter_node_cycles * src_node;
        instruction->write_cycle += instruction->dma_execution_cycle + settings.dma_cycles + settings.intra_node_cycles * dst_node + settings.inter_node_cycles * dst_node;
        instruction->placed = true;
        return 0;
    }

    return 1;
}

bool fpga_instructions::check_dma_index_available(uint32_t dma_index){
    for(auto instruction : instructions){
        if(instruction.dma_execution_cycle == dma_index){
            return false;
        }
    }
    return true;
}

uint32_t fpga_instructions::condense_instructions(){
    // add wait instructions to fill in the gaps between instructions

    uint32_t index = 0;
    uint32_t cycle = 0;
    uint32_t prev_send_cycle = 0;

    dynamic_instructions.clear();   // clear the dynamic instructions list


    // cant sort the instructions here because we need to keep the order of the instructions for pointers referencing them
    // instead make a temporary array of pointers to the instructions and sort that


    struct sorted_instruction{
        uint32_t original_index;
        copy* instruction;
    };

    sorted_instruction sorted_instructions[instructions.size()];

    for (size_t i = 0; i < instructions.size(); ++i) {
        sorted_instructions[i].instruction = &instructions[i];
        sorted_instructions[i].original_index = i;
    }

    std::sort(sorted_instructions, sorted_instructions + instructions.size(), [](const sorted_instruction a, const sorted_instruction b) {
        return a.instruction->dma_execution_cycle < b.instruction->dma_execution_cycle;
    });

    for(auto sorted_inst : sorted_instructions){
        copy* instruction = sorted_inst.instruction;

        if(instruction->block_placeholder){
            continue;
        }

        if(!instruction->placed){
            throw std::invalid_argument("instruction is not placed");
        }

        if(instruction->dma_execution_cycle <= prev_send_cycle && index != 0){
            throw std::invalid_argument("instruction dma execution cycle is before previous instruction");
            return 1;
        }

        uint32_t diff = instruction->dma_execution_cycle - prev_send_cycle;
        if(diff > 2){
            cycle += diff - 1;
            condensed_instructions.push_back(create_instruction_WAIT(cycle));
            index++;
        }
        // else if(diff == 2){
        //     condensed_instructions.push_back(create_instruction_NOP());
        //     cycle++;
        //     index++;
        // }

        
        condensed_instructions.push_back(create_instruction_COPY(instruction->src_node, instruction->src_addr, instruction->dst_node, instruction->dst_addr));
        

        // add dynamic data to vector so we know where to update them later
        if(instruction->is_dynamic){
            dynamic_data data;
            data.instruction_object_index = sorted_inst.original_index;
            data.condensed_instruction_index = condensed_instructions.size()-1;
            dynamic_instructions.push_back(data);
        }
        
        cycle++;
        index++;

        prev_send_cycle = instruction->dma_execution_cycle;
    }

    // add a final wait to ensure all instructions are completed
    //condensed_instructions.push_back(create_instruction_WAIT(cycle + settings.full_cycles));


    // run DMA for entire cycle to keep the AXI transfer in sync
    condensed_instructions.push_back(create_instruction_WAIT(full_update_counts - 2500));   // ~25us before the end of the update period to ensure there is time for the AXI transfer (~20us) to complete

    index++;

    condensed_instructions.push_back(create_instruction_END());

    return 0;
}

uint32_t fpga_instructions::update_dynamic_instructions(){
    // update the dynamic instructions without recompiling everything

    for(auto data : dynamic_instructions){
        copy* instruction = &(instructions[data.instruction_object_index]);

        uint32_t* dynamic_reg_index = &(instruction->dynamic_reg_index);    // for testing

        uint32_t index = *(instruction->dynamic_reg_index_ptr);

        if(index == -1){
            condensed_instructions[data.condensed_instruction_index] = create_instruction_NOP();    // do nothing
            continue;
        }

        uint32_t addr = instruction->dynamic_reg_starting_address + index;

        if(instruction->src_node != 0){
            instruction->src_addr = addr;
        }
        else{
            instruction->dst_addr = addr;
        }

         condensed_instructions[data.condensed_instruction_index] = create_instruction_COPY(instruction->src_node, instruction->src_addr, instruction->dst_node, instruction->dst_addr);
    }

    return 0;
}

uint64_t fpga_instructions::create_instruction_COPY(uint8_t src_node, uint16_t src_addr, uint8_t dst_node, uint16_t dst_addr){
    std::cout << "src_node: " << (uint64_t)src_node << " src_addr: " << (uint64_t)src_addr << " dst_node: " << (uint64_t)dst_node << " dst_addr: " << (uint64_t)dst_addr << std::endl;
    return  ((uint64_t)src_node << 0) | ((uint64_t)dst_node << 8) | ((uint64_t)src_addr << 16) | ((uint64_t)dst_addr << 32) | ((uint64_t)instruction_type::COPY << 48);
}

uint64_t fpga_instructions::create_instruction_WAIT(uint32_t cycles){
    std::cout << "wait: " << cycles << std::endl;
    return ((uint64_t)cycles) | ((uint64_t)instruction_type::WAIT << 48);
}

uint64_t fpga_instructions::create_instruction_END(){
    //std::cout << "end" << std::endl;
    return ((uint64_t)instruction_type::END << 48);
}

uint64_t fpga_instructions::create_instruction_NOP(){
    //std::cout << "nop" << std::endl;
    return ((uint64_t)instruction_type::NOP << 48);
}



