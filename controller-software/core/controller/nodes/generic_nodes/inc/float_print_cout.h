#include "node_factory.h"
#include <iostream>

#pragma once


class float_print_cout: public base_node {
    private:
        float default_input_value = 0.0;
        float output_value = 0.0;

        input* input_value = nullptr;
    public:

        float_print_cout(){
            inputs.emplace("input", input(io_type::FLOAT, &default_input_value));
            
            input_value = &inputs["input"];
        }

        unsigned int run() override {
            output_value = (*reinterpret_cast<float*>(input_value->data_pointer));
            std::cout << "float output: " << output_value << std::endl;
            return 0;
        }

};

