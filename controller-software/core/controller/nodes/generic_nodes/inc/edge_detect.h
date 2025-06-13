#include "node_factory.h"

#pragma once


class edge_detect: public base_node {
    private:
        bool out = false; // output value
        bool falling_edge = false; // true if the edge is falling, false if rising
        input* in = nullptr; // pointer to the input

        bool last_in = false;

        bool configured = false;
        
    public:

        edge_detect(){
        }

        unsigned int run() override {

            bool in_value = *(bool*)(in->data_pointer);

            if(in_value != last_in){
                // edge detected
                if(falling_edge){
                    out = !in_value; // output true if falling edge
                }
                else{
                    out = in_value; // output true if rising edge
                }
            }
            else{
                out = false; // no edge detected
            }
            last_in = in_value;
            
            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "rising_edge": "true",
                    "falling_edge": "false",
                    }
            }
            */

            if(configured){
                std::cerr << "Error: edge_delay node already configured" << std::endl;
                return 1;
            }

            if(json->find("rising_edge") != json->end()){
                falling_edge = !json->at("rising_edge").get<bool>();
            }
            else if(json->find("falling_edge") != json->end()){
                falling_edge = json->at("falling_edge").get<bool>();
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