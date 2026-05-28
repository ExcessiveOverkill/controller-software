#include "fanuc_encoders.h"

static Driver_Registrar<fanuc_encoders> registrar("fanuc_encoders");

uint32_t fanuc_encoders::custom_load_config(json* user_driver_config){

    cpy_time_reference(0);
    cpy_write_priority();
    cpy_execution_window(0, 1000);

    
    uint8_t encoder_count = 6;  // TODO: get this from the config file

    encoders.resize(encoder_count);
    for (uint8_t i = 0; i < encoder_count; i++) {
        auto encoder = loader.get_group("encoder", i);

        encoders[i].multiturn_count_reg = encoder->get_register("multiturn_count", 0);
        encoders[i].singleturn_count_reg = encoder->get_register("singleturn_count", 0);
        encoders[i].commutation_count_reg = encoder->get_register("commutation_count", 0);

        Register* status_reg = encoder->get_register("status", 0);
        encoders[i].status_reg = status_reg;
        encoders[i].crc_error_reg = status_reg->get_register("crc_fail");
        encoders[i].no_response_reg = status_reg->get_register("no_response");
        encoders[i].unindexed_reg = status_reg->get_register("unindexed");
        encoders[i].battery_fail_reg = status_reg->get_register("battery_fail");
        encoders[i].done_reg = status_reg->get_register("done");

        Register* config_reg = encoder->get_register("config", 0);
        encoders[i].config_reg = config_reg;
        encoders[i].rs485_mode_reg = config_reg->get_register("rs485_mode");
        encoders[i].encoder_type_reg = config_reg->get_register("encoder_type");

        std::string encoder_name = "encoder:" + std::to_string(i) + ".";

        cpy_source(encoders[i].singleturn_count_reg);
        cpy_destination(encoder_name+"singleturn_count:0");
        cpy_add_instruction();

        cpy_source(encoders[i].multiturn_count_reg);
        cpy_destination(encoder_name+"multiturn_count:0");
        cpy_add_instruction();

        cpy_source(encoders[i].commutation_count_reg);
        cpy_destination(encoder_name+"commutation_count:0");
        cpy_add_instruction();

        cpy_source(encoders[i].status_reg);
        cpy_destination(encoder_name+"status:0");
        cpy_add_instruction();

        cpy_destination(encoders[i].config_reg);
        cpy_source("config:"+std::to_string(i));
        cpy_add_instruction();

        // TODO: add other registers as needed


        node_core->create_global_variable("fanuc_encoders:"+std::to_string(i)+".total_turns", io_type::DOUBLE);
        node_core->set_global_variable_data_ptr("fanuc_encoders:"+std::to_string(i)+".total_turns", &(encoders[i].total_turns));

        node_core->create_global_variable("fanuc_encoders:"+std::to_string(i)+".unindexed", io_type::BOOL);
        node_core->set_global_variable_data_ptr("fanuc_encoders:"+std::to_string(i)+".unindexed", &(encoders[i].unindexed));

        std::string rs485_mode_str = "";
        try{
            rs485_mode_str = (*user_driver_config)["rs485_modes"][std::to_string(i)].get<std::string>();
        }
        catch(json::exception e){
            std::cerr << "Error: Invalid rs485 mode for encoder " << i << std::endl;
        }
        if(rs485_mode_str == "2WIRE"){
            encoders[i].rs485_mode = 1;
        }
        else if(rs485_mode_str == "4WIRE"){
            encoders[i].rs485_mode = 0;
        }
        else{
            std::cerr << "Error: Invalid rs485 mode for encoder " << i << ": " << rs485_mode_str << std::endl;
        }

        std::string encoder_type_str = "";
        try{
            encoder_type_str = (*user_driver_config)["encoder_types"][std::to_string(i)].get<std::string>();
        }
        catch(json::exception e){
            std::cerr << "Error: Invalid encoder type for encoder " << i << std::to_string(i) << std::endl;
        }
        if(encoder_type_str == "ai64"){
            encoders[i].encoder_type = 0;
        }
        else if(encoder_type_str == "ai128"){
            encoders[i].encoder_type = 1;
        }
        else{
            std::cerr << "Error: Invalid encoder type for encoder " << i << ": " << encoder_type_str << std::endl;
        }

    }

    cpy_time_reference(1001);

    trigger = loader.get_register("trigger", 0);
    cpy_destination(trigger);
    cpy_source("trigger");
    cpy_add_instruction();

    return 0;
}

void fanuc_encoders::convert_turns(encoder_data* data){

    // create double for multiturn + singleturn

    double fraction = double(data->singleturn_count) / double(singleturn_res);

        // Convert unsigned multiturn to signed (wrap-around handling)
    int64_t signed_multiturn;
    if (data->multiturn_count < half_multiturn_count) {
        signed_multiturn = int64_t(data->multiturn_count);
    } else {
        signed_multiturn = int64_t(data->multiturn_count) - int64_t(multiturn_res);
    }

    data->total_turns = double(signed_multiturn) + fraction;

    // Error if we've used the full ±half-range (i.e. ambiguous wrap)
    if(signed_multiturn <= -int64_t(half_multiturn_count) || signed_multiturn >= int64_t(half_multiturn_count)){
        data->turn_wrap = true;
    }

}

uint32_t fanuc_encoders::run(){

    trigger->set_value(0b111111);   // trigger 6 encoders
    
    for(uint8_t i = 0; i < encoders.size(); i++) {
        auto& data = encoders[i];

        data.multiturn_count = data.multiturn_count_reg->get_value<uint32_t>();
        data.singleturn_count = data.singleturn_count_reg->get_value<uint32_t>();

        data.rs485_mode_reg->set_value(data.rs485_mode);
        data.encoder_type_reg->set_value(data.encoder_type);

        data.commutation_angle = data.commutation_count_reg->get_value<uint16_t>();
        data.crc_error = data.crc_error_reg->get_value<bool>();
        data.no_response = data.no_response_reg->get_value<bool>();
        data.unindexed = data.unindexed_reg->get_value<bool>();
        data.battery_fail = data.battery_fail_reg->get_value<bool>();
        data.done = data.done_reg->get_value<bool>();

        convert_turns(&data);
    }

    return 0;
}