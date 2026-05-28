#include <stdint.h>
#include <string>
#include <poll.h>

#pragma once

struct fpga_mem_layout {
    
    uint32_t OCM_BASE_ADDR=0;
    uint32_t OCM_SIZE=0;

    uint32_t PS_to_PL_control_base_addr_offset=0;
    uint32_t PS_to_PL_control_size=0;   // size of the memory in 8 bit words
    uint32_t PL_to_PS_control_base_addr_offset=0;
    uint32_t PL_to_PS_control_size=0;   // size of the memory in 8 bit words

    uint32_t PS_to_PL_data_base_addr_offset=0;
    uint32_t PS_to_PL_data_size=0;   // size of the memory in 8 bit words
    uint32_t PL_to_PS_data_base_addr_offset=0;
    uint32_t PL_to_PS_data_size=0;   // size of the memory in 8 bit words

    uint32_t PS_to_PL_dma_instructions_base_addr_offset=0;
    uint32_t PS_to_PL_dma_instructions_size=0;   // size of the memory in 8 bit words

    uint32_t data_memory_size=0;    // size of the data memory in 32 bit words
};


class Fpga_Interface {
public:
    struct IrqCountSnapshot {
        uint32_t run_count = 0;
        uint32_t done_count = 0;
        uint32_t run_delta = 0;
        uint32_t done_delta = 0;
        bool baseline_valid = false;
        int32_t baseline_run_minus_done = 0;
        int32_t current_run_minus_done = 0;
        uint64_t delta_change_events = 0;
        uint64_t increment_anomaly_events = 0;
        bool last_increment_anomaly = false;
    };

    Fpga_Interface();
    ~Fpga_Interface();

    uint32_t initialize(fpga_mem_layout mem_layout, std::string bitstreamPath); // resets shared memory and loads bitstream


    // get pointers to memory accessible by the FPGA
    // these should ONLY be used after the FPGA has finished reading/writing to the memory and the fpga_busy flag is cleared
    // note that if the FPGA memory becomes unallocated, these functions will throw errors, but the previously returned pointers will likely lead to segfaults
    void* get_PS_to_PL_control_pointer(uint32_t offset);    // should only be written to
    void* get_PL_to_PS_control_pointer(uint32_t offset);    // should only be read from

    void* get_PS_to_PL_data_pointer(uint32_t offset);    // should only be written to
    void* get_PL_to_PS_data_pointer(uint32_t offset);    // should only be read from

    void* get_PS_to_PL_dma_instructions_pointer(uint32_t offset);   // should only be written to

    uint32_t wait_for_update();   // wait for new data from the FPGA to be ready

    IrqCountSnapshot get_irq_count_snapshot() const;

    uint32_t set_update_frequency(uint32_t frequency);    // frequency at which the FPGA will update in Hz

    void cache_flush_all(); // writes any changed data from CPU to memory
    void cache_invalidate_all();    // invalidates cached memory from FPGA



private:
    fpga_mem_layout mem_layout;

    uint16_t* fpga_main_trigger_counter = nullptr;    // 16 bit counter at 25 Mhz
    uint16_t* fpga_watchdog = nullptr;    // this value must be written at LEAST every 16 update cycles to prevent the FPGA DMA from shutting down
    bool first_cycle = true;

    void* ocm_base_pointer;

    struct pollfd mem_update_running_fds[1];
    struct pollfd mem_update_done_fds[1];

    uint32_t last_running_irq_count = 0;
    uint32_t last_done_irq_count = 0;
    bool irq_delta_baseline_valid = false;
    int32_t irq_delta_baseline = 0;
    bool irq_delta_last_observed_valid = false;
    int32_t irq_delta_last_observed = 0;
    uint64_t irq_delta_change_events = 0;
    uint32_t prev_running_irq_count = 0;
    uint32_t prev_done_irq_count = 0;
    bool irq_prev_counts_valid = false;
    uint64_t irq_increment_anomaly_events = 0;
    bool last_irq_increment_anomaly = false;
    uint32_t last_cycle_run_delta = 0;
    uint32_t last_cycle_done_delta = 0;
    
    void cache_flush(void* addr, uint32_t size);
    void cache_invalidate(void* addr, uint32_t size);

    void feed_watchdog();   // feed the watchdog to prevent the FPGA DMA from shutting down

    void error_if_nullptr();    // check to ensure memory is allocated before returning any pointers
};
