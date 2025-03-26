#pragma once

#include "base_call.h"

// configure a node in the node system
class configure_node: public Base_API {
    private:
        uint32_t config_size = 0;

    public:
        configure_node(){
            api_call_id = api_call_id_t::CONFIGURE_NODE;
        }
        
        unsigned int web_input_data(json* data) override {

            if(!data->contains("value")){
                return 1;
            }

            json_data = data->at("value");

            if(!json_data.contains("node_network")){
                return 2;
            }
            if(!json_data.contains("node_name")){
                return 2;
            }
            if(!json_data.contains("config")){
                return 2;
            }

            return 0;
        }

        unsigned int web_output_data(json* data) override {
            data->emplace("call_name", "configure_node");
            data->emplace("value", json_data);
            return 0;
        }

        unsigned int web_write_to_shared_mem() override {
            // write the call info to shared memory

            uint32_t start_index = web_to_controller_data_mem_index;
            std::string con = json_data.dump();
            //std::cout << "con: " << con << std::endl;
            config_size = con.size();
            //std::cout << "config_size: " << config_size << std::endl;

            uint32_t size = config_size + sizeof(config_size);

            copy_to_web_to_controller_data_buffer(&config_size, sizeof(config_size));
            copy_to_web_to_controller_data_buffer(con.data(), config_size);

            update_web_to_controller_control_buffer(api_call_id_t::CONFIGURE_NODE, start_index, size);

            return 0;
        }

         unsigned int controller_read_from_shared_mem() override {
            // read the data from shared memory, layout must match the format set in web_write_to_shared_mem
            copy_from_web_to_controller_data_buffer(&config_size, sizeof(config_size));
            char* con_char = new char[config_size];
            copy_from_web_to_controller_data_buffer(con_char, config_size);
            json_data_string = std::string(con_char, config_size);
            delete[] con_char;

            json_parsed = false;

            return 0;
        }

        unsigned int controller_write_to_shared_mem() override {
            // write the call info to shared memory, layout must match web_read_from_shared_mem

            uint32_t start_index = controller_to_web_data_mem_index;
            std::string con = json_data.dump();
            config_size = con.size();

            uint32_t size = config_size + sizeof(config_size);

            copy_to_controller_to_web_data_buffer(&config_size, sizeof(config_size));
            copy_to_controller_to_web_data_buffer(con.data(), config_size);

            update_controller_to_web_control_buffer(api_call_id_t::CONFIGURE_NODE, start_index, size);

            return 0;
        }

        unsigned int web_read_from_shared_mem() override {
            // read the data from shared memory, layout must match the format set in controller_write_to_shared_mem
            std::string con;
            copy_from_controller_to_web_data_buffer(&config_size, sizeof(config_size));
            char* con_char = new char[config_size];
            copy_from_controller_to_web_data_buffer(con_char, config_size);
            con = std::string(con_char, config_size);
            delete[] con_char;

            // web side doesn't use a separate parsing thread
            json_data = json::parse(con);

            return 0;
        }
        
        unsigned int run() override {

            if(!json_parsed){
                return 1;   // signal not done
            }
            
            node_core->configure_node(json_data.at("node_network"), json_data.at("node_name"), &json_data.at("config"));

            return 0;
        }
};
