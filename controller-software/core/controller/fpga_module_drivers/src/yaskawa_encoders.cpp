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


    auto encoder = loader.get_group("encoder", 0);
    multiturn_count = encoder->get_register("multiturn_count", 0);
    singleturn_count = encoder->get_register("singleturn_count", 0);

    // node_core->create_global_variable("singleturn_count", io_type::UINT32);
    // node_core->set_global_variable_data_ptr("singleturn_count", &(data.singleturn_count));
    // node_core->create_global_variable("multiturn_count", io_type::UINT32);
    // node_core->set_global_variable_data_ptr("multiturn_count", &(data.multiturn_count));

    cpy_source(singleturn_count);
    cpy_destination("singleturn_count");
    cpy_add_instruction();

    cpy_source(multiturn_count);
    cpy_destination("multiturn_count");
    cpy_add_instruction();


    node_core->create_global_variable("total_turns", io_type::DOUBLE);
    node_core->set_global_variable_data_ptr("total_turns", &(data.total_turns));

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

    data.singleturn_count = singleturn_count->get_value<uint32_t>();
    data.multiturn_count = multiturn_count->get_value<uint32_t>();

    convert_turns(&data);

    return 0;
}