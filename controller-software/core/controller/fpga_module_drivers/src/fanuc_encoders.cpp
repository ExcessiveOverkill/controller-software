#include "fanuc_encoders.h"

static Driver_Registrar<fanuc_encoders> registrar("fanuc_encoders");

uint32_t fanuc_encoders::load_config(json config, std::vector<uint64_t>* instructions){

    if(!load_json_value(config, "node_address", &node_address)){
        std::cerr << "Failed to load node address" << std::endl;
        return 1;
    }

    // configure registers
    Address_Map_Loader loader;
    loader.setup(&config, &base_mem, node_address, instructions);


    auto encoder = loader.get_group("encoder", 0);
    multiturn_count = encoder->get_register("multiturn_count", 0);
    loader.sync_with_PS(multiturn_count);

    singleturn_count = encoder->get_register("singleturn_count", 0);
    loader.sync_with_PS(singleturn_count);
    commutation_count = encoder->get_register("commutation_count", 0);
    loader.sync_with_PS(commutation_count);
    auto status = encoder->get_register("status", 0);
    loader.sync_with_PS(status);
    battery_fail = status->get_register("battery_fail");
    unindexed = status->get_register("unindexed");
    no_response = status->get_register("no_response");
    crc_error = status->get_register("crc_fail");
    done = status->get_register("done");

    // test print instructions
    // for (auto& inst : *instructions){
    //     std::cout << "Instruction: " << inst << std::endl;
    // }

    encoder_pos = singleturn_count->get_raw_data_ptr<uint32_t>();
    encoder_multiturn_count = multiturn_count->get_raw_data_ptr<uint32_t>();

    return 0;
}

uint32_t fanuc_encoders::run(){
    
    if(*microseconds > old_microseconds + 1000000){
        // std::cout << "Encoder multiturn count: " << multiturn_count->get_value() << std::endl;
        std::cout << "Encoder single turn count: " << (singleturn_count->get_value()) << std::endl;
        // std::cout << "Encoder commutation count: " << commutation_count->get_value() << std::endl;
        //std::cout << "Encoder CRC error: " << crc_error->get_value() << std::endl;
        //std::cout << "Encoder no response: " << no_response->get_value() << std::endl;
        //std::cout << "Encoder unindexed: " << unindexed->get_value() << std::endl;
        //std::cout << "Encoder battery fail: " << battery_fail->get_value() << std::endl;
        //std::cout << "Encoder done: " << done->get_value() << std::endl << std::endl;
        old_microseconds = *microseconds;
    }

    return 0;
}