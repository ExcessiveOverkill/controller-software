#include "node_factory.h"

#pragma once


class bitwise_join: public base_node {
    private:
        uint32_t out = 0;
        input* in_bits[32] = {nullptr}; // pointers to input bits
        uint8_t bit_count = 0;

        io_type type = io_type::UNDEFINED;

        bool configured = false;
        
    public:

        bitwise_join(){
        }

        unsigned int run() override {

            out = 0;

            for(uint8_t i = 0; i < bit_count; i++){
                out |= (*(bool*)(in_bits[i]->data_pointer) << i);
            }
            
            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "type": "uint8" // only unsigned types supported
                    }
            }
            */

            if(configured){
                std::cerr << "Error: bitwise_join node already configured" << std::endl;
                return 1;
            }

            auto config = json->find("config");
            type = get_io_type(config->at("type").get<std::string>());

            switch(type){
                case io_type::UINT8:
                    bit_count = 8;
                    break;
                case io_type::UINT16:
                    bit_count = 16;
                    break;
                case io_type::UINT32:
                    bit_count = 32;
                    break;
                default:
                    std::cerr << "Error: invalid type for bitwise_join node" << std::endl;
                    return 1;
            }

            outputs.emplace("output", output(type, &out, &execution_number));

            for(uint8_t i = 0; i < bit_count; i++){
                inputs.emplace("input_bit_" + std::to_string(i), input(io_type::BOOL, nullptr));
                in_bits[i] = &inputs["input_bit_" + std::to_string(i)];
            }

            configured = true;
            
            return 0;
        }

};