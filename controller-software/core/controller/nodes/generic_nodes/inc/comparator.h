#include "node_factory.h"

#pragma once


class comparator: public base_node {
    private:
        bool out = false;
        input* in_0 = nullptr; // pointer to the first input object
        input* in_1 = nullptr; // pointer to the second input object

        io_type type = io_type::UNDEFINED;

        bool configured = false;

        enum class comparator_type {
            EQUAL,
            NOT_EQUAL,
            LESS_THAN,
            LESS_THAN_OR_EQUAL,
            GREATER_THAN,
            GREATER_THAN_OR_EQUAL,
            UNDEFINED
        } comp_type = comparator_type::UNDEFINED;

        comparator_type get_comparator_type(std::string type_str) {
            if (type_str == "equal") {
                return comparator_type::EQUAL;
            } else if (type_str == "not_equal") {
                return comparator_type::NOT_EQUAL;
            } else if (type_str == "less_than") {
                return comparator_type::LESS_THAN;
            } else if (type_str == "less_than_or_equal") {
                return comparator_type::LESS_THAN_OR_EQUAL;
            } else if (type_str == "greater_than") {
                return comparator_type::GREATER_THAN;
            } else if (type_str == "greater_than_or_equal") {
                return comparator_type::GREATER_THAN_OR_EQUAL;
            } else {
                return comparator_type::UNDEFINED;
            }
        }

        template<typename T>
        bool compare() {
            T val1 = *(T*)in_0->data_pointer;
            T val2 = *(T*)in_1->data_pointer;
            switch (comp_type) {
                case comparator_type::EQUAL:
                    return val1 == val2;
                case comparator_type::NOT_EQUAL:
                    return val1 != val2;
                case comparator_type::LESS_THAN:
                    return val1 < val2;
                case comparator_type::LESS_THAN_OR_EQUAL:
                    return val1 <= val2;
                case comparator_type::GREATER_THAN:
                    return val1 > val2;
                case comparator_type::GREATER_THAN_OR_EQUAL:
                    return val1 >= val2;
                default:
                    return false; // undefined comparison
            }
        }
        
        
    public:

        comparator(){
        }

        unsigned int run() override {

            switch(type){
                case io_type::UINT8:
                    out = compare<uint8_t>();
                    break;
                case io_type::INT8:
                    out = compare<int8_t>();
                    break;
                case io_type::UINT16:
                    out = compare<uint16_t>();
                    break;
                case io_type::INT16:
                    out = compare<int16_t>();
                    break;
                case io_type::UINT32:
                    out = compare<uint32_t>();
                    break;
                case io_type::INT32:
                    out = compare<int32_t>();
                    break;
                case io_type::DOUBLE:
                    out = compare<double>();
                    break;
                case io_type::FLOAT:
                    out = compare<float>();
                    break;
                case io_type::BOOL:
                    out = compare<bool>();
                    break;
                default:
                    std::cerr << "Error: invalid type for comparator node" << std::endl;
                    return 1; // invalid type
            }

            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "type": "uint8",
                    "comparator_type": "",
                        
                }
            }
            */

            if(configured){
                std::cerr << "Error: comparator node already configured" << std::endl;
                return 1;
            }

            auto config = json->find("config");
            io_type type = get_io_type(config->at("type").get<std::string>());
            if(type == io_type::UNDEFINED){
                std::cerr << "Error: invalid type for comparator node" << std::endl;
                return 1;
            }

            comp_type = get_comparator_type(config->at("comparator_type").get<std::string>());
            if(comp_type == comparator_type::UNDEFINED){
                std::cerr << "Error: invalid comparator type for comparator node" << std::endl;
                return 1;
            }

            outputs.emplace("output", output(io_type::BOOL, &out, &execution_number));
            inputs.emplace("in_0", input(type, nullptr));
            inputs.emplace("in_1", input(type, nullptr));
            in_0 = &inputs["in_0"];
            in_1 = &inputs["in_1"];

            configured = true;
            
            return 0;
        }

};