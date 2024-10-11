#include "controller_api.h"
#include <chrono>
#include <thread>


int main() {

    controller_api api;

    while(1){

        api.get_new_call();
        api.run_calls();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}