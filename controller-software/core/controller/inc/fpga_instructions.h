
#pragma once

#include <stdint.h>
#include <iostream>
#include <vector>
#include "register_helper.h"
#include "node_core.h"

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
                uint8_t src_node = 255;
                uint16_t src_addr = 0;
                uint8_t dst_node = 255;
                uint16_t dst_addr = 0;

                uint32_t dynamic_reg_index = -1;
                uint16_t dynamic_reg_starting_address = 0;
                bool is_dynamic = false;

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

                uint32_t set_source(std::string name);
                uint32_t set_destination(std::string name);

                // for dynamic data access
                uint32_t set_register_index_source(std::string name);
                void clear_register_index_source();

                uint32_t load_from_json(json* json_data);
                uint32_t save_to_json(json* json_data);

                int32_t instruction_index = -1;
                int32_t time_reference_instruction = -1;

                std::string source_name = "";   // node var or register name
                std::string destination_name = "";  // node var or register name
                std::string register_index_source_name = "";    // node var name

        };


        struct dma_settings{
            uint8_t inter_node_cycles = 1;
            uint8_t intra_node_cycles = 2;
            uint8_t dma_cycles = 1;
            uint8_t total_nodes = 5;
            uint32_t full_cycles = 0;
        } settings;

        uint32_t add(copy instruction);

        void set_update_frequency(uint32_t frequency);

        uint32_t compile();

        uint32_t save(std::string file_path, std::string fpga_config_file, bool write_protected = false);

        uint32_t load(std::string file_path, std::string fpga_config_file);

        void setup(json* fpga_config, Node_Core* node_core);
        fpga_mem* base_mem = nullptr;

        Node_Core* node_core = nullptr;

        std::vector<uint64_t> condensed_instructions;

        uint32_t update_dynamic_instructions(); // update the dynamic instructions without recompiling everything

        

    private:

        uint32_t place_instruction(uint32_t index);
        uint32_t condense_instructions();

        bool check_dma_index_available(uint32_t dma_index);

        uint64_t create_instruction_COPY(uint8_t src_node, uint16_t src_addr, uint8_t dst_node, uint16_t dst_addr);
        uint64_t create_instruction_WAIT(uint32_t cycles);
        uint64_t create_instruction_END();
        uint64_t create_instruction_NOP();

        std::vector<copy> instructions;

        uint32_t running_instruction_index = 0;

        uint32_t full_update_counts = 0;

        json* fpga_config = nullptr;

        struct dynamic_data{
            uint32_t instruction_object_index = -1;
            uint32_t condensed_instruction_index = -1;
        };

        std::vector<dynamic_data> dynamic_instructions;
};