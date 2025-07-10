#pragma once
#include <string>
#include <vector>
#include "node_io.h"
#include <any>
#include "json.hpp"
using json = nlohmann::json;

// global variables are accessible by all node networks
// values do not persist between restarts


#include <memory>

class global_variable {
    private:
        io_type var_type = io_type::UNDEFINED;
        void* data_pointer = nullptr;
        std::string name;
        bool external_data = false;
        std::vector<void**> external_data_users;

        void delete_data_pointer(){
            switch (var_type){
                case io_type::UINT8:
                    delete (uint8_t*)data_pointer;
                    break;
                case io_type::INT8:
                    delete (int8_t*)data_pointer;
                    break;
                case io_type::UINT16:
                    delete (uint16_t*)data_pointer;
                    break;
                case io_type::INT16:
                    delete (int16_t*)data_pointer;
                    break;
                case io_type::UINT32:
                    delete (uint32_t*)data_pointer;
                    break;
                case io_type::INT32:
                    delete (int32_t*)data_pointer;
                    break;
                case io_type::DOUBLE:
                    delete (double*)data_pointer;
                    break;
                case io_type::FLOAT:
                    delete (float*)data_pointer;
                    break;
                case io_type::BOOL:
                    delete (bool*)data_pointer;
                    break;
                default:
                    break;  // TODO: add error handling
            }
        }

    public:
            
        global_variable(std::string name, io_type type_){
            this->name = name;
            var_type = type_;
            switch (var_type){
                case io_type::UINT8:
                    data_pointer = new uint8_t(0);
                    break;
                case io_type::INT8:
                    data_pointer = new int8_t(0);
                    break;
                case io_type::UINT16:
                    data_pointer = new uint16_t(0);
                    break;
                case io_type::INT16:
                    data_pointer = new int16_t(0);
                    break;
                case io_type::UINT32:
                    data_pointer = new uint32_t(0);
                    break;
                case io_type::INT32:
                    data_pointer = new int32_t(0);
                    break;
                case io_type::DOUBLE:
                    data_pointer = new double(0.0);
                    break;
                case io_type::BOOL:
                    data_pointer = new bool(false);
                    break;
                case io_type::FLOAT:
                    data_pointer = new float(0.0f);
                    break;
                case io_type::EM_SERIAL_DEVICE:
                    external_data = true;
                    break;
                default:
                    throw std::runtime_error("Invalid type");
                    break;  // TODO: add error handling
            }
        }

        void get_data_pointer(void** data){
            if(data == nullptr){
                return;
            }
            *data = data_pointer;
            external_data_users.push_back(data);    // save this so we know if the data is still in use and what to update if the data pointer changes
        }

        void set_data_pointer(void* data){
            if(!external_data){
                delete_data_pointer();
            }
            data_pointer = data;
            external_data = true;

            for(auto it = external_data_users.begin(); it != external_data_users.end(); it++){
                **it = data;
            }
        }

        uint32_t set_value(void* value){
            // TODO: probably should find a way to do this that is type safe, maybe with tempates for the correct types + checking?
            if(value == nullptr){
                return 1;
            }
            switch (var_type){
                case io_type::UINT8:
                    memcpy(data_pointer, value, sizeof(uint8_t));
                    break;
                case io_type::INT8:
                    memcpy(data_pointer, value, sizeof(int8_t));
                    break;
                case io_type::UINT16:
                    memcpy(data_pointer, value, sizeof(uint16_t));
                    break;
                case io_type::INT16:
                    memcpy(data_pointer, value, sizeof(int16_t));
                    break;
                case io_type::UINT32:
                    memcpy(data_pointer, value, sizeof(uint32_t));
                    break;
                case io_type::INT32:
                    memcpy(data_pointer, value, sizeof(int32_t));
                    break;
                case io_type::DOUBLE:
                    memcpy(data_pointer, value, sizeof(double));
                    break;
                case io_type::BOOL:
                    memcpy(data_pointer, value, sizeof(bool));
                    break;
                case io_type::FLOAT:
                    memcpy(data_pointer, value, sizeof(float));
                    break;
                default:
                    std::cerr << "Error: Cannot write to this variable type" << std::endl;
                    return 1;
                    break;
            }

            return 0;
        }

        io_type get_type(){
            return var_type;
        }

        json export_json(){
            json j;
            j["name"] = name;
            std::string type_str;
            switch (var_type){
                case io_type::UINT32:
                    type_str = "UINT32";
                    break;
                case io_type::INT32:
                    type_str = "INT32";
                    break;
                case io_type::DOUBLE:
                    type_str = "DOUBLE";
                    break;
                case io_type::BOOL:
                    type_str = "BOOL";
                    break;
                default:
                    type_str = "UNDEFINED";
                    break;  // TODO: add error handling
            }
            j["type"] = type_str;
            return j;
        }

        bool is_in_use(){
            if(external_data_users.size() > 0){
                return true;
            }
            return false;
        }

        ~global_variable(){
            if(is_in_use()){
                //std::cerr << "Error: global variable was still in use when deleted." << std::endl;
            }

            if(!external_data){
                delete_data_pointer();
            }
        }
};;
