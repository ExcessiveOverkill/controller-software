#include "fpga_module_driver_factory.h"
#include "register_helper.h"


#pragma once

class fanuc_encoders : public base_driver {
public:    
    uint32_t load_config(json config, std::vector<uint64_t>* instructions) override;

    uint32_t run() override;

private:

    Register* multiturn_count = nullptr;
    Register* singleturn_count = nullptr;
    Register* commutation_count = nullptr;
    Register* crc_error = nullptr;
    Register* no_response = nullptr;
    Register* unindexed = nullptr;
    Register* battery_fail = nullptr;
    Register* done = nullptr;

    struct encoder_data{
        uint32_t multiturn_count = 0;
        uint32_t singleturn_count = 0;
        uint16_t commutation_angle = 0;
        bool battery_fail = false;
        bool unindexed = false;
        bool no_resonse = false;
        bool crc_error = false;
        bool done = false;
    };

    uint32_t old_microseconds = 0;

    std::vector<encoder_data> encoders;

};