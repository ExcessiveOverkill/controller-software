#include <string>
#include <vector>
#include "../node_io.h"
#include <any>

// global variables are accessible by all node networks
// values do not persist between restarts


#include <memory>

class global_variable {
    private:
        io_type var_type = io_type::UNDEFINED;
        std::shared_ptr<void> data_pointer = nullptr;
        //std::string name;

    public:
            
        global_variable(io_type type_){
            //name = name_;
            var_type = type_;

            switch (var_type)
            {
            case io_type::UINT32:
                data_pointer = std::make_shared<uint32_t>(0);
                break;
            case io_type::INT32:
                data_pointer = std::make_shared<int32_t>(0);
                break;
            case io_type::DOUBLE:
                data_pointer = std::make_shared<double>(0.0);
                break;
            case io_type::BOOL:
                data_pointer = std::make_shared<bool>(false);
                break;
            default:
                break;  // TODO: add error handling
            }
        }

        // uint32_t set_name(std::string name_){
        //     name = name_;
        //     return 0;
        // }

        std::shared_ptr<void> get_data_pointer(){
            return data_pointer;
        }

        bool is_in_use(){
            if(data_pointer.use_count() > 1){
                return true;
            }
            return false;
        }

        ~global_variable(){
            if(is_in_use()){
                std::cerr << "Error: global variable was still in use when deleted." << std::endl;
            }
        }
};;
