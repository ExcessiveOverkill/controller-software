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
    
    if(shared_mem_obj.controller_create_shared_mem() !=0 ){
        std::cerr << "Error creating shared memory" << std::endl;
        return 1;
    }
    
    default_call default_call_obj;
    default_call_obj.set_shared_memory(shared_mem_obj);

    // Create a vector of Base_API pointers
    std::vector<std::shared_ptr<Base_API>> calls;

    api_call_id_t api_call_id = api_call_id_t::DEFAULT;

    while(1){
        if(get_next_api_call_id_from_shared_mem(&default_call_obj, &api_call_id) == 0){
            std::cout << "new call received" << std::endl;
            if(get_new_call_object_from_call_id(&calls, api_call_id) == 1){
                std::cerr << "Error creating new call object, unknown call ID" << std::endl;
                return 1;
            };
            auto ret = get_last_web_to_controller_call(&calls);

            if (ret == 1) {
                std::cerr << "no new call to process" << std::endl;
                return 1;
            }
            else if(ret == 2){
                std::cout << "api call ID mismatch" << std::endl;
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
        }

        for (auto it = calls.begin(); it != calls.end(); /* no increment here */) {
            auto ret = (*it)->run();  // Run the API call
            
            if (ret == 0) {
                std::cout << "API call completed, writing data to shared mem" << "\n";
                // TODO: write data to shared mem here
                (*it)->controller_write_to_shared_mem();

                // Erase the call and get the new iterator pointing to the next element
                it = calls.erase(it);  // 'erase' returns an iterator to the next element
            }
            else if (ret == 1) {
                std::cout << "Running API call in progress" << "\n";
                ++it;  // Manually increment the iterator if not erasing
            }
            else {
                std::cerr << "Unknown error running API call" << "\n";
                ++it;  // Manually increment the iterator if not erasing
            }
        }

        Sleep(100);
    }

    shared_mem_obj.close_shared_mem();
    return 0;
}