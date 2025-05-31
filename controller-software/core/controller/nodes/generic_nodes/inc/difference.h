#include "node_factory.h"

#pragma once

class difference: public base_node {
    private:
        input* in_a = nullptr; // First input
        input* in_b = nullptr; // Second input

        io_type type = io_type::UNDEFINED; // Type of the inputs and output

        bool configured = false;

        void* out_data = nullptr;

    public:
        unsigned int run() override {
            
            if(!configured) {
                return 1; // Error
            }

            switch(type) {
                case io_type::UINT8:
                    *reinterpret_cast<uint8_t*>(out_data) = 
                        *reinterpret_cast<uint8_t*>(in_a->data_pointer) - 
                        *reinterpret_cast<uint8_t*>(in_b->data_pointer);
                    break;
                case io_type::UINT16:
                    *reinterpret_cast<uint16_t*>(out_data) = 
                        *reinterpret_cast<uint16_t*>(in_a->data_pointer) - 
                        *reinterpret_cast<uint16_t*>(in_b->data_pointer);
                    break;
                case io_type::UINT32:
                    *reinterpret_cast<uint32_t*>(out_data) = 
                        *reinterpret_cast<uint32_t*>(in_a->data_pointer) - 
                        *reinterpret_cast<uint32_t*>(in_b->data_pointer);
                    break;
                case io_type::INT8:
                    *reinterpret_cast<int8_t*>(out_data) = 
                        *reinterpret_cast<int8_t*>(in_a->data_pointer) - 
                        *reinterpret_cast<int8_t*>(in_b->data_pointer);
                    break;
                case io_type::INT16:
                    *reinterpret_cast<int16_t*>(out_data) = 
                        *reinterpret_cast<int16_t*>(in_a->data_pointer) - 
                        *reinterpret_cast<int16_t*>(in_b->data_pointer);
                    break;
                case io_type::INT32:
                    *reinterpret_cast<int32_t*>(out_data) = 
                        *reinterpret_cast<int32_t*>(in_a->data_pointer) - 
                        *reinterpret_cast<int32_t*>(in_b->data_pointer);
                    break;
                case io_type::FLOAT:
                    *reinterpret_cast<float*>(out_data) = 
                        *reinterpret_cast<float*>(in_a->data_pointer) - 
                        *reinterpret_cast<float*>(in_b->data_pointer);
                    break;
                case io_type::DOUBLE:
                    *reinterpret_cast<double*>(out_data) = 
                        *reinterpret_cast<double*>(in_a->data_pointer) - 
                        *reinterpret_cast<double*>(in_b->data_pointer);
                    break;
                default:
                    return 2; // Unsupported type
            }

            return 0; // Success
        }

        uint32_t configure_settings(json* data) override {

            /*
            parse settings from json
            
            {
                "config":
                {
                    "type": "double" // Type of the inputs and output
                }

            }
            
            */

            if(configured) {
                std::cerr << "Error: already configured" << std::endl;
                return 1; // Already configured
            }

            if(data->find("config") == data->end()) {
                std::cerr << "Error: config not found" << std::endl;
                return 1; // Config not found
            }

            auto config = data->at("config");

            if(config.find("type") == config.end()) {
                std::cerr << "Error: type not found in config" << std::endl;
                return 2; // Type not found
            }

            type = get_io_type(config["type"].get<std::string>());
            if(type == io_type::UNDEFINED) {
                std::cerr << "Error: invalid type specified" << std::endl;
                return 3; // Invalid type
            }

            switch (type)
            {
            case io_type::BOOL:
                out_data = new bool(false);
                break;
            case io_type::UINT8:
                out_data = new uint8_t(0);
                break;
            case io_type::UINT16:
                out_data = new uint16_t(0);
                break;
            case io_type::UINT32:
                out_data = new uint32_t(0);
                break;
            case io_type::INT8:
                out_data = new int8_t(0);
                break;
            case io_type::INT16:
                out_data = new int16_t(0);
                break;
            case io_type::INT32:
                out_data = new int32_t(0);
                break;
            case io_type::FLOAT:
                out_data = new float(0.0);
                break;
            case io_type::DOUBLE:
                out_data = new double(0.0);
                break;
            default:
                std::cerr << "Error: unsupported type for difference node" << std::endl;
                return 4; // Unsupported type
            }


            inputs.emplace("A", input(type, nullptr));
            inputs.emplace("B", input(type, nullptr));
            outputs.emplace("output", output(type, out_data, &execution_number));

            in_a = &inputs["A"];
            in_b = &inputs["B"];

            configured = true;
            return 0; // Success
        }
};