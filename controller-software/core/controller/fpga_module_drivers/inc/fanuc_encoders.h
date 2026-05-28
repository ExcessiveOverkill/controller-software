#include "fpga_module_driver_factory.h"
//#include "register_helper.h"


#pragma once

class fanuc_encoders : public base_driver {
public:    
    uint32_t custom_load_config(json* user_driver_config) override;

    uint32_t run() override;

private:

    Register* trigger = nullptr;

    const uint64_t singleturn_res = 1ULL << 32;
    const uint64_t multiturn_res = 1ULL << 16; // only 16 bits for multiturn count
    const uint64_t half_multiturn_count = multiturn_res >> 1;

    struct encoder_data{
        uint32_t multiturn_count = 0;
        uint32_t singleturn_count = 0;
        uint16_t commutation_angle = 0;
        bool battery_fail = false;
        bool unindexed = false;
        bool no_response = false;
        bool crc_error = false;
        bool done = false;

        double total_turns = 0.0; // calculated from multiturn and singleturn counts
        bool turn_wrap = false; // flag if multiturn count is ambiguous

        Register* multiturn_count_reg = nullptr;
        Register* singleturn_count_reg = nullptr;
        Register* commutation_count_reg = nullptr;

        Register* status_reg = nullptr;
        Register* crc_error_reg = nullptr;
        Register* no_response_reg = nullptr;
        Register* unindexed_reg = nullptr;
        Register* battery_fail_reg = nullptr;
        Register* done_reg = nullptr;

        uint8_t rs485_mode = 0;   // 0 = disabled, 1 = enabled
        uint8_t encoder_type = 0;

        Register* config_reg = nullptr;
        Register* rs485_mode_reg = nullptr;
        Register* encoder_type_reg = nullptr;

    };

    void convert_turns(encoder_data* data);
    std::vector<encoder_data> encoders;

};