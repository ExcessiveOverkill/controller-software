#include "api_objects.h"


uint32_t get_new_call_object_from_call_id(std::vector<std::shared_ptr<Base_API>>* calls, const api_call_id_t* api_call_id) {
    if(*api_call_id == api_call_id_t::DEFAULT){
        calls->push_back(std::make_shared<Base_API>());
    }
    else if(*api_call_id == api_call_id_t::MACHINE_STATE){
        calls->push_back(std::make_shared<machine_state>());
    }
    else if(*api_call_id == api_call_id_t::PRINT_UINT32){
        calls->push_back(std::make_shared<print_uint32>());
    }
    else {
        return 1;   // error: unknown call ID
    }

    return 0;
}

uint32_t create_new_call_obj_from_string(std::vector<std::shared_ptr<Base_API>>* calls, std::string api_call_name, uint32_t* api_call_number) {
    if(api_call_name.compare("machine_state") == 0){
        calls->push_back(std::make_shared<machine_state>());
    }
    else if(api_call_name.compare("print_uint32") == 0){
        calls->push_back(std::make_shared<print_uint32>());
    }
    else {
        return 1;
    }

    calls->back()->api_call_number = Base_API::web_to_controller_call_count + 1;
    *api_call_number = calls->back()->api_call_number;
    Base_API::web_to_controller_call_count++;
    calls->back()->persistent_web_mem[0] = Base_API::web_to_controller_call_count;

    return 0;
}

uint32_t remove_last_call_obj_from_string(std::vector<std::shared_ptr<Base_API>>* calls, uint32_t* api_call_number) {  // remove the last call object from the vector, used if the call object is not valid
    
    // decrement call numbers
    api_call_number--;
    Base_API::web_to_controller_call_count--;
    calls->back()->persistent_web_mem[0] = Base_API::web_to_controller_call_count;
    
    calls->pop_back();   // remove failed call object
    return 0;
}