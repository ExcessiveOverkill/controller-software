#include <vector>
#include <memory> // For smart pointers
#include <string>
#include "shared_mem.h"
#include "../json.hpp"


#include "calls/machine_state.h"

using json = nlohmann::json;

#pragma once

uint32_t get_new_call_object_from_call_id(std::vector<std::shared_ptr<Base_API>>* calls, const api_call_id_t* api_call_id) {
    if(*api_call_id == api_call_id_t::DEFAULT){
        calls->push_back(std::make_shared<default_call>());
    }
    else if(*api_call_id == api_call_id_t::MACHINE_STATE){
        calls->push_back(std::make_shared<machine_state>());
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
    else {
        return 1;
    }

    calls->back()->api_call_number = Base_API::web_to_controller_call_count + 1;
    *api_call_number = calls->back()->api_call_number;
    Base_API::web_to_controller_call_count++;

    return 0;
}