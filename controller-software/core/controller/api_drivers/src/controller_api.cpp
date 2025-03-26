#include "controller_api.h"


controller_api::controller_api() {    
}

void controller_api::setup(Node_Core* node_core_) {
    node_core = node_core_;
    default_call_obj.node_core = node_core;

    if(shared_mem_obj.controller_create_shared_mem() !=0 ){
        std::cerr << "Error creating shared memory" << std::endl;
    }

    default_call_obj.set_shared_memory(shared_mem_obj);

    setup_json_parser();
}

void controller_api::setup_json_parser() {
    // json parser thread that will run in the background, this keeps slow json parsing from blocking the main thread

    std::thread json_parser(json_parser_thread);

    // set thread priority
    sched_param sch;
    int policy;
    pthread_getschedparam(json_parser.native_handle(), &policy, &sch);
    sch.sched_priority = 20;
    if (pthread_setschedparam(json_parser.native_handle(), SCHED_RR, &sch)) {
        std::cerr << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
    }

    json_parser.detach();

}

void controller_api::json_parser_thread() {
    while (true) {
        std::unique_lock<std::mutex> lock(controller_api::data_mutex);
        data_ready.wait(lock, [] { return controller_api::parsed_json != nullptr && controller_api::json_to_parse != nullptr && controller_api::json_parsed != nullptr; });
        lock.unlock();

        if(controller_api::json_to_parse == nullptr || controller_api::parsed_json == nullptr || controller_api::json_parsed == nullptr){
            continue;
        }

        // slow parsing operation
        *parsed_json = json::parse(*json_to_parse);

        *json_parsed = true;
        json_to_parse = nullptr;
        parsed_json = nullptr;
        json_parsed = nullptr;
        
        parsing_complete.store(true, std::memory_order_release);
    }
}

std::mutex controller_api::data_mutex;
std::condition_variable controller_api::data_ready;
std::string* controller_api::json_to_parse = nullptr;
json* controller_api::parsed_json = nullptr;
bool* controller_api::json_parsed = nullptr;
std::atomic<bool> controller_api::parsing_complete{false};

controller_api::~controller_api(){
    shared_mem_obj.unmap_shared_mem();
    shared_mem_obj.close_shared_mem();
}


uint32_t controller_api::get_new_call() {
    // read call from shared memory and create new call object

    if(get_next_api_call_id_from_shared_mem() == 0){
        //std::cout << "new call received" << std::endl;
        if(get_new_call_object_from_call_id(&calls, &api_call_id) == 1){
            std::cerr << "Error creating new call object, unknown call ID" << std::endl;
            return 2;
        };

        calls.back()->node_core = node_core;

        uint32_t ret = get_last_web_to_controller_call();

        if (ret == 1) {
            std::cerr << "no new call to process" << std::endl;
            return 3;
        }
        else if(ret == 2){
            std::cout << "api call ID mismatch" << std::endl;
            return 4;
        }
        else if(ret == 3){
            std::cerr << "data start index out of bounds" << std::endl;
            return 5;
        }
        else if(ret == 4){
            std::cerr << "data size is 0" << std::endl;
            return 6;
        }
        else if(ret != 0){
            std::cerr << "Error reading call from shared memory" << std::endl;
            return 7;
        }
        return 0;
    }
    return 1;   // no new call to process
}

uint32_t controller_api::run_calls() {
    
    for (auto it = calls.begin(); it != calls.end();) {

        if(!(*it)->json_parsed && (*it)->json_data_string != ""){
            {
                std::lock_guard<std::mutex> lock(controller_api::data_mutex);
                controller_api::json_to_parse = &(*it)->json_data_string;
                controller_api::parsed_json = &(*it)->json_data;
                controller_api::json_parsed = &(*it)->json_parsed;
            }
            controller_api::data_ready.notify_one();
        }

        uint32_t ret = (*it)->run();  // Run the call
        
        if (ret == 0) {     // call complete
            
            // write to shared memory then erase from vector
            (*it)->controller_write_to_shared_mem();
            it = calls.erase(it);
            continue;   // skip incrementing the iterator
        }
        else if (ret == 1) {    // call in progress
            
        }
        else {  // error
            std::cerr << "Unknown error running API call" << "\n";
        }

        ++it;
    }

    return 0;
}

uint32_t controller_api::get_next_api_call_id_from_shared_mem() {
    // make sure there is a new call to process
    if (*(static_cast<uint32_t*>(default_call_obj.web_to_controller_control_base_address) + default_call_obj.web_to_controller_control_mem_index) != default_call_obj.web_to_controller_call_count + 1) {  // TODO: might want to make this more resilent so if it somehow gets out of alignment it can restart
        return 1;   // no new call to process
    }

    api_call_id = *(static_cast<api_call_id_t*>(default_call_obj.web_to_controller_control_base_address) + default_call_obj.web_to_controller_control_mem_index + 1);

    return 0;
}

uint32_t controller_api::get_last_web_to_controller_call() {

    Base_API* call_obj = calls.back().get();

    // make sure there is a new call to process
    if (*(static_cast<uint32_t*>(call_obj->web_to_controller_control_base_address) + call_obj->web_to_controller_control_mem_index) != call_obj->web_to_controller_call_count + 1) {  // TODO: might want to make this more resilent so if it somehow gets out of alignment it can restart
        return 1;   // no new call to process
    }

    call_obj->web_to_controller_call_count++;

    if(call_obj->api_call_id != *(static_cast<uint32_t*>(call_obj->web_to_controller_control_base_address) + call_obj->web_to_controller_control_mem_index + 1)){
        return 2;   // error: api call ID does not match
    }

    // copy over data
    memcpy(&(call_obj->api_call_number), static_cast<uint32_t*>(call_obj->web_to_controller_control_base_address) + call_obj->web_to_controller_control_mem_index, sizeof(call_obj->api_call_number));
    call_obj->web_to_controller_control_mem_index += 2;
    memcpy(&(call_obj->data_start_index), static_cast<uint32_t*>(call_obj->web_to_controller_control_base_address) + call_obj->web_to_controller_control_mem_index, sizeof(call_obj->data_start_index));
    call_obj->web_to_controller_control_mem_index += 1;
    memcpy(&(call_obj->data_size), static_cast<uint32_t*>(call_obj->web_to_controller_control_base_address) + call_obj->web_to_controller_control_mem_index, sizeof(call_obj->data_size));
    call_obj->web_to_controller_control_mem_index += 1;

    if(call_obj->web_to_controller_control_mem_index >= call_obj->control_buffer_size/4){
        call_obj->web_to_controller_control_mem_index = 0;
    }

    if (call_obj->data_start_index >= call_obj->data_buffer_size) {
        return 3;   // error: data start index is out of bounds
    }

    if (call_obj->data_size == 0) {
        return 4;   // error: data size is 0
    }

    call_obj->web_to_controller_data_mem_index = call_obj->data_start_index;

    // read from shared memory into the call object
    if(call_obj->controller_read_from_shared_mem() != 0){
        return 5;   // error: could not read from shared memory
    }

    return 0;
}
