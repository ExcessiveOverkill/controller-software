#include "node_factory.h"
#include <iostream>

#pragma once


class convert_float_to_bool: public base_node {
    private:
        float default_input_value = 0.0;
        bool output_value = false;

        input* input_value_ptr = nullptr;
    public:

        convert_float_to_bool(){
            inputs.emplace("input", input(io_type::FLOAT, &default_input_value));
            outputs.emplace("output", output(io_type::BOOL, &output_value, &execution_number));
            
            input_value_ptr = &inputs["input"];
        }

        unsigned int run() override {
            float input_value = (*reinterpret_cast<float*>(input_value_ptr->data_pointer));
            output_value = input_value > 0.0 ? true : false;
            return 0;
        }

};

