#include "global_timers.h"

static Driver_Registrar<global_timers> registrar("global_timers");

uint32_t global_timers::load_config(json config, std::vector<uint64_t>* instructions){

    if(!load_json_value(config, "node_address", &node_address)){
        std::cerr << "Failed to load node address" << std::endl;
        return 1;
    }

    // configure registers
    Address_Map_Loader loader;
    loader.setup(&config, &base_mem, node_address, instructions);

    // timer_values.push_back(loader.get_register("counter", 0));
    // loader.sync_with_PS(timer_values[0]);

    // timer_values[0]->set_value(100);

    // test print instructions
    // for (auto& inst : *instructions){
    //     std::cout << "Instruction: " << inst << std::endl;
    // }

    return 0;
}

uint32_t global_timers::run(){
    // do nothing for now
    return 0;
}