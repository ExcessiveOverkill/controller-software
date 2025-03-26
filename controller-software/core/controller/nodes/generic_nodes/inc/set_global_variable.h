#include "node_factory.h"

/*

Make a global variable accessible by nodes

*/

#pragma once

class set_global_variable: public base_node {
    private:
        struct input_pair{
            input* input_ptr = nullptr;
            global_variable* variable = nullptr;
        };

        std::vector<input_pair> input_pairs;
        
    public:

        set_global_variable(){
            //execution_number = -1;
            type = "set_global_variable";
        }

        unsigned int run() override {

            // copy input data to global variable
            for(auto& pair : input_pairs){
                if(pair.variable != nullptr && pair.input_ptr->data_pointer != nullptr){
                    pair.variable->set_value(pair.input_ptr->data_pointer);
                }
            }
            return 0;
        }

        uint32_t set_global_var_input(std::string input_name, global_variable* variable) override {
            // add if input doesn't already exist
            if(inputs.find(input_name) != inputs.end()){
                std::cerr << "Error: input name already exists" << std::endl;
                return 2;   // input name already exists
            }

            inputs.emplace(input_name, input(variable->get_type(), nullptr));

            input_pair new_pair;
            new_pair.input_ptr = &inputs[input_name];
            new_pair.variable = variable;
            input_pairs.push_back(new_pair);
            
            return 0;
        }

};