#include "base_node.h"

class logic_not: public base_node {
    private:
        bool default_input_value = false;
        bool output_value = true;

        input* input_value = nullptr;
    public:

        logic_not(){
            inputs.emplace("input", input(io_type::BOOL, &default_input_value));

            outputs.emplace("output", output(io_type::BOOL, &output_value, &execution_number));
            
            input_value = &inputs["input"];
        }

        unsigned int run() override {
            output_value = !(*reinterpret_cast<bool*>(input_value->data_pointer));
            return 0;
        }

};