#include "node_factory.h"

#pragma once

class logic_gate: public base_node {
    private:
        input* input_ptrs[32];  // max of 32 inputs supported
        bool output_value = false; // output value of the gate

        bool configured = false;


        enum class gate_type{
            AND,
            OR,
            NOT,
            NAND,
            NOR,
            XOR,
            XNOR,
            UNDEFINED
        } type = gate_type::UNDEFINED;
        uint8_t input_count = 0;

        gate_type get_gate_type(const std::string& type_str) {
            if(type_str == "and") return gate_type::AND;
            if(type_str == "or") return gate_type::OR;
            if(type_str == "not") return gate_type::NOT;
            if(type_str == "nand") return gate_type::NAND;
            if(type_str == "nor") return gate_type::NOR;
            if(type_str == "xor") return gate_type::XOR;
            if(type_str == "xnor") return gate_type::XNOR;
            return gate_type::UNDEFINED;
        }

        uint32_t convert_to_bits(){
            // returns the input bits as an uint32_t
            uint32_t bits = 0;
            for(uint8_t i = 0; i < input_count; i++){
                bits |= (*(bool*)(input_ptrs[i]->data_pointer) << i);
            }
            return bits;
        }
        
    public:

        logic_gate(){
        }

        unsigned int run() override {

            uint32_t bits = convert_to_bits();

            switch(type){
                case gate_type::AND:
                    output_value = (bits == ((1 << input_count) - 1));
                    break;
                case gate_type::OR:
                    output_value = (bits != 0);
                    break;
                case gate_type::NOT:
                    output_value = !(bits & 1); // only NOT the first input
                    break;
                case gate_type::NAND:
                    output_value = (bits != ((1 << input_count) - 1));
                    break;
                case gate_type::NOR:
                    output_value = (bits == 0);
                    break;
                case gate_type::XOR:
                    output_value = __builtin_popcount(bits) % 2 == 1;
                    break;
                case gate_type::XNOR:
                    output_value = __builtin_popcount(bits) % 2 == 0;
                    break;
                default:
                    std::cerr << "Error: undefined gate type" << std::endl;
                    return 1;
            }

            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "gate_type": "and",
                    "input_count": 2
                }
            }
            */

            if(configured){
                std::cerr << "Error: logic gate node already configured" << std::endl;
                return 1;
            }


            auto config = json->find("config");

            std::string gate_type_str = config->at("gate_type").get<std::string>();
            type = get_gate_type(gate_type_str);

            if(type == gate_type::UNDEFINED){
                std::cerr << "Error: invalid gate type for logic gate node" << std::endl;
                return 1;
            }

            input_count = config->at("input_count").get<uint8_t>();
            if(input_count > 32){
                std::cerr << "Error: input count exceeds maximum of 32 for logic gate node" << std::endl;
                return 1;
            }

            // create inputs
            for(uint8_t i = 0; i < input_count; i++){
                std::string input_name = "input_" + std::to_string(i);
                inputs.emplace(input_name, input(io_type::BOOL, nullptr));
                input_ptrs[i] = &inputs[input_name];
            }

            // create output
            outputs.emplace("output", output(io_type::BOOL, &output_value, &execution_number));

            configured = true;
            
            return 0;
        }

};