#include "api_objects.h"
#include "shared_mem.h"

#pragma once

// Define static member variables

uint32_t Base_API::data_buffer_size = 0;
uint32_t Base_API::control_buffer_size = 0;

void* Base_API::web_to_controller_data_base_address = nullptr;
void* Base_API::controller_to_web_data_base_address = nullptr;

void* Base_API::web_to_controller_control_base_address = nullptr;
void* Base_API::controller_to_web_control_base_address = nullptr;

volatile uint32_t* Base_API::persistent_web_mem = nullptr;

uint32_t Base_API::web_to_controller_data_mem_index = 0;
uint32_t Base_API::controller_to_web_data_mem_index = 0;

uint32_t Base_API::web_to_controller_control_mem_index = 0;
uint32_t Base_API::controller_to_web_control_mem_index = 0;

uint32_t Base_API::web_to_controller_call_count = 0;
uint32_t Base_API::controller_to_web_call_count = 0;

#define MAX_COMPLETED_CALL_TO_GET 10

class web_api {
    private:
        std::vector<std::shared_ptr<Base_API>> calls;
        std::vector<std::shared_ptr<Base_API>> completed_calls;
        shared_mem shared_mem_obj;
        default_call default_call_obj;
        api_call_id_t api_call_id = api_call_id_t::DEFAULT;

        uint32_t get_next_api_call_id_from_shared_mem();

        uint32_t get_last_controller_to_web_call();
        
    public:
        web_api(){
            if(shared_mem_obj.web_create_shared_mem() !=0 ){
                std::cerr << "Error creating shared memory" << std::endl;
            }

            default_call_obj.set_shared_memory(shared_mem_obj);

        }

        uint32_t add_call(json* data, uint32_t* api_call_number);

        uint32_t write_web_calls_to_controller();

        uint32_t get_completed_calls(json* data_out);

        ~web_api(){
            shared_mem_obj.unmap_shared_mem();
            shared_mem_obj.close_shared_mem();
        }
};

uint32_t web_api::write_web_calls_to_controller() {
    for (const auto& obj : calls) {
        uint32_t ret = obj->web_write_to_shared_mem();
        if(ret){
            return ret;
        }
    }
    calls.clear();
    return 0;
}

uint32_t web_api::get_completed_calls(json* data_out) {
    // read calls from shared memory and create new call objects in vector
    
    completed_calls.clear();    // make sure there are no old completed calls in vector

    for(int i = 0; i < MAX_COMPLETED_CALL_TO_GET; i++){
        if(get_next_api_call_id_from_shared_mem() == 0){
            //std::cout << "new call received" << std::endl;
            if(get_new_call_object_from_call_id(&completed_calls, &api_call_id) == 1){
                std::cerr << "Error creating new call object, unknown call ID" << std::endl;
                return 1;
            };
            auto ret = get_last_controller_to_web_call();

            if (ret == 1) {
                std::cerr << "no new call to process" << std::endl;
                return 1;
            }
            else if(ret == 2){
                std::cout << "api call ID mismatch" << std::endl;
                return 1;
            }
            else if(ret == 3){
                std::cerr << "data start index out of bounds" << std::endl;
                return 1;
            }
            else if(ret == 4){
                std::cerr << "data size is 0" << std::endl;
                return 1;
            }
            else if(ret != 0){
                std::cerr << "Error reading call from shared memory" << std::endl;
                return 1;
            }

            json call_data = json::object();

            completed_calls.back()->web_output_data(&call_data);

            (*data_out)[std::to_string(completed_calls.back()->api_call_number)] = call_data;
        }
        else {
            return 0;
        }
    }
    std::cerr << "WARNING: Unable to read all completed calls in single cycle" << std::endl;
    return 0;
}

uint32_t web_api::add_call(json* data, uint32_t* api_call_number) {

    if(!data->contains("call_name")){
        std::cerr << "Error: call_name not found in JSON" << std::endl;
        return 1;
    }
    if(!data->at("call_name").is_string()){
        std::cerr << "Error: call_name is not a string" << std::endl;
        return 1;
    }
           
    if(create_new_call_obj_from_string(&calls, data->at("call_name").get<std::string>(), api_call_number) != 0){
        std::cerr << "Error creating new call object" << std::endl;
        return 1;
    }

    if(calls.back()->web_input_data(data) != 0){
        std::cerr << "Error adding data to call object" << std::endl;
        remove_last_call_obj_from_string(&calls, api_call_number);   // remove failed call object
        return 1;
    }

    return 0;
}

uint32_t web_api::get_next_api_call_id_from_shared_mem() {
    // make sure there is a new call to process
    if (*(static_cast<uint32_t*>(default_call_obj.controller_to_web_control_base_address) + default_call_obj.controller_to_web_control_mem_index) != default_call_obj.controller_to_web_call_count + 1) {  // TODO: might want to make this more resilent so if it somehow gets out of alignment it can restart
        return 1;   // no new call to process
    }

    api_call_id = *(static_cast<api_call_id_t*>(default_call_obj.controller_to_web_control_base_address) + default_call_obj.controller_to_web_control_mem_index + 1);

    return 0;
}

uint32_t web_api::get_last_controller_to_web_call() {

        Base_API* call_obj = completed_calls.back().get();

        // make sure there is a new call to process
        if (*(static_cast<uint32_t*>(call_obj->controller_to_web_control_base_address) + call_obj->controller_to_web_control_mem_index) != call_obj->controller_to_web_call_count + 1) {  // TODO: might want to make this more resilent so if it somehow gets out of alignment it can restart
            return 1;   // no new call to process
        }

        call_obj->controller_to_web_call_count++;
        call_obj->persistent_web_mem[3] = call_obj->controller_to_web_call_count;

        if(call_obj->api_call_id != *(static_cast<uint32_t*>(call_obj->controller_to_web_control_base_address) + call_obj->controller_to_web_control_mem_index + 1)){
            return 2;   // error: api call ID does not match
        }

        // copy over data
        memcpy(&(call_obj->api_call_number), static_cast<uint32_t*>(call_obj->controller_to_web_control_base_address) + call_obj->controller_to_web_control_mem_index, sizeof(call_obj->api_call_number));
        call_obj->controller_to_web_control_mem_index += 2;
        memcpy(&(call_obj->data_start_index), static_cast<uint32_t*>(call_obj->controller_to_web_control_base_address) + call_obj->controller_to_web_control_mem_index, sizeof(call_obj->data_start_index));
        call_obj->controller_to_web_control_mem_index += 1;
        memcpy(&(call_obj->data_size), static_cast<uint32_t*>(call_obj->controller_to_web_control_base_address) + call_obj->controller_to_web_control_mem_index, sizeof(call_obj->data_size));
        call_obj->controller_to_web_control_mem_index += 1;

        if(call_obj->controller_to_web_control_mem_index >= call_obj->control_buffer_size/4){
            call_obj->controller_to_web_control_mem_index = 0;
        }

        if (call_obj->data_start_index >= call_obj->data_buffer_size) {
            return 3;   // error: data start index is out of bounds
        }

        if (call_obj->data_size == 0) {
            return 4;   // error: data size is 0
        }

        call_obj->controller_to_web_data_mem_index = call_obj->data_start_index;

        // read from shared memory into the call object
        if(call_obj->web_read_from_shared_mem() != 0){
            return 5;   // error: could not read from shared memory
        }

        return 0;
    }