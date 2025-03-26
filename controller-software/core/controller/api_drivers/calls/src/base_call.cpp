#include "base_call.h"

uint32_t Base_API::update_web_to_controller_control_buffer(api_call_id_t api_call_id, uint32_t start_index, uint32_t size) {  // web side

    memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index + 1, &api_call_id, sizeof(api_call_id)); // api call ID

    start_index = start_index % data_buffer_size;
    memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index + 2, &start_index, sizeof(start_index)); // start index
    memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index + 3, &size, sizeof(size)); // size

    // write call number last to ensure the controller doesn't read the data before it is ready
    memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index, &api_call_number, sizeof(api_call_number)); // call number
    
    
    web_to_controller_control_mem_index += 4;

    if(web_to_controller_control_mem_index >= control_buffer_size/4){
        web_to_controller_control_mem_index = 0;
    }
    
    return 0;
}


uint32_t Base_API::update_controller_to_web_control_buffer(api_call_id_t api_call_id, uint32_t start_index, uint32_t size) {  // controller side

    memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index + 1, &api_call_id, sizeof(api_call_id)); // api call ID
    
    start_index = start_index % data_buffer_size;
    memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index + 2, &start_index, sizeof(start_index)); // start index
    memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index + 3, &size, sizeof(size)); // size

    // write call number last to ensure the web side doesn't read the data before it is ready
    memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index, &api_call_number, sizeof(api_call_number)); // call number
    
    
    controller_to_web_control_mem_index += 4;

    if(controller_to_web_control_mem_index >= control_buffer_size/4){
        controller_to_web_control_mem_index = 0;
    }
    
    return 0;
}


uint32_t Base_API::copy_to_web_to_controller_data_buffer(void* data_in, uint32_t size) {  // web side

    //TODO: check to make sure we don't overwrite data before it has been read

    if (size + web_to_controller_data_mem_index > data_buffer_size) { // check if we need to loop back to the beginning of the buffer
        memcpy(static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, data_in, data_buffer_size - web_to_controller_data_mem_index);    // copy part of the data to the end of the buffer
        memcpy(web_to_controller_data_base_address, static_cast<char*>(data_in) + (data_buffer_size - web_to_controller_data_mem_index), size - (data_buffer_size - web_to_controller_data_mem_index));    // copy the rest of the data to the beginning of the buffer
        web_to_controller_data_mem_index = size - (data_buffer_size - web_to_controller_data_mem_index);    // update the index
    }
    else {
        memcpy(static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, data_in, size);    // copy the data to the buffer
        web_to_controller_data_mem_index += size;    // update the index
        //web_to_controller_data_mem_index %= data_buffer_size;
    }
    persistent_web_mem[1] = web_to_controller_data_mem_index;    // update the persistent memory

    return 0;
}


uint32_t Base_API::copy_to_controller_to_web_data_buffer(void* data_in, uint32_t size) {  // controller side

    //TODO: check to make sure we don't overwrite data before it has been read

    if (size + controller_to_web_data_mem_index > data_buffer_size) { // check if we need to loop back to the beginning of the buffer
        memcpy(static_cast<char*>(controller_to_web_data_base_address) + controller_to_web_data_mem_index, data_in, data_buffer_size - controller_to_web_data_mem_index);    // copy part of the data to the end of the buffer
        memcpy(controller_to_web_data_base_address, static_cast<char*>(data_in) + (data_buffer_size - controller_to_web_data_mem_index), size - (data_buffer_size - controller_to_web_data_mem_index));    // copy the rest of the data to the beginning of the buffer
        controller_to_web_data_mem_index = size - (data_buffer_size - controller_to_web_data_mem_index);    // update the index
    }
    else {
        memcpy(static_cast<char*>(controller_to_web_data_base_address) + controller_to_web_data_mem_index, data_in, size);    // copy the data to the buffer
        controller_to_web_data_mem_index += size;    // update the index
    }

    return 0;
}


uint32_t Base_API::copy_from_web_to_controller_data_buffer(void* data_out, uint32_t size) {   // controller side

    if (size + web_to_controller_data_mem_index > data_buffer_size) { // check if we need to loop back to the beginning of the buffer
        memcpy(data_out, static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, data_buffer_size - web_to_controller_data_mem_index);    // copy part of the data from the end of the buffer
        memcpy(static_cast<char*>(data_out) + (data_buffer_size - web_to_controller_data_mem_index), web_to_controller_data_base_address, size - (data_buffer_size - web_to_controller_data_mem_index));    // copy the rest of the data from the beginning of the buffer
        web_to_controller_data_mem_index = size - (data_buffer_size - web_to_controller_data_mem_index);    // update the index
    }
    else {
        memcpy(data_out, static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, size);    // copy the data from the buffer
        web_to_controller_data_mem_index += size;    // update the index
        //web_to_controller_data_mem_index %= data_buffer_size;
    }

    return 0;
}


uint32_t Base_API::copy_from_controller_to_web_data_buffer(void* data_out, uint32_t size) {   // web side

    if (size + controller_to_web_data_mem_index > data_buffer_size) { // check if we need to loop back to the beginning of the buffer
        memcpy(data_out, static_cast<char*>(controller_to_web_data_base_address) + controller_to_web_data_mem_index, data_buffer_size - controller_to_web_data_mem_index);    // copy part of the data from the end of the buffer
        memcpy(static_cast<char*>(data_out) + (data_buffer_size - controller_to_web_data_mem_index), controller_to_web_data_base_address, size - (data_buffer_size - controller_to_web_data_mem_index));    // copy the rest of the data from the beginning of the buffer
        controller_to_web_data_mem_index = size - (data_buffer_size - controller_to_web_data_mem_index);    // update the index
    }
    else {
        memcpy(data_out, static_cast<char*>(controller_to_web_data_base_address) + controller_to_web_data_mem_index, size);    // copy the data from the buffer
        controller_to_web_data_mem_index += size;    // update the index
    }
    persistent_web_mem[2] = controller_to_web_data_mem_index;    // update the persistent memory

    return 0;
}

uint32_t Base_API::set_shared_memory(shared_mem shared_mem_obj){
    // set shared mem sizes
    data_buffer_size = shared_mem_obj.get_data_buffer_size();
    control_buffer_size = shared_mem_obj.get_control_buffer_size();

    // set shared mem addresses
    web_to_controller_data_base_address = shared_mem_obj.get_web_to_controller_data_mem();
    web_to_controller_control_base_address = shared_mem_obj.get_web_to_controller_control_mem();
    controller_to_web_data_base_address = shared_mem_obj.get_controller_to_web_data_mem();
    controller_to_web_control_base_address = shared_mem_obj.get_controller_to_web_control_mem();
    persistent_web_mem = static_cast<uint32_t*>(shared_mem_obj.get_persistent_web_mem());


    // set current call count and memory indexes from persistent memory

    web_to_controller_call_count = persistent_web_mem[0];
    controller_to_web_call_count = persistent_web_mem[3];
    web_to_controller_control_mem_index = (web_to_controller_call_count * 4) % (control_buffer_size/4);
    controller_to_web_control_mem_index = (controller_to_web_call_count * 4) % (control_buffer_size/4);
    web_to_controller_data_mem_index = persistent_web_mem[1];
    controller_to_web_data_mem_index = persistent_web_mem[2];
    
    return 0;
}

uint32_t Base_API::controller_write_to_shared_mem(){
    return 2;
}

uint32_t Base_API::controller_read_from_shared_mem(){
    return 2;
}

uint32_t Base_API::run(){
    return 2;
}

uint32_t Base_API::web_input_data(json* data){
    return 2;
}

uint32_t Base_API::web_output_data(json* data){
    return 2;
}

uint32_t Base_API::web_write_to_shared_mem(){
    return 2;
}

uint32_t Base_API::web_read_from_shared_mem(){
    return 2;
}

Base_API::Base_API(){
    api_call_id = api_call_id_t::DEFAULT;
}

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