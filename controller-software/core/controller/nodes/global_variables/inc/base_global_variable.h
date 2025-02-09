#include <string>
#include <vector>
#include "node_io.h"
#include <any>

// global variables are accessible by all node networks
// values do not persist between restarts


#include <memory>

class global_variable {
    private:
        io_type var_type = io_type::UNDEFINED;
        void* data_pointer = nullptr;
        std::string name;
        bool external_data = false;

    public:
            
        global_variable(std::string name, io_type type_, void* ext_data_pointer){
            this->name = name;
            var_type = type_;

            if(ext_data_pointer != nullptr){    // use external data if pointer is provided
                data_pointer = ext_data_pointer;
                external_data = true;
            }
            else{   // create new data to point to
                switch (var_type){
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
                    default:
                        break;  // TODO: add error handling
                }
            }
        }

        void* get_data_pointer(){
            return data_pointer;
        }

        bool is_in_use(){
            // TODO: implement
            return false;
        }

        ~global_variable(){
            if(is_in_use()){
                std::cerr << "Error: global variable was still in use when deleted." << std::endl;
            }

            if(!external_data){
                switch (var_type){
                    case io_type::UINT32:
                        delete (uint32_t*)data_pointer;
                        break;
                    case io_type::INT32:
                        delete (int32_t*)data_pointer;
                        break;
                    case io_type::DOUBLE:
                        delete (double*)data_pointer;
                        break;
                    case io_type::BOOL:
                        delete (bool*)data_pointer;
                        break;
                    default:
                        break;  // TODO: add error handling
                }
            }
        }
};;
