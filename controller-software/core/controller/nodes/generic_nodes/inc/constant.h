#include "node_factory.h"

#pragma once

// constants cannot be changed once configured

class constant: public base_node {
    private:
        void* data_ptr = nullptr; // pointer to the data that will be used as output
        output* output_ptr = nullptr; // pointer to the output object
        
    public:

        constant(){
            execution_number = -1;
        }

        unsigned int run() override {
            // nothing to do
            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "type": "uint8",
                    "value": -1.0,
                        
                }
            }
            */

            if(data_ptr != nullptr){
                std::cerr << "Error: constant node already configured" << std::endl;
                return 1;
            }

            io_type type = get_io_type(json->at("type").get<std::string>());
            if(type == io_type::UNDEFINED){
                std::cerr << "Error: invalid type for constant node" << std::endl;
                return 1;
            }

            switch(type){
                case io_type::UINT8:
                    data_ptr = new uint8_t(json->at("value").get<uint8_t>());
                    break;
                case io_type::INT8:
                    data_ptr = new int8_t(json->at("value").get<int8_t>());
                    break;
                case io_type::UINT16:
                    data_ptr = new uint16_t(json->at("value").get<uint16_t>());
                    break;
                case io_type::INT16:
                    data_ptr = new int16_t(json->at("value").get<int16_t>());
                    break;
                case io_type::UINT32:
                    data_ptr = new uint32_t(json->at("value").get<uint32_t>());
                    break;
                case io_type::INT32:
                    data_ptr = new int32_t(json->at("value").get<int32_t>());
                    break;
                case io_type::FLOAT:
                    data_ptr = new float(json->at("value").get<float>());
                    break;
                case io_type::DOUBLE:
                    data_ptr = new double(json->at("value").get<double>());
                    break;
                case io_type::BOOL:
                    data_ptr = new bool(json->at("value").get<bool>());
                    break;
                default:
                    std::cerr << "Error: invalid type for constant node" << std::endl;
                    return 1;
            }

            outputs.emplace("output", output(type, data_ptr, &execution_number));
            
            return 0;
        }

};