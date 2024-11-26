#include "fpga_module_driver_factory.h"


#pragma once

class serial_interface_card : public base_driver {
public:
    serial_interface_card();
    ~serial_interface_card();

    uint32_t load_config_pre(json config) override;
    uint32_t load_config_post(json config) override;
    uint32_t get_base_instructions(std::vector<uint64_t>* instructions) override;

    uint32_t run() override;

private:
    struct registers{
        uint32_t* port_mode_enable = nullptr;   // write
        uint32_t* i2c_config = nullptr; // write
        uint32_t* i2c_status = nullptr; // read
        uint32_t* i2c_data_rx = nullptr;    // read
        uint32_t* i2c_data_tx = nullptr;    // write

    } regs;

    struct i2c_transfer{
        uint8_t device_address = 0;
        uint8_t reg_address = 0;
        uint32_t* data = nullptr;
        uint8_t data_length = 0;
        bool read = false;
    };

    struct io_expander_data{
        uint16_t configuration = 0;
        uint16_t inputs = 0;
        uint16_t outputs = 0;
    };

    io_expander_data io_expander_data[3];  // data for the 3 io expanders

    enum port_mode : uint8_t {
        RS485 = 0,
        RS422 = 1,
        //QUADRATURE = 2 // not implemented yet
    };

    port_mode port_modes[10];   // current port modes for the 10 ports

    // TODO: make a separate class for i2c

    std::vector<i2c_transfer> i2c_transfers;    // list of i2c transfers to be done
    uint8_t last_i2c_transfer = 0;  // index of the last i2c transfer sent
    uint32_t i2c_consecutive_error_count = 0;

    void run_i2c_transfers();

    uint32_t configure_port_mode(uint8_t port, port_mode mode);

    enum led_mode : uint8_t {
        OFF = 0,
        ON = 1,
        BLINK_SLOW = 2,
        BLINK_MED = 3,
        BLINK_FAST = 4
    };

    led_mode led_modes[3];  // current led modes for the 3 leds

    bool power_out_5v5_1_ok = false;
    bool power_out_5v5_2_ok = false;
    bool power_out_24v_ok = false;
    bool power_out_enable = false;


};