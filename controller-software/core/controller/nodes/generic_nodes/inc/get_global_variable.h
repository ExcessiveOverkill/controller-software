#include "node_factory.h"

/*

Make a global variable accessible by nodes

*/

#pragma once

class get_global_variable: public base_node {
    private:
        
    public:

        get_global_variable(){
            execution_number = -1;
            type = "get_global_variable";
        }

        unsigned int run() override {
            // nothing to do
            return 0;
        }

        uint32_t set_global_var_output(std::string output_name, global_variable* variable) override {
            // add if output doesn't already exist
            if(outputs.find(output_name) != outputs.end()){
                std::cerr << "Error: output name already exists" << std::endl;
                return 2;   // output name already exists
            }

            // TODO: might want to store these pointers somewhere so they can be freed later
            void** variable_ptr = new void*;

            variable->get_data_pointer(variable_ptr);

            outputs.emplace(output_name, output(variable->get_type(), *variable_ptr, &execution_number));
            
            return 0;
        }

};