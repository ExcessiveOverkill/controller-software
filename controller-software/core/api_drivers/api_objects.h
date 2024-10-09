#include <vector>
#include <memory> // For smart pointers
#include <string>
#include "shared_mem.h"

#pragma once

enum api_call_id_t {
    DEFAULT,
    MACHINE_STATE
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

    
    virtual ~Base_API() = default;
    
    // controller side

    // controller uses this to actually run the API call and interact with the rest of the controller
    
    // reads the data from shared memory
    virtual unsigned int controller_read_from_shared_mem() = 0;

    // run the API call
    virtual unsigned int run() = 0;


    // web side
    
    // interprets a string and pointer to the associated data, then casts the pointer to the correct type and saves it to the object
    virtual unsigned int web_input_data(std::string variable_name, std::string data) = 0;

    // writes the control info and data to shared memory
    virtual unsigned int web_write_to_shared_mem() = 0;

    // writes the control info and data to shared memory
    virtual unsigned int controller_write_to_shared_mem() = 0;


    uint32_t update_web_to_controller_control_buffer(api_call_id_t api_call_id, uint32_t start_index, uint32_t size) {  // web side

        memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index + 1, &api_call_id, sizeof(api_call_id)); // api call ID
        memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index + 2, &start_index, sizeof(start_index)); // start index
        memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index + 3, &size, sizeof(size)); // size

        // write call number last to ensure the controller doesn't read the data before it is ready
        memcpy(static_cast<uint32_t*>(web_to_controller_control_base_address) + web_to_controller_control_mem_index, &api_call_number, sizeof(api_call_number)); // call number
        
        
        web_to_controller_control_mem_index += 4;

        if(web_to_controller_control_mem_index >= control_buffer_size){
            web_to_controller_control_mem_index = 0;
        }
        
        return 0;
    }


    uint32_t update_controller_to_web_control_buffer(api_call_id_t api_call_id, uint32_t start_index, uint32_t size) {  // controller side

        memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index + 1, &api_call_id, sizeof(api_call_id)); // api call ID
        memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index + 2, &start_index, sizeof(start_index)); // start index
        memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index + 3, &size, sizeof(size)); // size

        // write call number last to ensure the web side doesn't read the data before it is ready
        memcpy(static_cast<uint32_t*>(controller_to_web_control_base_address) + controller_to_web_control_mem_index, &api_call_number, sizeof(api_call_number)); // call number
        
        
        controller_to_web_control_mem_index += 4;

        if(controller_to_web_control_mem_index >= control_buffer_size){
            controller_to_web_control_mem_index = 0;
        }
        
        return 0;
    }


    uint32_t copy_to_web_to_controller_data_buffer(void* data_in, uint32_t size) {  // web side

        //TODO: check to make sure we don't overwrite data before it has been read

        if (size + web_to_controller_data_mem_index > data_buffer_size) { // check if we need to loop back to the beginning of the buffer
            memcpy(static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, data_in, data_buffer_size - web_to_controller_data_mem_index);    // copy part of the data to the end of the buffer
            memcpy(web_to_controller_data_base_address, static_cast<char*>(data_in) + (data_buffer_size - web_to_controller_data_mem_index), size - (data_buffer_size - web_to_controller_data_mem_index));    // copy the rest of the data to the beginning of the buffer
            web_to_controller_data_mem_index = size - (data_buffer_size - web_to_controller_data_mem_index);    // update the index
        }
        else {
            memcpy(static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, data_in, size);    // copy the data to the buffer
            web_to_controller_data_mem_index += size;    // update the index
        }

        return 0;
    }


    uint32_t copy_to_controller_to_web_data_buffer(void* data_in, uint32_t size) {  // controller side

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


    uint32_t copy_from_web_to_controller_data_buffer(void* data_out, uint32_t size) {   // controller side

        if (size + web_to_controller_data_mem_index > data_buffer_size) { // check if we need to loop back to the beginning of the buffer
            memcpy(data_out, static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, data_buffer_size - web_to_controller_data_mem_index);    // copy part of the data from the end of the buffer
            memcpy(static_cast<char*>(data_out) + (data_buffer_size - web_to_controller_data_mem_index), web_to_controller_data_base_address, size - (data_buffer_size - web_to_controller_data_mem_index));    // copy the rest of the data from the beginning of the buffer
            web_to_controller_data_mem_index = size - (data_buffer_size - web_to_controller_data_mem_index);    // update the index
        }
        else {
            memcpy(data_out, static_cast<char*>(web_to_controller_data_base_address) + web_to_controller_data_mem_index, size);    // copy the data from the buffer
            web_to_controller_data_mem_index += size;    // update the index
        }

        return 0;
    }
    

    uint32_t set_shared_memory(shared_mem shared_mem_obj){
        // set shared mem sizes
        data_buffer_size = shared_mem_obj.get_data_buffer_size();
        control_buffer_size = shared_mem_obj.get_control_buffer_size();

        // set shared mem addresses
        web_to_controller_data_base_address = shared_mem_obj.get_web_to_controller_data_mem();
        web_to_controller_control_base_address = shared_mem_obj.get_web_to_controller_control_mem();
        controller_to_web_data_base_address = shared_mem_obj.get_controller_to_web_data_mem();
        controller_to_web_control_base_address = shared_mem_obj.get_controller_to_web_control_mem();

        return 0;
    }
    
};



// default
class default_call: public Base_API {
    private:
    public:
        default_call(){
            api_call_id = api_call_id_t::DEFAULT;
        }

        unsigned int web_input_data(std::string variable_name, std::string data) override {
            return 2;
        }

        unsigned int web_write_to_shared_mem() override {
            return 2;
        }

        unsigned int controller_write_to_shared_mem() override {
            return 2;
        }

        unsigned int controller_read_from_shared_mem() override {
            return 2;
        }

        unsigned int run() override {
            return 2;
        }
};

// get/set machione state
class machine_state: public Base_API {
    private:
        

        enum machine_state_t {
            NONE,
            ON,
            OFF
        };

        machine_state_t command_state = NONE;
        machine_state_t requested_state = NONE;
        machine_state_t current_state = NONE;

        unsigned int controller_read_from_shared_mem() override {
            // read the data from shared memory (data format must match the format set in web_write_to_shared_mem)

            copy_from_web_to_controller_data_buffer(&command_state, sizeof(command_state));

            return 0;
        }

        

    public:
        machine_state(){
            api_call_id = api_call_id_t::MACHINE_STATE;
        }
        
        unsigned int web_input_data(std::string variable_name, std::string data) override {
            if (variable_name.compare("commanded_state") == 0) {

                if (data.compare("") == 0) {
                    command_state = NONE;
                }
                else if (data.compare("on") == 0) {
                    command_state = ON;
                }
                else if (data.compare("off") == 0) {
                    command_state = OFF;
                }
                else {
                    return 1;
                }
            }
            else {
                return 1;
            }

            return 0;
        }

        unsigned int web_write_to_shared_mem() override {
            // write the call info to shared memory

            uint32_t start_index = web_to_controller_data_mem_index;
            uint32_t size = sizeof(command_state);

            copy_to_web_to_controller_data_buffer(&command_state, sizeof(command_state));

            update_web_to_controller_control_buffer(api_call_id_t::MACHINE_STATE, start_index, size);

            return 0;
        }

        unsigned int controller_write_to_shared_mem() override {
            // write the call info to shared memory

            uint32_t start_index = controller_to_web_data_mem_index;
            uint32_t size = sizeof(command_state);

            copy_to_controller_to_web_data_buffer(&command_state, sizeof(command_state));

            update_controller_to_web_control_buffer(api_call_id_t::MACHINE_STATE, start_index, size);

            return 0;
        }

        unsigned int run() override {
            // run the API call

            // check if the command state is different from the current state
            if (command_state != machine_state_t::NONE && command_state != current_state) {
                std::cout << "requested state changed to: " << command_state << "\n";
                requested_state = command_state;

                current_state = requested_state;
                requested_state = machine_state_t::NONE;
                std::cout << "current state changed to: " << current_state << "\n";
            }

            return 0;
        }
};


uint32_t write_web_calls_to_controller(std::vector<std::shared_ptr<Base_API>> calls) {
    for (const auto& obj : calls) {
        obj->web_write_to_shared_mem();
    }
    return 0;
}


uint32_t get_new_call_object_from_string(std::vector<std::shared_ptr<Base_API>>* calls, std::string api_call_name) {
    if(api_call_name.compare("machine_state") == 0){
        calls->push_back(std::make_shared<machine_state>());
        calls->back()->api_call_number = Base_API::web_to_controller_call_count + 1;
        Base_API::web_to_controller_call_count++;
    }
    else {
        return 1;
    }
    return 0;
}

uint32_t get_new_call_object_from_call_id(std::vector<std::shared_ptr<Base_API>>* calls, api_call_id_t api_call_id) {
    if(api_call_id == api_call_id_t::DEFAULT){
        calls->push_back(std::make_shared<default_call>());
    }
    else if(api_call_id == api_call_id_t::MACHINE_STATE){
        calls->push_back(std::make_shared<machine_state>());
    }
    else {
        return 1;   // error: unknown call ID
    }

    return 0;
}

uint32_t get_next_api_call_id_from_shared_mem(Base_API* call_obj, api_call_id_t* new_call_id) {
    // make sure there is a new call to process
    if (*(static_cast<uint32_t*>(call_obj->web_to_controller_control_base_address) + call_obj->web_to_controller_control_mem_index) != call_obj->web_to_controller_call_count + 1) {  // TODO: might want to make this more resilent so if it somehow gets out of alignment it can restart
        return 1;   // no new call to process
    }

    *new_call_id = *(static_cast<api_call_id_t*>(call_obj->web_to_controller_control_base_address) + call_obj->web_to_controller_control_mem_index + 1);

    return 0;
}

uint32_t get_last_web_to_controller_call(std::vector<std::shared_ptr<Base_API>>* calls) {

        Base_API* call_obj = calls->back().get();

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