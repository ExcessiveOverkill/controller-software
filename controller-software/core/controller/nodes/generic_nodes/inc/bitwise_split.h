#include "node_factory.h"

#pragma once


class bitwise_split: public base_node {
    private:
        bool output_bits[32] = {false};
        uint8_t bit_count = 0;
        input* in = nullptr;

        io_type type = io_type::UNDEFINED;

        bool configured = false;
        
    public:

        bitwise_split(){
        }

        unsigned int run() override {

            for(uint8_t i = 0; i < bit_count; i++){
                output_bits[i] = (*(uint32_t*)(in->data_pointer) >> i) & 1;
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
                std::cerr << "Error: bitwise_split node already configured" << std::endl;
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
                    std::cerr << "Error: invalid type for bitwise_split node" << std::endl;
                    return 1;
            }

            inputs.emplace("input", input(type, nullptr));
            in = &inputs["input"];

            for(uint8_t i = 0; i < bit_count; i++){
                outputs.emplace("output_bit_" + std::to_string(i), output(io_type::BOOL, &output_bits[i], &execution_number));
            }

            configured = true;
            
            return 0;
        }

};