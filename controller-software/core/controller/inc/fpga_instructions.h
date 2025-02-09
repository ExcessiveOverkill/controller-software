
#pragma once

#include <stdint.h>
#include <iostream>
#include <vector>
#include "register_helper.h"


enum instruction_type : uint8_t {
    END = 0,
    NOP = 1,
    COPY = 2,
    WAIT = 3
};

static uint64_t create_instruction_END(){
    return ((uint64_t)instruction_type::END << 48);
}

static uint64_t create_instruction_NOP(){
    return ((uint64_t)instruction_type::NOP << 48);
}

static uint64_t create_instruction_COPY(uint8_t src_node, uint16_t src_addr, uint8_t dst_node, uint16_t dst_addr){
    // you may copy to/from any address in node 0 (memory which is synced with the PS),
    // but only the lower half is copied to the PS, the upper half is copied from the PS to PL
    // the halfway offset is determined by DATA_MEMORY_SIZE in the config (in 32 bit words)
    // node addresses are in single increments, but contain 32 bit words
    std::cout << "src_node: " << (uint64_t)src_node << " src_addr: " << (uint64_t)src_addr << " dst_node: " << (uint64_t)dst_node << " dst_addr: " << (uint64_t)dst_addr << std::endl;
    return  ((uint64_t)src_node << 0) | ((uint64_t)dst_node << 8) | ((uint64_t)src_addr << 16) | ((uint64_t)dst_addr << 32) | ((uint64_t)instruction_type::COPY << 48);
}



class fpga_instructions{

    public:
        class copy{
            public:
                uint8_t src_node;
                uint16_t src_addr;
                uint8_t dst_node;
                uint16_t dst_addr;

                uint32_t time_reference = 0;
                int32_t earliest_execution = 0;
                int32_t latest_execution = 0;
                bool write_priority = false;
                bool read_priority = false;

                copy* ref_instruction = nullptr;
                bool ref_instruction_write_priority = false;
                bool ref_instruction_read_priority = false;

                uint32_t time_window = 0;

                bool placed = false;
                bool block_placeholder = false;
                copy* blocking_instruction = nullptr;
                uint32_t dma_execution_cycle = -1;
                uint32_t read_cycle = -1;
                uint32_t write_cycle = -1;

            
                uint32_t set_time_reference(uint32_t cycle);    // set an absolute time reference for the instruction (dma cycles)
                uint32_t set_time_reference(copy* ref_instruction, bool write_priority); // set an absolute time reference for the instruction (based on another instruction)
                uint32_t set_execution_window(int32_t earliest, int32_t latest);    // set when the instruction can be executed, relative to the time reference
                uint32_t set_write_priority();    // control the timing of the write action
                uint32_t set_read_priority();    // control the timing of the read action

                void set_source(Register* reg);
                //void set_source(Global_Variable* var);
                void set_destination(Register* reg);
                //void set_destination(Global_Variable* var);

                uint32_t load_from_json(json* json_data);
                uint32_t save_to_json(json* json_data);

            private:
                uint32_t instruction_index = -1;
                uint32_t time_reference_instruction = -1;


                std::string source_name = "";
                std::string destination_name = "";

        };


        struct dma_settings{
            uint32_t inter_node_cycles = 1;
            uint32_t intra_node_cycles = 2;
            uint32_t dma_cycles = 1;
            uint8_t total_nodes = 5;
            uint32_t full_cycles = 0;
        } settings;

        uint32_t add(copy* instruction);

        uint32_t compile();

        

    private:

        uint32_t place_instruction(copy* instruction);
        uint32_t condense_instructions();

        bool check_dma_index_available(uint32_t dma_index);

        uint64_t create_instruction_COPY(uint8_t src_node, uint16_t src_addr, uint8_t dst_node, uint16_t dst_addr);
        uint64_t create_instruction_WAIT(uint32_t cycles);
        uint64_t create_instruction_END();
        uint64_t create_instruction_NOP();

        std::vector<copy*> instructions;

        std::vector<uint64_t> condensed_instructions;
};