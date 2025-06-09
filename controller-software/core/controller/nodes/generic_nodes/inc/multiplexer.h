#include "node_factory.h"

#pragma once


class multiplexer: public base_node {
    private:
        void* data_ptr = nullptr; // pointer to the data that will be used as output
        input* input_bits[8] = {nullptr};
        input* input_data[256] = {nullptr};
        uint8_t input_count = 0;
        uint8_t input_bit_count = 0;

        io_type type = io_type::UNDEFINED;

        bool configured = false;
        
    public:

        multiplexer(){
        }

        unsigned int run() override {
            uint8_t selected_index = 0;

            for(uint8_t i = 0; i < input_bit_count; i++){
                if(*(bool*)input_bits[i]->data_pointer){
                    selected_index |= (1 << i);
                }
            }

            void* selected_data = input_data[selected_index]->data_pointer;

            switch(type){
                case io_type::UINT8:
                    *(uint8_t*)data_ptr = *(uint8_t*)selected_data;
                    break;
                case io_type::INT8:
                    *(int8_t*)data_ptr = *(int8_t*)selected_data;
                    break;
                case io_type::UINT16:
                    *(uint16_t*)data_ptr = *(uint16_t*)selected_data;
                    break;
                case io_type::INT16:
                    *(int16_t*)data_ptr = *(int16_t*)selected_data;
                    break;
                case io_type::UINT32:
                    *(uint32_t*)data_ptr = *(uint32_t*)selected_data;
                    break;
                case io_type::INT32:
                    *(int32_t*)data_ptr = *(int32_t*)selected_data;
                    break;
                case io_type::FLOAT:
                    *(float*)data_ptr = *(float*)selected_data;
                    break;
                case io_type::DOUBLE:
                    *(double*)data_ptr = *(double*)selected_data;
                    break;
                case io_type::BOOL:
                    *(bool*)data_ptr = *(bool*)selected_data;
                    break;
                default:
                    std::cerr << "Error: invalid type for multiplexer node" << std::endl;
                    return 1;   // invalid output type
            }

            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "type": "uint8",
                    "input_bits": 8
                }
            }
            */

            if(configured){
                std::cerr << "Error: multiplexer node already configured" << std::endl;
                return 1;
            }

            auto config = json->find("config");
            type = get_io_type(config->at("type").get<std::string>());
            if(type == io_type::UNDEFINED){
                std::cerr << "Error: invalid type for multiplexer node" << std::endl;
                return 1;
            }

            input_bit_count = config->at("input_bits").get<uint8_t>();
            if(input_bit_count < 1 || input_bit_count > 8){
                std::cerr << "Error: input_bits must be between 1 and 8" << std::endl;
                return 1;
            }
            input_count = pow(2, input_bit_count);

            switch(type){
                case io_type::UINT8:
                    data_ptr = new uint8_t(config->at("value").get<uint8_t>());
                    break;
                case io_type::INT8:
                    data_ptr = new int8_t(config->at("value").get<int8_t>());
                    break;
                case io_type::UINT16:
                    data_ptr = new uint16_t(config->at("value").get<uint16_t>());
                    break;
                case io_type::INT16:
                    data_ptr = new int16_t(config->at("value").get<int16_t>());
                    break;
                case io_type::UINT32:
                    data_ptr = new uint32_t(config->at("value").get<uint32_t>());
                    break;
                case io_type::INT32:
                    data_ptr = new int32_t(config->at("value").get<int32_t>());
                    break;
                case io_type::FLOAT:
                    data_ptr = new float(config->at("value").get<float>());
                    break;
                case io_type::DOUBLE:
                    data_ptr = new double(config->at("value").get<double>());
                    break;
                case io_type::BOOL:
                    data_ptr = new bool(config->at("value").get<bool>());
                    break;
                default:
                    std::cerr << "Error: invalid type for multiplexer node" << std::endl;
                    return 1;
            }

            outputs.emplace("output", output(type, data_ptr, &execution_number));

            // select inputs
            for(uint8_t i = 0; i < input_bit_count; i++){
                std::string input_name = "select_bit_" + std::to_string(i);
                inputs.emplace(input_name, input(io_type::BOOL, nullptr));
                input_bits[i] = &inputs[input_name];
            }

            // data inputs
            for(uint8_t i = 0; i < input_count; i++){
                std::string input_name = "input_" + std::to_string(i);
                inputs.emplace(input_name, input(type, nullptr));
                input_data[i] = &inputs[input_name];
            }

            configured = true;
            
            return 0;
        }

};