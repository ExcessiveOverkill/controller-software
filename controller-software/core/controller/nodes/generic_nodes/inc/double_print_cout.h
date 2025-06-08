#include "node_factory.h"
#include <iostream>

#pragma once


class double_print_cout: public base_node {
    private:
        double default_input_value = 0.0;
        double output_value = 0.0;

        uint32_t update_cycles = 1;
        uint32_t update_counter = 0;

        std::string name = "";

        input* input_value = nullptr;
    public:

        double_print_cout(){
            inputs.emplace("input", input(io_type::DOUBLE, &default_input_value));
            
            input_value = &inputs["input"];
        }

        unsigned int run() override {

            if(update_counter < update_cycles) {
                update_counter++;
                return 0; // skip this run
            }
            update_counter = 0; // reset counter
            output_value = (*reinterpret_cast<double*>(input_value->data_pointer));
            std::cout << name << ": " << output_value << std::endl;
            return 0;
        }

        uint32_t configure_settings(json* data) override {

            /*
            parse settings from json
            
            {
                "name": "name to print",
                "update_rate_cycles": 10,
            }
            
            */

            if(data->find("name") == data->end()) {
                std::cerr << "Error: name not found" << std::endl;
                return 1;
            }

            name = data->at("name");

            if(data->find("update_rate_cycles") != data->end()) {
                update_cycles = data->at("update_rate_cycles").get<uint32_t>();
            }
        
            return 0;
        }

};

