#include "api_objects.h"
#include "shared_mem.h"

// Define static member variables

uint32_t Base_API::data_buffer_size = 0;
uint32_t Base_API::control_buffer_size = 0;


void* Base_API::web_to_controller_data_base_address = nullptr;
void* Base_API::controller_to_web_data_base_address = nullptr;


void* Base_API::web_to_controller_control_base_address = nullptr;
void* Base_API::controller_to_web_control_base_address = nullptr;


uint32_t Base_API::web_to_controller_data_mem_index = 0;
uint32_t Base_API::controller_to_web_data_mem_index = 0;


uint32_t Base_API::web_to_controller_control_mem_index = 0;
uint32_t Base_API::controller_to_web_control_mem_index = 0;


uint32_t Base_API::web_to_controller_call_count = 0;
uint32_t Base_API::controller_to_web_call_count = 0;

int main() {

    shared_mem shared_mem_obj;
    
    if(shared_mem_obj.web_create_shared_mem() !=0 ){
        std::cerr << "Error creating shared memory" << std::endl;
        return 1;
    }

    default_call default_call_obj;
    default_call_obj.set_shared_memory(shared_mem_obj);


    // Create a vector of Base_API pointers
    std::vector<std::shared_ptr<Base_API>> calls;
 
    get_new_call_object_from_string(&calls, "machine_state");
    calls.back()->web_input_data("commanded_state", "on");

    get_new_call_object_from_string(&calls, "machine_state");
    calls.back()->web_input_data("commanded_state", "off");

    write_web_calls_to_controller(calls);


    // Call functions
    //callFunctions(objects);
    shared_mem_obj.close_shared_mem();
    return 0;
}