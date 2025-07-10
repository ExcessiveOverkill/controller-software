#include "node_factory.h"

#pragma once


class print: public base_node {
    private:
        input* in = nullptr;
        input* enable_in = nullptr; // rising edge bool to trigger the print
        std::string message = "";

        io_type type = io_type::UNDEFINED; // type of the data
        
        bool last_enable_in = false;

        bool configured = false;
        
    public:

        print(){
        }

        unsigned int run() override {

            bool enable_input_value = *(bool*)enable_in->data_pointer;

            if(enable_input_value && !last_enable_in){
                // rising edge detected
                if(type == io_type::BOOL){
                    std::cout << message << ": " << *(bool*)in->data_pointer << std::endl;
                }
                else if(type == io_type::UINT8){
                    std::cout << message << ": " << *(uint8_t*)in->data_pointer << std::endl;
                }
                else if(type == io_type::INT8){
                    std::cout << message << ": " << *(int8_t*)in->data_pointer << std::endl;
                }
                else if(type == io_type::UINT16){
                    std::cout << message << ": " << *(uint16_t*)in->data_pointer << std::endl;
                }
                else if(type == io_type::INT16){
                    std::cout << message << ": " << *(int16_t*)in->data_pointer << std::endl;
                }
                else if(type == io_type::UINT32){
                    std::cout << message << ": " << *(uint32_t*)in->data_pointer << std::endl;
                }
                else if(type == io_type::INT32){
                    std::cout << message << ": " << *(int32_t*)in->data_pointer << std::endl;
                }
                else if(type == io_type::DOUBLE){
                    std::cout << message << ": " << *(double*)in->data_pointer << std::endl;
                }
                else if(type == io_type::FLOAT){
                    std::cout << message << ": " << *(float*)in->data_pointer << std::endl;
                }
            }
            last_enable_in = enable_input_value;
            
            
            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "type": "uint8",
                    "name": "My Print Node"
                    }
            }
            */

            if(configured){
                std::cerr << "Error: cycle_delay node already configured" << std::endl;
                return 1;
            }

            
            type = get_io_type(json->at("type").get<std::string>());
            if(type == io_type::UNDEFINED){
                std::cerr << "Error: cycle_delay node type is undefined" << std::endl;
                return 1;
            }
            message = json->at("name").get<std::string>();

            inputs.emplace("input", input(type, nullptr));
            inputs.emplace("enable", input(io_type::BOOL, nullptr));
            in = &inputs["input"];
            enable_in = &inputs["enable"];

            configured = true;
            
            return 0;
        }

};