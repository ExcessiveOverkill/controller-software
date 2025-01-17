#include "../base_node.h"

class logic_and: public base_node {
    private:
        bool default_input_value = false;
        bool output_value = true;

        input* input_value_A = nullptr;
        input* input_value_B = nullptr;
    public:

        logic_and(){
            inputs.emplace("input_A", input(io_type::BOOL, &default_input_value));
            inputs.emplace("input_B", input(io_type::BOOL, &default_input_value));

            outputs.emplace("output", output(io_type::BOOL, &output_value, &execution_number));
            
            input_value_A = &inputs["input_A"];
            input_value_B = &inputs["input_B"];
        }

        unsigned int run() override {
            output_value = (*reinterpret_cast<bool*>(input_value_A->data_pointer)) & (*reinterpret_cast<bool*>(input_value_B->data_pointer));
            return 0;
        }

};