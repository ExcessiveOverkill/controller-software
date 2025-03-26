#include "node_factory.h"
#include <iostream>

#pragma once


class uint16_print_cout: public base_node {
    private:
        uint16_t default_input_value = 0.0;
        uint16_t output_value = 0.0;

        input* input_value = nullptr;
    public:

    uint16_print_cout(){
            inputs.emplace("input", input(io_type::UINT16, &default_input_value));
            
            input_value = &inputs["input"];
        }

        unsigned int run() override {
            output_value = (*reinterpret_cast<uint16_t*>(input_value->data_pointer));
            std::cout << "uint16 output: " << output_value << std::endl;
            return 0;
        }

};

