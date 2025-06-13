#include "node_factory.h"

#pragma once


class cycle_delay: public base_node {
    private:
        void* out = nullptr; // output value
        input* in = nullptr; // pointer to the input

        static const uint16_t mem_size_bytes = 256; // memory for storing the delay data, not that max cycles will depend on the data type also
        uint8_t data[mem_size_bytes];

        io_type type = io_type::UNDEFINED; // type of the data
        size_t data_size = 0; // size of the data in bytes
        uint16_t max_cycles = 0;    // maximum cycles that can be delayed, depends on the data type and memory size

        // times are in execution cycles
        uint16_t cycles = 0;
        uint16_t current_in_cycle = 0;
        uint16_t current_out_cycle = 0;

        bool configured = false;
        
    public:

        cycle_delay(){
        }

        unsigned int run() override {

            memcpy(data + current_in_cycle * data_size, in->data_pointer, data_size); // input to buffer
            memcpy(out, data + current_out_cycle * data_size, data_size);   // buffer to output

            current_in_cycle++;
            current_out_cycle++;
            if(current_in_cycle > cycles){
                current_in_cycle = 0; // reset the input cycle counter
            }
            if(current_out_cycle > cycles){
                current_out_cycle = 0; // reset the output cycle counter
            }
            
            
            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "type": "uint8",
                    "cycles": 10
                    }
            }
            */

            if(configured){
                std::cerr << "Error: cycle_delay node already configured" << std::endl;
                return 1;
            }

            
            type = get_io_type(json->at("type").get<std::string>());
            cycles = json->at("cycles").get<uint16_t>();
            if(type == io_type::UNDEFINED){
                std::cerr << "Error: cycle_delay node type is undefined" << std::endl;
                return 1;
            }
            if(cycles == 0){
                std::cerr << "Error: cycle_delay node cycles is 0" << std::endl;
                return 1;
            }
            data_size = io_type_size.at(type);
            max_cycles = mem_size_bytes / data_size;
            if(cycles > max_cycles){
                std::cerr << "Error: cycle_delay node cycles is greater than maximum cycles for type " << get_io_type_string(type)  << " (memory too small)" << std::endl;
                return 1;
            }


            outputs.emplace("output", output(type, &out, &execution_number));

            inputs.emplace("input", input(type, nullptr));
            in = &inputs["input"];

            if(in->data_pointer == nullptr){
                std::cerr << "Error: cycle_delay node input has no default" << std::endl;
                return 1;
            }

            // initialize the data memory
            for(uint16_t i = 0; i < max_cycles; i++){
                memcpy(data + i*data_size, in->data_pointer, data_size);
            }

            memcpy(out, in->data_pointer, data_size); // set the output to the current input value

            current_in_cycle = cycles;

            configured = true;
            
            return 0;
        }

};