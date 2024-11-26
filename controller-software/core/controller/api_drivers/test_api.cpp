#include "web_api.h"
#include <chrono>
#include <thread>

int main() {
    web_api api;

    uint32_t api_call_number;

    for(int c=0; c<1; c++){

    for(int i=0; i<8; i++){
        json call = {
            {"call_name", "print_uint32"},
            {"value", i + 8*c}
        };
        //std::cout << "value: " << i + 8*c << std::endl;
        api.add_call(&call, &api_call_number);
    }
    json call = {
        {"call_name", "print_uint32"},
        {"bad_key", 0}
    };
    //api.add_call(&call, &api_call_number);

    api.write_web_calls_to_controller();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    json response;
    api.get_completed_calls(&response);
    
    std::cout << response.dump(4) << std::endl;  // Pretty print with indentation

    }
    return 0;
}