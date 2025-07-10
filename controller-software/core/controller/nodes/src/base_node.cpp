#include "base_node.h"


base_node::base_node(){
    // do nothing
}

uint32_t base_node::get_output_object(std::string output_name, output*& ptr){
    if(outputs.find(output_name) == outputs.end()){
        return 1;   // output name not found
    }
    ptr = &outputs[output_name];
    return 0;
}

uint32_t base_node::connect_input(std::string input_name, std::shared_ptr<base_node> source_node, std::string source_output_name){
    if(inputs.find(input_name) == inputs.end()){
        std::cerr << "Error: internal input name not found: " << input_name << std::endl;
        return 1;   // internal input name not found
    }

    output* output_pointer = nullptr;
    if(source_node->get_output_object(source_output_name, output_pointer) != 0){
        std::cerr << "Error: source output name not found: " << source_output_name << std::endl;
        return 2;   // source output name not found
    }

    if (inputs[input_name].set_input_data(output_pointer) != 0){
        std::cerr << "Error: setting input data on " << input_name << std::endl;
        return 3;   // error setting input data
    }

    return 0;
}

uint32_t base_node::connect_input(std::string input_name, output* source_output){
    if(inputs.find(input_name) == inputs.end()){
        return 1;   // internal input name not found
    }

    if (inputs[input_name].set_input_data(source_output) != 0){
        return 2;   // error setting input data
    }

    return 0;
}

uint32_t base_node::set_global_var_input(std::string input_name, global_variable* variable){
    return 1;   // not implemented by default
}

uint32_t base_node::set_global_var_output(std::string output_name, global_variable* variable){
    return 1;   // not implemented by default
}

uint32_t base_node::disconnect_input(std::string input_name){
    if(inputs.find(input_name) == inputs.end()){
        return 1;   // internal input name not found
    }

    if (inputs[input_name].set_default() != 0){
        return 2;   // error setting default value
    }

    return 0;
}

// fix any inputs pointing to invalid ouitputs by switching to the default value
uint32_t base_node::reconnect_inputs(){
    for (auto& pair : inputs) {
        pair.second.reconnect();
    }
    return 0;
}

uint32_t base_node::configure_execution_number(bool* execution_number_modified){
    if(reconnect_inputs() != 0){
        return 1;   // error reconnecting inputs
    }

    // find the highest execution number of all inputs
    int highest_execution_input = -256;
    for (auto& pair : inputs) {
        if(*pair.second.source_execution_number > highest_execution_input){
            highest_execution_input = *pair.second.source_execution_number;
        }
    }

    // update the execution number if needed and signal it was modified
    if(execution_number != highest_execution_input + 1){
        *execution_number_modified = true;
        execution_number = highest_execution_input + 1;
    }
    
    return 0;
}

// mark all outputs for deletion
void base_node::mark_for_deletion(){
    for (auto& pair : outputs) {
        pair.second.mark_for_deletion();
    }
    marked_for_deletion = true;
}

// configure internal settings
uint32_t base_node::configure_settings(json* data){
    return 0;   // no settings to configure
}

base_node::~base_node(){
    // do nothing
}