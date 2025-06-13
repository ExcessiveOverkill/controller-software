#include "node_factory.h"
#include <chrono>

/*

Allows API commands to be sent to the controller to set signal values
values are clamped to the min/max settings
if a timeout value is set, the value will be reset to the configured default value (0 by default) after the timeout period

*/

#pragma once

class api_input: public base_node {
    private:

        double min = 0.0;
        double max = 0.0;
        double default_value = 0.0;
        double timeout = 0.0;    // timeout in seconds, if set to 0.0, no timeout will be applied
        double last_set_time = 0.0;

        void* data_ptr = nullptr; // pointer to the data that will be used as output
        double in_data = 0.0;
        output* output_ptr = nullptr; // pointer to the output object

        io_type type = io_type::UNDEFINED;  // output type

        bool configured = false; // true if the node has been configured

    public:

        api_input(){
            execution_number = -1;
        }

        unsigned int run() override {

            double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now().time_since_epoch()).count();

            // reset value if timeout has been reached
            if(timeout > 0.0 && last_set_time + timeout < seconds){
                in_data = default_value;
            }

            // update output value
            if(output_ptr == nullptr){
                throw std::runtime_error("Output data pointer is null, configure the api input node first.");
            }
            switch(type){
                case io_type::UINT8:
                    *(uint8_t*)output_ptr->data_pointer = (uint8_t)in_data;
                    break;
                case io_type::INT8:
                    *(int8_t*)output_ptr->data_pointer = (int8_t)in_data;
                    break;
                case io_type::UINT16:
                    *(uint16_t*)output_ptr->data_pointer = (uint16_t)in_data;
                    break;
                case io_type::INT16:
                    *(int16_t*)output_ptr->data_pointer = (int16_t)in_data;
                    break;
                case io_type::UINT32:
                    *(uint32_t*)output_ptr->data_pointer = (uint32_t)in_data;
                    break;
                case io_type::INT32:
                    *(int32_t*)output_ptr->data_pointer = (int32_t)in_data;
                    break;
                case io_type::DOUBLE:
                    *(double*)output_ptr->data_pointer = in_data;
                    break;
                case io_type::FLOAT:
                    *(float*)output_ptr->data_pointer = (float)in_data;
                    break;
                case io_type::BOOL:
                    *(bool*)output_ptr->data_pointer = (in_data != 0.0); // convert to bool
                    break;
                default:
                    std::cerr << "Error: invalid output type for api_input node" << std::endl;
                    return 1;   // invalid output type
            }

            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":
                {
                    "type": "float",
                    "min": -1.0,
                    "max": 1.0,
                    "default": 0.0,
                    "timeout": 0.0
                },
                "set": 0.5
            }
            */

            

            if(json->find("config") != json->end()){    // config mode
                if(configured){
                    std::cerr << "Error: api_input node already configured" << std::endl;
                    return 1;
                }
                auto c = (*json)["config"];
                if(c.at("min").is_null()){
                    min = -INFINITY;
                }
                else{
                    min = c["min"];
                }
                if(c.at("max").is_null()){
                    max = INFINITY;
                }
                else{
                    max = c["max"];
                }
                if(c.at("default").is_null()){
                    default_value = 0.0;
                }
                else{
                    default_value = c["default"];
                }
                if(c.at("timeout").is_null()){
                    timeout = 0.0;
                }
                else{
                    timeout = c["timeout"];
                }
                if(timeout < 0.0){
                    timeout = 0.0; // set to 0 if negative
                }
                type = get_io_type(c["type"].get<std::string>());

                if(min > max){
                    std::cerr << "Error: min is greater than max" << std::endl;
                    return 2;
                }
                if(timeout < 0.0){
                    std::cerr << "Error: timeout cannot be negative" << std::endl;
                    return 3;
                }
                if(default_value < min || default_value > max){
                    std::cerr << "Error: default value is out of range" << std::endl;
                    return 4;
                }

                switch(type){
                    case io_type::UINT8:
                        data_ptr = new uint8_t(default_value);
                        break;
                    case io_type::INT8:
                        data_ptr = new int8_t(default_value);
                        break;
                    case io_type::UINT16:
                        data_ptr = new uint16_t(default_value);
                        break;
                    case io_type::INT16:
                        data_ptr = new int16_t(default_value);
                        break;
                    case io_type::UINT32:
                        data_ptr = new uint32_t(default_value);
                        break;
                    case io_type::INT32:
                        data_ptr = new int32_t(default_value);
                        break;
                    case io_type::DOUBLE:
                        data_ptr = new double(default_value);
                        break;
                    case io_type::FLOAT:
                        data_ptr = new float(default_value);
                        break;
                    case io_type::BOOL:
                        data_ptr = new bool(default_value != 0.0); // convert to bool
                        break;
                    default:
                        std::cerr << "Error: invalid type for api_input node" << std::endl;
                        return 5;
                }

                outputs.emplace("output", output(type, data_ptr, &execution_number));
                output_ptr = &outputs["output"]; // save the output pointer for later use

                configured = true;
            }

            if(!configured){
                std::cerr << "Error: api_input node not configured" << std::endl;
                return 1;
            }

            if(json->find("set") != json->end()){    // set mode
                if(!configured){
                    std::cerr << "Error: api_input node not configured, cannot set value" << std::endl;
                    return 1;
                }
                if(json->at("set").is_null()){
                    std::cerr << "Error: set value is null" << std::endl;
                    return 1;
                }
                double value;
                if(type == io_type::BOOL){
                    value = json->at("set").get<bool>() ? 1.0 : 0.0; // convert bool to double
                }
                else{
                    value = json->at("set").get<double>();
                }
                value = std::max(min, std::min(max, value)); // clamp value to min/max
                in_data = value;
                last_set_time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now().time_since_epoch()).count();
            }
            
            return 0;
        }

};