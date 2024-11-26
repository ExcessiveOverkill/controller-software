
#pragma once

#include <stdint.h>


enum instruction_type : uint8_t {
    END = 0,
    NOP = 1,
    COPY = 2
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
    
    return  ((uint64_t)src_node << 0) | ((uint64_t)dst_node << 8) | ((uint64_t)src_addr << 16) | ((uint64_t)dst_addr << 32) | ((uint64_t)instruction_type::COPY << 48);
}