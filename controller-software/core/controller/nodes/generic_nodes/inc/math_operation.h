#include "node_factory.h"
#include <cmath>

#pragma once


class math_operation: public base_node {
    private:
        void* out = nullptr; // pointer to the output data
        input* input_signals[32] = {nullptr};
        uint8_t input_count = 0;

        io_type type = io_type::UNDEFINED;

        bool configured = false;

        enum class operation_mode {
            NARY,
            BINARY,
            UNARY,
            UNDEFINED
        } mode = operation_mode::UNDEFINED;

        enum class nary_operation {
            ADD,
            SUB,
            MUL,
            DIV,
            MIN,
            MAX,
            AVG,
            UNDEFINED
        } nary_op = nary_operation::UNDEFINED;

        enum class binary_operation {
            POW,
            ATAN2,
            UNDEFINED
        } binary_op = binary_operation::UNDEFINED;

        enum class unary_operation {
            LN,
            LOG10,
            SINE,
            COS,
            TAN,
            ASIN,
            ACOS,
            ATAN,
            NEGATE,
            ABS,
            UNDEFINED
        } unary_op = unary_operation::UNDEFINED;

        operation_mode get_operation_mode(std::string mode_str) {
            if (mode_str == "nary") {
                return operation_mode::NARY;
            } else if (mode_str == "binary") {
                return operation_mode::BINARY;
            } else if (mode_str == "unary") {
                return operation_mode::UNARY;
            } else {
                return operation_mode::UNDEFINED;
            }
        }

        nary_operation get_nary_operation(std::string op_str) {
            if (op_str == "add") {
                return nary_operation::ADD;
            } else if (op_str == "sub") {
                return nary_operation::SUB;
            } else if (op_str == "div") {
                return nary_operation::DIV;
            } else if (op_str == "mul") {
                return nary_operation::MUL;
            } else if (op_str == "min") {
                return nary_operation::MIN;
            } else if (op_str == "max") {
                return nary_operation::MAX;
            } else if (op_str == "avg") {
                return nary_operation::AVG;
            } else {
                return nary_operation::UNDEFINED;
            }
        }

        binary_operation get_binary_operation(std::string op_str) {
            if (op_str == "pow") {
                return binary_operation::POW;
            } else if (op_str == "atan2") {
                return binary_operation::ATAN2;
            } else {
                return binary_operation::UNDEFINED;
            }
        }

        unary_operation get_unary_operation(std::string op_str) {
            if (op_str == "ln") {
                return unary_operation::LN;
            } else if (op_str == "log10") {
                return unary_operation::LOG10;
            } else if (op_str == "sine") {
                return unary_operation::SINE;
            } else if (op_str == "cos") {
                return unary_operation::COS;
            } else if (op_str == "tan") {
                return unary_operation::TAN;
            } else if (op_str == "asin") {
                return unary_operation::ASIN;
            } else if (op_str == "acos") {
                return unary_operation::ACOS;
            } else if (op_str == "atan") {
                return unary_operation::ATAN;
            } else if (op_str == "negate") {
                return unary_operation::NEGATE;
            } else if (op_str == "abs") {
                return unary_operation::ABS;
            } else {
                return unary_operation::UNDEFINED;
            }
        }

        template<typename T>
        uint32_t do_nary_operation() {
            T result = 0;
            for (uint8_t i = 0; i < input_count; ++i) {
                T value = *(T*)input_signals[i]->data_pointer;
                switch (nary_op) {
                    case nary_operation::ADD:
                        result += value;
                        break;
                    case nary_operation::SUB:
                        if (i == 0) {
                            result = value; // initialize result with the first value
                        }
                        else{
                            result -= value;
                        }
                        break;
                    case nary_operation::MUL:
                        if (i == 0) {
                            result = value; // initialize result with the first value
                        }
                        else{
                            result *= value;
                        }
                        break;
                    case nary_operation::DIV:
                        if (i == 0) {
                            result = value; // initialize result with the first value
                        }
                        else{
                            if (value == 0) {
                                std::cerr << "Error: division by zero in math_operation node" << std::endl;
                                return 2; // division by zero error
                            }
                            result /= value;
                        }
                        break;
                    case nary_operation::MIN:
                        if (i == 0 || value < result) {
                            result = value;
                        }
                        break;
                    case nary_operation::MAX:
                        if (i == 0 || value > result) {
                            result = value;
                        }
                        break;
                    case nary_operation::AVG:
                        result += value; // will be divided by input_count later
                        break;
                    default:
                        std::cerr << "Error: undefined nary operation" << std::endl;
                        return 2; // undefined operation error
                }
            }
            if (nary_op == nary_operation::AVG) {
                result /= input_count; // calculate average
            }
            *(T*)out = result;
            return 0; // success
        }


        template<typename T>
        uint32_t do_binary_operation() {
            T value1 = *(T*)input_signals[0]->data_pointer;
            T value2 = *(T*)input_signals[1]->data_pointer;
            switch (binary_op) {
                case binary_operation::POW:
                    *(T*)out = pow(value1, value2);
                    break;
                case binary_operation::ATAN2:
                    *(T*)out = atan2(value2, value1); // atan2 expects (y, x)
                    break;
                default:
                    std::cerr << "Error: undefined binary operation" << std::endl;
                    return 2; // undefined operation error
            }
            return 0; // success
        }


        template<typename T>
        uint32_t do_unary_operation() {
            T value = *(T*)input_signals[0]->data_pointer;
            switch(unary_op) {
                case unary_operation::LN:
                    *(T*)out = log(value);
                    break;
                case unary_operation::LOG10:
                    *(T*)out = log10(value);
                    break;
                case unary_operation::SINE:
                    *(T*)out = sin(value);
                    break;
                case unary_operation::COS:
                    *(T*)out = cos(value);
                    break;
                case unary_operation::TAN:
                    *(T*)out = tan(value);
                    break;
                case unary_operation::ASIN:
                    *(T*)out = asin(value);
                    break;
                case unary_operation::ACOS:
                    *(T*)out = acos(value);
                    break;
                case unary_operation::ATAN:
                    *(T*)out = atan(value);
                    break;
                case unary_operation::NEGATE:
                    *(T*)out = -value;
                    break;
                case unary_operation::ABS:
                    *(T*)out = abs(value);
                    break;
                default:
                    std::cerr << "Error: undefined unary operation" << std::endl;
                    return 2; // undefined operation error
            }
            double d = *(double*)out;
            float f = *(float*)out;
            return 0; // success
        }
        
    public:

        math_operation(){
        }

        unsigned int run() override {

            switch(mode) {
                case operation_mode::NARY:
                    switch(type) {
                        case io_type::UINT8:
                            return do_nary_operation<uint8_t>();
                        case io_type::UINT16:
                            return do_nary_operation<uint16_t>();
                        case io_type::UINT32:
                            return do_nary_operation<uint32_t>();
                        case io_type::INT8:
                            return do_nary_operation<int8_t>();
                        case io_type::INT16:
                            return do_nary_operation<int16_t>();
                        case io_type::INT32:
                            return do_nary_operation<int32_t>();
                        case io_type::DOUBLE:
                            return do_nary_operation<double>();
                        case io_type::FLOAT:
                            return do_nary_operation<float>();
                        default:
                            std::cerr << "Error: invalid type for nary operation" << std::endl;
                            return 1; // invalid type error
                    }
                    break;
                case operation_mode::BINARY:
                    switch(type) {
                        case io_type::UINT8:
                            return do_binary_operation<uint8_t>();
                        case io_type::UINT16:
                            return do_binary_operation<uint16_t>();
                        case io_type::UINT32:
                            return do_binary_operation<uint32_t>();
                        case io_type::INT8:
                            return do_binary_operation<int8_t>();
                        case io_type::INT16:
                            return do_binary_operation<int16_t>();
                        case io_type::INT32:
                            return do_binary_operation<int32_t>();
                        case io_type::DOUBLE:
                            return do_binary_operation<double>();
                        case io_type::FLOAT:
                            return do_binary_operation<float>();
                        default:
                            std::cerr << "Error: invalid type for binary operation" << std::endl;
                            return 1; // invalid type error
                    }
                    break;
                case operation_mode::UNARY:
                    switch(type) {
                        case io_type::UINT8:
                            return do_unary_operation<uint8_t>();
                        case io_type::UINT16:
                            return do_unary_operation<uint16_t>();
                        case io_type::UINT32:
                            return do_unary_operation<uint32_t>();
                        case io_type::INT8:
                            return do_unary_operation<int8_t>();
                        case io_type::INT16:
                            return do_unary_operation<int16_t>();
                        case io_type::INT32:
                            return do_unary_operation<int32_t>();
                        case io_type::DOUBLE:
                            return do_unary_operation<double>();
                        case io_type::FLOAT:
                            return do_unary_operation<float>();
                        default:
                            std::cerr << "Error: invalid type for unary operation" << std::endl;
                            return 1; // invalid type error
                    }
                    break;
                default:
                    std::cerr << "Error: undefined operation mode" << std::endl;
                    return 1; // undefined operation mode error
            }

            return 0;
        }

        unsigned int configure_settings(json* json) override{
            
            /*
            parse settings from json
            {
                "config":{
                    "type": "uint8",
                    "op_type": "nary",
                    "op": "add",
                    "input_count": 3
                        
                }
            }
            */

            if(configured){
                std::cerr << "Error: comparator node already configured" << std::endl;
                return 1;
            }

            type = get_io_type(json->at("type").get<std::string>());
            if(type == io_type::UNDEFINED){
                std::cerr << "Error: invalid type for comparator node" << std::endl;
                return 1;
            }

            mode = get_operation_mode(json->at("op_type").get<std::string>());
            if (mode == operation_mode::UNDEFINED) {
                std::cerr << "Error: invalid operation mode" << std::endl;
                return 2;
            }

            switch(mode) {
                case operation_mode::NARY:
                    nary_op = get_nary_operation(json->at("op").get<std::string>());
                    if (nary_op == nary_operation::UNDEFINED) {
                        std::cerr << "Error: invalid nary operation" << std::endl;
                        return 3;
                    }
                    input_count = json->at("input_count").get<uint8_t>();
                    if (input_count < 2 || input_count > 32) {
                        std::cerr << "Error: input count must be between 2 and 32" << std::endl;
                        return 4;
                    }
                    break;
                case operation_mode::BINARY:
                    binary_op = get_binary_operation(json->at("op").get<std::string>());
                    if (binary_op == binary_operation::UNDEFINED) {
                        std::cerr << "Error: invalid binary operation" << std::endl;
                        return 4;
                    }
                    input_count = 2; // binary operations always have 2 inputs
                    break;
                case operation_mode::UNARY:
                    unary_op = get_unary_operation(json->at("op").get<std::string>());
                    if (unary_op == unary_operation::UNDEFINED) {
                        std::cerr << "Error: invalid unary operation" << std::endl;
                        return 5;
                    }
                    input_count = 1; // unary operations always have 1 input
                    break;
                default:
                    std::cerr << "Error: unknown operation mode" << std::endl;
                    return 6;
            }

            switch(type) {
                case io_type::UINT8:
                    out = new uint8_t(0);
                    break;
                case io_type::UINT16:
                    out = new uint16_t(0);
                    break;
                case io_type::UINT32:
                    out = new uint32_t(0);
                    break;
                case io_type::INT8:
                    out = new int8_t(0);
                    break;
                case io_type::INT16:
                    out = new int16_t(0);
                    break;
                case io_type::INT32:
                    out = new int32_t(0);
                    break;
                case io_type::DOUBLE:
                    out = new double(0.0);
                    break;
                case io_type::FLOAT:
                    out = new float(0.0f);
                    break;
                default:
                    std::cerr << "Error: invalid type for math operation node" << std::endl;
                    return 7; // invalid type
            }

            outputs.emplace("output", output(type, out, &execution_number));

            for (uint8_t i = 0; i < input_count; ++i) {
                std::string input_name = "input_" + std::to_string(i);
                inputs.emplace(input_name, input(type, nullptr));
                input_signals[i] = &inputs[input_name];
            }

            configured = true;
            
            return 0;
        }

};