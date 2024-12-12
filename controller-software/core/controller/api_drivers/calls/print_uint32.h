#include "base_call.h"

#pragma once

// print value to the console on the controller side
class print_uint32: public Base_API {
    private:
        uint32_t value = 0;

    public:
        print_uint32(){
            api_call_id = api_call_id_t::PRINT_UINT32;
        }
        
        unsigned int web_input_data(json* data) override {
            if(data->contains("value")){
                if(data->at("value").is_number_integer()){
                    value = data->at("value").get<uint32_t>();
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

        unsigned int web_output_data(json* data) override {
            data->emplace("call_name", "print_uint32");
            data->emplace("value", value);
            return 0;
        }

        unsigned int web_write_to_shared_mem() override {
            // write the call info to shared memory

            uint32_t start_index = web_to_controller_data_mem_index;
            uint32_t size = sizeof(value);

            copy_to_web_to_controller_data_buffer(&value, sizeof(value));

            update_web_to_controller_control_buffer(api_call_id_t::PRINT_UINT32, start_index, size);

            return 0;
        }

         unsigned int controller_read_from_shared_mem() override {
            // read the data from shared memory, layout must match the format set in web_write_to_shared_mem

            copy_from_web_to_controller_data_buffer(&value, sizeof(value));

            return 0;
        }

        unsigned int controller_write_to_shared_mem() override {
            // write the call info to shared memory, layout must match web_read_from_shared_mem

            uint32_t start_index = controller_to_web_data_mem_index;
            uint32_t size = sizeof(value);

            copy_to_controller_to_web_data_buffer(&value, sizeof(value));

            update_controller_to_web_control_buffer(api_call_id_t::PRINT_UINT32, start_index, size);

            return 0;
        }

        unsigned int web_read_from_shared_mem() override {
            // read the data from shared memory, layout must match the format set in controller_write_to_shared_mem

            copy_from_controller_to_web_data_buffer(&value, sizeof(value));

            return 0;
        }
        
        unsigned int run() override {
            // run the API call

            std::cout << "value: " << value << "\n";

            return 0;
        }
};
