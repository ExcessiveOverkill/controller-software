#include "node_factory.h"

#pragma once


class edge_delay: public base_node {
    private:
        bool out = false; // output value
        bool falling_edge = false; // true if the edge is falling, false if rising
        input* in = nullptr; // pointer to the input

        // times are in execution cycles
        uint32_t cycles = 0;
        uint32_t elapsed_cycles = 0; // cycles since the last edge

        bool configured = false;
        
    public:

        edge_delay(){
        }

        unsigned int run() override {

            bool in_value = *(bool*)(in->data_pointer);

            if(falling_edge){
                if(in_value){
                    elapsed_cycles = 0;
                    out = true;
                }
                else if (elapsed_cycles < cycles){
                    elapsed_cycles++;
                }
                else{
                    out = false;
                }
            }
            else{   // rising edge
                if(!in_value){
                    elapsed_cycles = 0;
                    out = false;
                }
                else if (elapsed_cycles < cycles){
                    elapsed_cycles++;
                }
                else{
                    out = true;
                }
            }
            
            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "rising_edge": "true",
                    "falling_edge": "false",
                    "cycles": 10
                    }
            }
            */

            if(configured){
                std::cerr << "Error: edge_delay node already configured" << std::endl;
                return 1;
            }

            auto config = json->find("config");
            if(config->find("rising_edge") != config->end()){
                falling_edge = !config->at("rising_edge").get<bool>();
            }
            else if(config->find("falling_edge") != config->end()){
                falling_edge = config->at("falling_edge").get<bool>();
            }
            else{
                std::cerr << "Error: edge_delay node configuration missing 'rising_edge' or 'falling_edge'" << std::endl;
                return 1;
            }


            outputs.emplace("output", output(io_type::BOOL, &out, &execution_number));

            inputs.emplace("input", input(io_type::BOOL, nullptr));
            in = &inputs["input"];

            configured = true;
            
            return 0;
        }

};