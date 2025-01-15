#include "../base_node.h"
#include <iostream>

class bool_print_cout: public base_node {
    private:
        bool default_input_value = false;
        bool output_value = true;

        input* input_value = nullptr;
    public:

        bool_print_cout(){
            inputs.emplace("input", input(io_type::BOOL, &default_input_value));
            
            input_value = &inputs["input"];
        }

        unsigned int run() override {
            output_value = (*reinterpret_cast<bool*>(input_value->data_pointer));
            std::cout << "bool output: " << output_value << std::endl;
            return 0;
        }

};