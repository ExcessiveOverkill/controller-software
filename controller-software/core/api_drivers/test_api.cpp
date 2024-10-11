#include "web_api.h"


int main() {
    web_api api;

    uint32_t api_call_number;

    json call = {
        {"call_name", "machine_state"},
        {"commanded_state", "on"}
    };

    api.add_call(&call, &api_call_number);
    api.add_call(&call, &api_call_number);
    api.add_call(&call, &api_call_number);
    api.add_call(&call, &api_call_number);

    // api.create_new_call_from_string("machine_state", &api_call_number);
    // api.add_data_to_call("commanded_state", "on");

    // api.create_new_call_from_string("machine_state", &api_call_number);
    // api.add_data_to_call("commanded_state", "off");
 
    // api.write_web_calls_to_controller();

    // api.create_new_call_from_string("machine_state", &api_call_number);
    // api.add_data_to_call("commanded_state", "on");

    // api.create_new_call_from_string("machine_state", &api_call_number);
    // api.add_data_to_call("commanded_state", "off");
 
    api.write_web_calls_to_controller();
    json response;
    api.get_completed_calls(&response);
    
    std::cout << response.dump(4) << std::endl;  // Pretty print with indentation
    return 0;
}