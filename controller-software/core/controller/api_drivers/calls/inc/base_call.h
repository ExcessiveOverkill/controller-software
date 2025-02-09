#include <vector>
#include <memory> // For smart pointers
#include <string>
#include "json.hpp"

using json = nlohmann::json;

#pragma once
#include "shared_mem.h"

enum api_call_id_t {    // list of all possible API calls
    DEFAULT,
    MACHINE_STATE,
    PRINT_UINT32
};

// Base_API class
class Base_API {
public:

    static uint32_t data_buffer_size;
    static uint32_t control_buffer_size;

    static void * web_to_controller_data_base_address;
    static void * controller_to_web_data_base_address;

    static void * web_to_controller_control_base_address;
    static void * controller_to_web_control_base_address;

    static volatile uint32_t * persistent_web_mem;    // created by the contoller, but updated and read by the web side to keep track of the last data index (incase web side restarts)

    static uint32_t web_to_controller_data_mem_index;
    static uint32_t controller_to_web_data_mem_index;

    static uint32_t web_to_controller_control_mem_index;    // index of 32 bit values
    static uint32_t controller_to_web_control_mem_index;    // index of 32 bit values

    static uint32_t web_to_controller_call_count;
    static uint32_t controller_to_web_call_count;

    static uint32_t last_api_call_number;

    uint32_t api_call_number = 0;
    uint32_t api_call_id = 0;
    uint32_t data_start_index = 0;
    uint32_t data_size = 0;

    Base_API();

    virtual ~Base_API() = default;
    
    // controller side
    
    // writes the control info and data to shared memory
    virtual uint32_t controller_write_to_shared_mem();

    // reads the data from shared memory
    virtual uint32_t controller_read_from_shared_mem();

    // run the API call
    virtual uint32_t run();


    // web side
    
    // interprets a string and pointer to the associated data, then casts the pointer to the correct type and saves it to the object
    virtual uint32_t web_input_data(json* data);

    // interprets the data in the object and saves it to a json object
    virtual uint32_t web_output_data(json* data);

    // writes the control info and data to shared memory
    virtual uint32_t web_write_to_shared_mem();

    // reads the data from shared memory
    virtual uint32_t web_read_from_shared_mem();
    

    uint32_t update_web_to_controller_control_buffer(api_call_id_t api_call_id, uint32_t start_index, uint32_t size);  // web side


    uint32_t update_controller_to_web_control_buffer(api_call_id_t api_call_id, uint32_t start_index, uint32_t size);  // controller side


    uint32_t copy_to_web_to_controller_data_buffer(void* data_in, uint32_t size);  // web side


    uint32_t copy_to_controller_to_web_data_buffer(void* data_in, uint32_t size);  // controller side


    uint32_t copy_from_web_to_controller_data_buffer(void* data_out, uint32_t size);   // controller side


    uint32_t copy_from_controller_to_web_data_buffer(void* data_out, uint32_t size);   // web side
    

    uint32_t set_shared_memory(shared_mem shared_mem_obj);
    
};