#include <vector>
#include <memory>
#include <string>
#include "json.hpp"


#include "machine_state.h"
#include "print_uint32.h"

using json = nlohmann::json;

#pragma once

uint32_t get_new_call_object_from_call_id(std::vector<std::shared_ptr<Base_API>>* calls, const api_call_id_t* api_call_id);

uint32_t create_new_call_obj_from_string(std::vector<std::shared_ptr<Base_API>>* calls, std::string api_call_name, uint32_t* api_call_number);

uint32_t remove_last_call_obj_from_string(std::vector<std::shared_ptr<Base_API>>* calls, uint32_t* api_call_number);  // remove the last call object from the vector, used if the call object is not valid