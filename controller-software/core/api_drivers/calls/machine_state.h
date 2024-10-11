#include "base_call.h"

#pragma once

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

        
    public:
        machine_state(){
            api_call_id = api_call_id_t::MACHINE_STATE;
        }
        
        unsigned int web_input_data(json* data) override {
            command_state = NONE;
            if(data->contains("commanded_state")){
                
                if(data->at("commanded_state").is_string()){
                    if(data->at("commanded_state").get<std::string>().compare("on") == 0){
                        command_state = ON;
                    }
                    else if(data->at("commanded_state").get<std::string>().compare("off") == 0){
                        command_state = OFF;
                    }
                    else {
                        return 1;
                    }
                }
                else {
                    return 1;
                }
            }

            return 0;
        }

        unsigned int web_output_data(json* data) override {
            data->emplace("call_name", "machine_state");

            if(current_state == ON){
                data->emplace("current_state", "on");
            }
            else if(current_state == OFF){
                data->emplace("current_state", "off");
            }
            else if(current_state == NONE){
                data->emplace("current_state", "none");
            }
            else {
                data->emplace("current_state", "unknown");
            }

            if(requested_state == ON){
                data->emplace("requested_state", "on");
            }
            else if(requested_state == OFF){
                data->emplace("requested_state", "off");
            }
            else if(requested_state == NONE){
                data->emplace("requested_state", "none");
            }
            else {
                data->emplace("requested_state", "unknown");
            }

            if(command_state == ON){
                data->emplace("commanded_state", "on");
            }
            else if(command_state == OFF){
                data->emplace("commanded_state", "off");
            }
            else if(command_state == NONE){
                data->emplace("commanded_state", "none");
            }
            else {
                data->emplace("commanded_state", "unknown");
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

         unsigned int controller_read_from_shared_mem() override {
            // read the data from shared memory, layout must match the format set in web_write_to_shared_mem

            copy_from_web_to_controller_data_buffer(&command_state, sizeof(command_state));

            return 0;
        }

        unsigned int controller_write_to_shared_mem() override {
            // write the call info to shared memory, layout must match web_read_from_shared_mem

            uint32_t start_index = controller_to_web_data_mem_index;
            uint32_t size = sizeof(command_state) + sizeof(current_state) + sizeof(requested_state);

            copy_to_controller_to_web_data_buffer(&command_state, sizeof(command_state));
            copy_to_controller_to_web_data_buffer(&current_state, sizeof(current_state));
            copy_to_controller_to_web_data_buffer(&requested_state, sizeof(requested_state));

            update_controller_to_web_control_buffer(api_call_id_t::MACHINE_STATE, start_index, size);

            return 0;
        }

        unsigned int web_read_from_shared_mem() override {
            // read the data from shared memory, layout must match the format set in controller_write_to_shared_mem

            copy_from_controller_to_web_data_buffer(&command_state, sizeof(command_state));
            copy_from_controller_to_web_data_buffer(&current_state, sizeof(current_state));
            copy_from_controller_to_web_data_buffer(&requested_state, sizeof(requested_state));

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
