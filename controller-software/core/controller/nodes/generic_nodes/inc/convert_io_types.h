#include "node_factory.h"
#include <iostream>

#pragma once

/*
Convert a value to another type
all conversions pass through a double type
*/


class convert_io_types: public base_node {
    private:
        struct converter{
            std::string name;
            input* in = nullptr;
            output* out = nullptr;

            double input_max = 0.0;
            double input_min = 0.0;

            double output_max = 0.0;
            double output_min = 0.0;

            bool invert = false;

            void* out_value_ptr = nullptr;

        };

        std::vector<converter> converters;

        std::vector<io_type> supported_types = {
            io_type::BOOL,
            io_type::UINT8,
            io_type::UINT16,
            io_type::UINT32,
            io_type::INT8,
            io_type::INT16,
            io_type::INT32,
            io_type::FLOAT,
            io_type::DOUBLE
        };

        
    public:

        convert_io_types(){
            
        }

        uint32_t run() override {
            
            // run all conversions
            for(auto& c : converters){

                // convert all input values to a double
                double in_value;
                switch(c.in->get_type()){
                    case io_type::BOOL:
                        in_value = *reinterpret_cast<bool*>(c.in->data_pointer) ? 1.0 : 0.0;
                        break;
                    case io_type::UINT8:
                        in_value = *reinterpret_cast<uint8_t*>(c.in->data_pointer);
                        break;
                    case io_type::UINT16:
                        in_value = *reinterpret_cast<uint16_t*>(c.in->data_pointer);
                        break;
                    case io_type::UINT32:
                        in_value = *reinterpret_cast<uint32_t*>(c.in->data_pointer);
                        break;
                    case io_type::INT8:
                        in_value = *reinterpret_cast<int8_t*>(c.in->data_pointer);
                        break;
                    case io_type::INT16:
                        in_value = *reinterpret_cast<int16_t*>(c.in->data_pointer);
                        break;
                    case io_type::INT32:
                        in_value = *reinterpret_cast<int32_t*>(c.in->data_pointer);
                        break;
                    case io_type::FLOAT:
                        in_value = *reinterpret_cast<float*>(c.in->data_pointer);
                        break;
                    case io_type::DOUBLE:
                        in_value = *reinterpret_cast<double*>(c.in->data_pointer);
                        break;
                    default:
                        // not supported
                        break;
                }

                // clamp to input range
                if(in_value < c.input_min){
                    in_value = c.input_min;
                }
                if(in_value > c.input_max){
                    in_value = c.input_max;
                }

                // scale to output range
                double out_value = (in_value - c.input_min) / (c.input_max - c.input_min) * (c.output_max - c.output_min) + c.output_min;
                if(c.invert){
                    out_value = c.output_max - out_value;
                }

                // clamp to output range (just in case)
                if(out_value < c.output_min){
                    out_value = c.output_min;
                }
                if(out_value > c.output_max){
                    out_value = c.output_max;
                }

                // set output value
                switch(c.out->get_type()){
                    case io_type::BOOL:
                        *reinterpret_cast<bool*>(c.out->data_pointer) = out_value >= 0.5 ? true : false;
                        break;
                    case io_type::UINT8:
                        *reinterpret_cast<uint8_t*>(c.out->data_pointer) = static_cast<uint8_t>(round(out_value));
                        break;
                    case io_type::UINT16:
                        *reinterpret_cast<uint16_t*>(c.out->data_pointer) = static_cast<uint16_t>(round(out_value));
                        break;
                    case io_type::UINT32:
                        *reinterpret_cast<uint32_t*>(c.out->data_pointer) = static_cast<uint32_t>(round(out_value));
                        break;
                    case io_type::INT8:
                        *reinterpret_cast<int8_t*>(c.out->data_pointer) = static_cast<int8_t>(round(out_value));
                        break;
                    case io_type::INT16:
                        *reinterpret_cast<int16_t*>(c.out->data_pointer) = static_cast<int16_t>(round(out_value));
                        break;
                    case io_type::INT32:
                        *reinterpret_cast<int32_t*>(c.out->data_pointer) = static_cast<int32_t>(round(out_value));
                        break;
                    case io_type::FLOAT:
                        *reinterpret_cast<float*>(c.out->data_pointer) = static_cast<float>(out_value);
                        break;
                    case io_type::DOUBLE:
                        *reinterpret_cast<double*>(c.out->data_pointer) = out_value;
                        break;
                    default:
                        // not supported
                        break;
                }
            }

            return 0;
        }

        uint32_t configure_settings(json* json) override {

            /*
            parse settings from json
            {
                "config": [
                    {
                        "name": "conversion name",  // input/output will be named this
                        "input_type": "float",
                        "output_type": "uint32",
                        "input_min": 0.0,
                        "input_max": 1.0,
                        "output_min": 0.0,
                        "output_max": 1.0,
                        "invert": false,
                    }
                ],

            }
            */

            for (auto& o : (*json)["config"]) {
                converter c;
                c.name = o["name"];
                c.input_min = o["input_min"];
                c.input_max = o["input_max"];
                c.output_min = o["output_min"];
                c.output_max = o["output_max"];
                c.invert = o["invert"];

                io_type input_type = get_io_type(o["input_type"].get<std::string>());
                io_type output_type = get_io_type(o["output_type"].get<std::string>());

                // some input validation

                if(inputs.find(c.name) != inputs.end()){
                    std::cerr << "Error: input name already exists" << std::endl;
                    return 1;
                }
                if(outputs.find(c.name) != outputs.end()){
                    std::cerr << "Error: output name already exists" << std::endl;
                    return 1;
                }


                if(c.input_max <= c.input_min){
                    std::cerr << "Error: input_max must be greater than input_min" << std::endl;
                    return 1;
                }
                if(c.output_max <= c.output_min){
                    std::cerr << "Error: output_max must be greater than output_min" << std::endl;
                    return 1;
                }

                if(std::find(supported_types.begin(), supported_types.end(), input_type) == supported_types.end()){
                    std::cerr << "Error: unsupported input type for conversion" << std::endl;
                    return 1;
                }
                if(std::find(supported_types.begin(), supported_types.end(), output_type) == supported_types.end()){
                    std::cerr << "Error: unsupported output type for conversion" << std::endl;
                    return 1;
                }

                // create input and output
                inputs.emplace(c.name, input(input_type, nullptr));

                c.out_value_ptr = new uint8_t[io_type_size.at(output_type)];  // create a new place to store the output data

                outputs.emplace(c.name, output(output_type, c.out_value_ptr, &execution_number));

                c.in = &inputs[c.name];
                c.out = &outputs[c.name];

                converters.push_back(c);
            }

            return 0;
        }

};

