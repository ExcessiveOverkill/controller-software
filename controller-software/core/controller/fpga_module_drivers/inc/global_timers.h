#include "fpga_module_driver_factory.h"
#include "register_helper.h"


#pragma once

class global_timers : public base_driver {
public:    
    uint32_t load_config(json config, std::vector<uint64_t>* instructions) override;

    uint32_t run() override;

private:

    std::vector<Register*> timer_values;

};