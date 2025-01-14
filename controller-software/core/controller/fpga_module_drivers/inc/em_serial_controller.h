#include "fpga_module_driver_factory.h"
#include "register_helper.h"


#pragma once

class em_serial_device{
public:
    em_serial_device(Address_Map_Loader* loader, uint8_t device_number);

    uint32_t set_address(uint32_t address);
    uint32_t get_address(uint16_t* address);
    uint32_t set_enabled(bool enabled);
    uint32_t get_enabled(bool* enabled);
    uint32_t set_cyclic_data_enabled(bool enabled);
    bool get_cyclic_data_enabled();
    bool get_sequential_cmds_complete();
    bool get_cyclic_config_complete();


    // "address" in below functions is the device's serial register address
    uint32_t sequential_write(uint16_t address, void* data, uint8_t size);
    uint32_t sequential_read(uint16_t address, void* data, uint8_t size);

    uint32_t configure_cyclic_read(uint16_t address, uint8_t size);
    //uint32_t disable_cyclic_read(uint16_t address); // TODO: implement

    uint32_t configure_cyclic_write(uint16_t address, uint8_t size);
    //uint32_t disable_cyclic_write(uint16_t address); // TODO: implement

    uint32_t get_cyclic_read_data_mem_address(uint16_t address, uint8_t* node_address, uint16_t* bram_address);    // get node and bram address for cyclic data, used for DMA instructions
    uint32_t get_cyclic_write_data_mem_address(uint16_t address, uint8_t* node_address, uint16_t* bram_address);    // get node and bram address for cyclic data, used for DMA instructions

    uint32_t set_cyclic_read_pointer(uint16_t address, void** data); // automatically move cyclic data to the pointer
    uint32_t set_cyclic_write_pointer(uint16_t address, void** data); // automatically move pointer data to the cyclic location

    uint32_t run(); // run the device, must be called at each FPGA update



    // TEMPORARY: for testing
    void* test_read = nullptr;


private:
    Address_Map_Loader* loader = nullptr;

    struct sequential_cmd{
        uint16_t address;   // serial register address to read/write
        uint32_t data = 0;  // data to write or read into
        uint8_t size;   // size of the data in bytes
        bool write;
        bool* complete_flag = nullptr;  // use this to let external code know when the command is complete
        bool* complete_flag_inverted = nullptr;  // use this to let external code know when the command is complete
    };
    const uint8_t max_outstanding_cmds = 64;    // TODO: get this from some config?
    std::vector<sequential_cmd> sequential_cmds;

    #define MAX_CYCLIC_REGS 64  // TODO: get this from some config
    uint16_t max_cyclic_registers = MAX_CYCLIC_REGS;
    bool cyclic_read_enabled[MAX_CYCLIC_REGS];
    bool cyclic_write_enabled[MAX_CYCLIC_REGS];

    struct controller_cyclic_config{   // configure a cyclic read or write

        // fpga module configs
        uint8_t size = 0;
        uint8_t offset = 0;

        // device configs
        uint16_t address = 0;

        // controller configs
        uint16_t packet_data_start_byte = 0;    // where the data starts in the packet (absolute byte index)
        uint16_t packet_data_end_byte = 0;  // where the data ends in the packet (absolute byte index)
        Register* cyclic_data_register = nullptr;    // the register that holds the cyclic data
    };

    std::vector<controller_cyclic_config> read_cyclic_configs;
    std::vector<controller_cyclic_config> write_cyclic_configs;

    struct raw_cyclic_config{
        uint8_t read_size = 0;
        uint8_t write_size = 0;
        uint8_t read_offset = 0;
        uint8_t write_offset = 0;
    }
    raw_cyclic_configs[MAX_CYCLIC_REGS];
    std::vector<uint8_t> outstanding_cyclic_configs;

    uint32_t address = 0;
    bool enabled = false;
    bool cyclic_data_enabled = false;
    bool cyclic_config_complete = true;
    bool sequential_cmds_complete = true;
    bool initial_config_complete = false;

    struct registers{   // registers needed to control and configure the device
        Register* control = nullptr;
        Register* control_enable = nullptr;
        Register* control_enable_cyclic_data = nullptr;
        Register* control_rx_cyclic_packet_size = nullptr;

        Register* status = nullptr;
        Register* status_no_response = nullptr;
        Register* status_response_not_finished = nullptr;
        Register* status_crc_invalid = nullptr;

        Register* cyclic_configs[MAX_CYCLIC_REGS];
        Register* cyclic_reads[MAX_CYCLIC_REGS];
        Register* cyclic_writes[MAX_CYCLIC_REGS];

        Dynamic_Register* cyclic_config;
        Register* cyclic_config_reg = nullptr;    // from the dynamic register
        Register* cyclic_config_read_size = nullptr;
        Register* cyclic_config_write_size = nullptr;
        Register* cyclic_config_read_offset = nullptr;
        Register* cyclic_config_write_offset = nullptr;

        // TODO: instead of saving every register object, maybe just save the PL addresses to save space?

        
    } regs;

    struct device_info{
        uint16_t hardware_type_addr = 0;
        uint16_t hardware_version_addr = 1;
        uint16_t firmware_version_addr = 2;
        uint16_t enable_cyclic_data_addr = 0xffff;
        uint16_t cyclic_read_address_0_addr = 0xffff;
        uint16_t cyclic_write_address_0_addr = 0xffff;

        uint16_t cyclic_addresses = 0;
        
        uint32_t hardware_type = 0;
        uint32_t hardware_version = 0;
        uint32_t firmware_version = 0;
    } dev_info;

    uint32_t add_sequential_cmd(uint16_t address, void* data, uint8_t size, bool write, bool* complete_flag, bool* complete_flag_inverted);

    struct faults{
        bool no_response = false;
        bool response_not_finished = false;
        bool crc_invalid = false;
    } fault;
    uint32_t consecutive_packet_errors = 0;
    uint32_t consecutive_unknown_packet_errors = 0;

    uint32_t run_sequential_cmds();
    uint32_t run_cylic_config();
    
};

class em_serial_controller : public base_driver {
public:
    uint32_t load_config(json config, std::vector<uint64_t>* instructions) override;

    uint32_t run() override;

    uint32_t configure_baud_rate(uint32_t baud_rate);   // all devices on port, minimum 115200
    uint32_t enable_transfers(bool enable); // enable/disable transfers for all enabled devices
    uint32_t autoconfigure_devices();    // autoconfigure all devices on the port

    //int32_t* cmd_q_current_milliamps = nullptr;

private:

    Register* control = nullptr;
    Register* start_transfers = nullptr;
    Register* bit_length = nullptr;
    Register* status = nullptr;
    Register* status_update_busy = nullptr;
    Register* status_update_done = nullptr;
    Register* status_update_error = nullptr;

    std::vector<em_serial_device*> devices;

};