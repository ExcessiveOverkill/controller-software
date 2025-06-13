#include "node_factory.h"
#include <iostream>

#pragma once

/*
Convert a value to another type
all conversions pass through a double type
*/


class converter: public base_node {
    private:


        bool configured = false;

        struct conv{
            input* in = nullptr;
            output* out = nullptr;

            double input_max = 0.0;
            double input_min = 0.0;

            double output_max = 0.0;
            double output_min = 0.0;

            bool invert = false;

            bool scale_mode = true; // if true, scale input to output range, if false, just copy input value to output

            void* out_value_ptr = nullptr;

        } c;

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

        converter(){
            
        }

        uint32_t run() override {
            
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

            double out_value;

            // if scale_mode is false, just copy the input value to the output
            if(!c.scale_mode){
                out_value = in_value;  // no scaling, just copy
            }
            else{

                // clamp to input range
                if(in_value < c.input_min){
                    in_value = c.input_min;
                }
                if(in_value > c.input_max){
                    in_value = c.input_max;
                }

                // scale to output range
                out_value = (in_value - c.input_min) / (c.input_max - c.input_min) * (c.output_max - c.output_min) + c.output_min;
                if(c.invert){
                    out_value = c.output_max - out_value + c.output_min; // invert the output value
                }

                // clamp to output range (just in case)
                if(out_value < c.output_min){
                    out_value = c.output_min;
                }
                if(out_value > c.output_max){
                    out_value = c.output_max;
                }
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

            return 0;
        }

        uint32_t configure_settings(json* json) override {

            /*
            parse settings from json
            {
                "config":{
                    "input_type": "float",
                    "output_type": "uint32",
                    "input_min": 0.0,
                    "input_max": 1.0,
                    "output_min": 0.0,
                    "output_max": 1.0,
                    "invert": false,
                    "mode": "scale" // "scale" or "direct", 
                                    // "scale" will scale the input to the output range,
                                    // "direct" will just copy the input value to the output
                                    // input/output min/max will be ignored in "direct" mode
                }
            }
            */

            if(configured){
                std::cerr << "Error: converter node already configured" << std::endl;
                return 1;
            }

            bool min_max_defined = true;
            if(json->find("input_min") == json->end() || 
                json->find("input_max") == json->end() || 
                json->find("output_min") == json->end() || 
                json->find("output_max") == json->end()){
                min_max_defined = false;
            }
            else{
                c.input_min = json->at("input_min").get<double>();
                c.input_max = json->at("input_max").get<double>();
                c.output_min = json->at("output_min").get<double>();
                c.output_max = json->at("output_max").get<double>();

                if(c.input_max <= c.input_min){
                    std::cerr << "Error: input_max must be greater than input_min" << std::endl;
                    return 1;
                }
                if(c.output_max <= c.output_min){
                    std::cerr << "Error: output_max must be greater than output_min" << std::endl;
                    return 1;
                }
            }

            std::string mode = json->at("mode").get<std::string>();
            if(mode != "scale" && mode != "direct"){
                std::cerr << "Error: invalid mode specified, must be 'scale' or 'direct'" << std::endl;
                return 1;
            }
            if(mode == "direct"){
                c.scale_mode = false;
                if(min_max_defined){
                    std::cerr << "Warning: input/output min/max will be ignored in 'direct' mode" << std::endl;
                }
            }
            else{
                c.scale_mode = true;
                if(!min_max_defined){
                    std::cerr << "Error: input/output min/max must be defined in 'scale' mode" << std::endl;
                    return 1;
                }
                c.invert = json->at("invert").get<bool>();
            }


            io_type input_type = get_io_type(json->at("input_type").get<std::string>());
            io_type output_type = get_io_type(json->at("output_type").get<std::string>());

            // some input validation

            if(std::find(supported_types.begin(), supported_types.end(), input_type) == supported_types.end()){
                std::cerr << "Error: unsupported input type for conversion" << std::endl;
                return 1;
            }
            if(std::find(supported_types.begin(), supported_types.end(), output_type) == supported_types.end()){
                std::cerr << "Error: unsupported output type for conversion" << std::endl;
                return 1;
            }

            // create input and output
            inputs.emplace("input", input(input_type, nullptr));

            c.out_value_ptr = new uint8_t[io_type_size.at(output_type)];  // create a new place to store the output data

            outputs.emplace("output", output(output_type, c.out_value_ptr, &execution_number));

            c.in = &inputs["input"];
            c.out = &outputs["output"];
            
            configured = true;

            return 0;
        }

};

