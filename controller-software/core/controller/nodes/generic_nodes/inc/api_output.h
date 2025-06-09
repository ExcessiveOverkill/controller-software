#include "node_factory.h"
#include <chrono>

/*

Allows API commands to be sent to the controller to set signal values
values are clamped to the min/max settings
if a timeout value is set, the value will be reset to the configured default value (0 by default) after the timeout period

*/

#pragma once

class api_output: public base_node {
    private:

        input* input_ptr = nullptr; // pointer to the input object

        io_type type = io_type::UNDEFINED;  // input type

        bool configured = false; // true if the node has been configured

    public:

        api_output(){
        }

        unsigned int run() override {
            // nothing to do
            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":
                {
                    "type": "uint8"
                },
                "get": 0    // adding the get key will trigger the retrieval of the value from the input
            }
            */

            

            if(json->find("config") != json->end()){    // config mode
                if(configured){
                    std::cerr << "Error: api_input node already configured" << std::endl;
                    return 1;
                }
                auto c = (*json)["config"];
                type = get_io_type(c["type"].get<std::string>());
                if(type == io_type::UNDEFINED || type == io_type::EM_SERIAL_DEVICE){
                    std::cerr << "Error: invalid type for api_output node" << std::endl;
                    return 1;
                }

                inputs.emplace("input", input(type, nullptr));
                input_ptr = &inputs["input"]; // save the input pointer for later use

                configured = true;
            }

            if(!configured){
                std::cerr << "Error: api_input node not configured" << std::endl;
                return 1;
            }

            if(json->find("get") != json->end()){    // get the value from the input
                switch(type){
                    case io_type::UINT8:
                        (*json)["get"] = *(uint8_t*)input_ptr->data_pointer;
                        break;
                    case io_type::INT8:
                        (*json)["get"] = *(int8_t*)input_ptr->data_pointer;
                        break;
                    case io_type::UINT16:
                        (*json)["get"] = *(uint16_t*)input_ptr->data_pointer;
                        break;
                    case io_type::INT16:
                        (*json)["get"] = *(int16_t*)input_ptr->data_pointer;
                        break;
                    case io_type::UINT32:
                        (*json)["get"] = *(uint32_t*)input_ptr->data_pointer;
                        break;
                    case io_type::INT32:
                        (*json)["get"] = *(int32_t*)input_ptr->data_pointer;
                        break;
                    case io_type::FLOAT:
                        (*json)["get"] = *(float*)input_ptr->data_pointer;
                    case io_type::DOUBLE:
                        (*json)["get"] = *(double*)input_ptr->data_pointer;
                        break;
                    case io_type::BOOL:
                        (*json)["get"] = *(bool*)input_ptr->data_pointer;
                        break;
                    default:
                        std::cerr << "Error: invalid type for api_output node" << std::endl;

                }
            }
            
            return 0;
        }

};