#include "fpga_instructions.h"

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
    this->earliest_execution = earliest;
    this->latest_execution = latest;
    return 0;
}

uint32_t fpga_instructions::copy::set_write_priority(){
    this->write_priority = true;
    return 0;
}

uint32_t fpga_instructions::copy::set_read_priority(){
    this->read_priority = true;
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
    "time_reference_instruction_edge": "write",
    "execution_window_earliest": 0,
    "execution_window_latest": 0,
    "edge": "write"
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
        !json_data->contains("edge")){

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
        ref_instruction = nullptr;    // TODO: must set this pointer later
    }
    else{
        std::cerr << "Error: No time reference" << std::endl;
        return 1;
    }

    return 0;
}

uint32_t fpga_instructions::add(copy* instruction){
    if(instruction == nullptr){
        throw std::invalid_argument("instruction is null");
    }
    instructions.push_back(instruction);
    return 0;
}

uint32_t fpga_instructions::compile(){
    /*
    cycle through all instructions
    instructions with the smallest window of execution are placed first (providing they are not dependent on an un-placed instruction)
     */

    settings.full_cycles = (settings.intra_node_cycles * settings.total_nodes) + (settings.inter_node_cycles * settings.total_nodes) + settings.dma_cycles;

    uint32_t smallest_window = 0;
    
    uint32_t place_ret = 0;

    while(1){
        uint32_t next_smallest_window = -1;

        for(auto instruction : instructions){
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
                place_ret = place_instruction(instruction);
            }

            if(place_ret != 0){
                std::cout << "error placing instruction" << std::endl;
                return place_ret;
            }

        }

        if(next_smallest_window == -1){
            break;
        }

        smallest_window = next_smallest_window;

    }

    std::cout << "instructions placed" << std::endl;

    condense_instructions();

    std::cout << "instructions condensed" << std::endl;

    // test print instructions
    for(auto instruction : condensed_instructions){
        std::cout << instruction << std::endl;
    }

    return 0;
}

uint32_t fpga_instructions::place_instruction(copy* instruction){

    int32_t earliest_cycle = 0;
    int32_t latest_cycle = 0;

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


    for(uint32_t i = earliest_cycle; i < latest_cycle; i++){
        uint32_t send_time = i - settings.dma_cycles;
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
            if(!check_dma_index_available(send_time) || !check_dma_index_available(send_time - settings.full_cycles)){
                continue;
            }

            // add a placeholder instruction to handle the blocking wrap-around
            auto cpy = new copy();
            cpy->dma_execution_cycle = send_time;
            cpy->block_placeholder = true;
            cpy->blocking_instruction = instruction;
            cpy->placed = true;
            instructions.push_back(cpy);

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
        if(instruction->dma_execution_cycle == dma_index){
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

    for(auto instruction : instructions){
        if(instruction->block_placeholder){
            continue;
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
        cycle++;
        index++;

        prev_send_cycle = instruction->dma_execution_cycle;
    }

    // add a final wait to ensure all instructions are completed
    condensed_instructions.push_back(create_instruction_WAIT(cycle + settings.full_cycles));
    index++;

    condensed_instructions.push_back(create_instruction_END());

    return 0;
}

uint64_t fpga_instructions::create_instruction_COPY(uint8_t src_node, uint16_t src_addr, uint8_t dst_node, uint16_t dst_addr){
    std::cout << "src_node: " << (uint64_t)src_node << " src_addr: " << (uint64_t)src_addr << " dst_node: " << (uint64_t)dst_node << " dst_addr: " << (uint64_t)dst_addr << std::endl;
    return  ((uint64_t)src_node << 0) | ((uint64_t)dst_node << 8) | ((uint64_t)src_addr << 16) | ((uint64_t)dst_addr << 32) | ((uint64_t)instruction_type::COPY << 48);
}

uint64_t fpga_instructions::create_instruction_WAIT(uint32_t cycles){
    return ((uint64_t)cycles) | ((uint64_t)instruction_type::WAIT << 48);
}

uint64_t fpga_instructions::create_instruction_END(){
    return ((uint64_t)instruction_type::END << 48);
}

uint64_t fpga_instructions::create_instruction_NOP(){
    return ((uint64_t)instruction_type::NOP << 48);
}



