#include "yaskawa_encoders.h"

static Driver_Registrar<yaskawa_encoders> registrar("yaskawa_encoders");

uint32_t yaskawa_encoders::custom_load_config(json* user_driver_config){

    cpy_time_reference(0);
    cpy_write_priority();
    cpy_execution_window(0, 1000);

    req_packet = loader.get_register("request_packet", 0);
    cpy_destination(req_packet);
    cpy_source("request_packet");
    cpy_add_instruction();

    uint8_t encoder_count = 6;  // TODO: get this from the config file

    encoders.resize(encoder_count);
    for (uint8_t i = 0; i < encoder_count; i++) {
        auto encoder = loader.get_group("encoder", i);

        encoders[i].multiturn_count_reg = encoder->get_register("multiturn_count", 0);
        encoders[i].singleturn_count_reg = encoder->get_register("singleturn_count", 0);
        // encoders[i].commutation_count_reg = encoder->get_register("commutation_count", 0);
        // encoders[i].crc_error_reg = encoder->get_register("crc_error", 0);
        // encoders[i].no_response_reg = encoder->get_register("no_response", 0);
        // encoders[i].unindexed_reg = encoder->get_register("unindexed", 0);
        // encoders[i].battery_fail_reg = encoder->get_register("battery_fail", 0);
        // encoders[i].done_reg = encoder->get_register("done", 0);

        std::string encoder_name = "encoder:" + std::to_string(i) + ".";

        cpy_source(encoders[i].singleturn_count_reg);
        cpy_destination(encoder_name+"singleturn_count:0");
        cpy_add_instruction();

        cpy_source(encoders[i].multiturn_count_reg);
        cpy_destination(encoder_name+"multiturn_count:0");
        cpy_add_instruction();

        // TODO: add other registers as needed


        node_core->create_global_variable("yaskawa_encoders:"+std::to_string(i)+".total_turns", io_type::DOUBLE);
        node_core->set_global_variable_data_ptr("yaskawa_encoders:"+std::to_string(i)+".total_turns", &(encoders[i].total_turns));

    }


    cpy_time_reference(1001);

    trigger = loader.get_register("trigger", 0);
    cpy_destination(trigger);
    cpy_source("trigger");
    cpy_add_instruction();

    return 0;
}

void yaskawa_encoders::convert_turns(encoder_data* data){

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

uint32_t yaskawa_encoders::run(){

    req_packet->set_value(0xffff);

    for(uint8_t i = 0; i < encoders.size(); i++) {
        auto& data = encoders[i];

        data.multiturn_count = data.multiturn_count_reg->get_value<uint32_t>();
        data.singleturn_count = data.singleturn_count_reg->get_value<uint32_t>();

        // data.commutation_angle = data.commutation_count_reg->get_value<uint16_t>();
        // data.crc_error = data.crc_error_reg->get_value<bool>();
        // data.no_resonse = data.no_response_reg->get_value<bool>();
        // data.unindexed = data.unindexed_reg->get_value<bool>();
        // data.battery_fail = data.battery_fail_reg->get_value<bool>();
        // data.done = data.done_reg->get_value<bool>();

        convert_turns(&data);
    }

    return 0;
}