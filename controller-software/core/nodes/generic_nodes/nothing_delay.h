#include "../base_node.h"
#include <chrono>
#include <thread>

class nothing_delay: public base_node {
    private:
        bool default_input_value = false;
        bool output_value = true;

        input* input_value = nullptr;
    public:

        nothing_delay(){
            inputs.emplace("input", input(io_type::BOOL, &default_input_value));

            outputs.emplace("output", output(io_type::BOOL, &output_value, &execution_number));
            
            input_value = &inputs["input"];
        }

        unsigned int run() override {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            output_value = (*reinterpret_cast<bool*>(input_value->data_pointer));
            return 0;
        }

};