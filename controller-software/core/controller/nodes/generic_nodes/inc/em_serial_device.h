#include <stdint.h>
#include <string>
#include "node_factory.h"
#include <filesystem>
#include <fstream>
#include "em_serial_controller.h"

#pragma once

// make everything from a device available to the node system

class em_serial_device: public base_node{
    public:

        bool device_error = false;
        bool device_warning = false;
        bool communication_error = false;

        em_serial_device(){
            //execution_number = -1;

            inputs.emplace("em_device", input(io_type::EM_SERIAL_DEVICE, nullptr));    // connection to em serial device FPGA driver
            //outputs.emplace("device_error", output(io_type::BOOL, &device_error, &execution_number));    // error from the device
            //outputs.emplace("device_warning", output(io_type::BOOL, &device_warning, &execution_number));    // warning from the device
            outputs.emplace("communication_error", output(io_type::BOOL, &communication_error, &execution_number));    // communication error (timeout)
        }

        uint32_t run() override {

            if(!driver_setup_done){
                initial_driver_setup();
                return 0;
            }

            if(!enabled){
                return 0;
            }


            // if(device->fault.crc_invalid || device->fault.no_response || device->fault.response_not_finished){
            //     comm_fault_count++;
            // }
            // else{
            //     comm_fault_count = 0;
            // }

            //device_protocol_fault_count = device->consecutive_packet_errors + device->consecutive_unknown_packet_errors;    // unused at the moment

            if(device->consecutive_packet_errors == comm.timeout_tries){
                communication_error = true;
                if(current_state == state::RUN){
                    std::cerr << "Error: EM serial device " << device->get_full_name() << " communication error, attempting to reconnect..." << std::endl;

                    if(firmware_buffer != nullptr){
                        std::cerr << "Error: connection lost while transfering firmware, update aborted, restart device before retrying" << std::endl;
                        firmware_offset = 0;
                        firmware_total_percent = 0;
                        firmware_write_wait = false;
                        delete[] firmware_buffer;
                        firmware_buffer = nullptr;
                    }
                }
                if(current_state != state::DISABLE_CYCLIC_MODE){
                    device->clear_pending_sequential_cmds();
                    device->force_disable_cyclic_data();
                    current_state = state::DISABLE_CYCLIC_MODE;
                }
                
            }
            else if(device->consecutive_packet_errors == 0){
                communication_error = false;
            }


            switch (current_state)
            {
            case state::INITIAL_CONFIG:
                if(enabled && device->set_enabled(true) == 0){
                    current_state = state::DISABLE_CYCLIC_MODE;
                }
                break;

            case state::DISABLE_CYCLIC_MODE:
                if(device->get_sequential_cmds_complete() && !device->get_cyclic_data_enabled()){
                    device->rerun_cylic_config();
                    current_state = state::CONFIGURE_CYCLIC_MODE;
                }
                break;
            
            case state::CONFIGURE_CYCLIC_MODE:
                if(device->get_cyclic_config_complete() && device->get_sequential_cmds_complete()){
                    device->set_cyclic_data_enabled(true);
                    current_state = state::ENABLE_CYCLIC_MODE;
                }
                break;

            case state::ENABLE_CYCLIC_MODE:
                if(device->get_cyclic_data_enabled()){
                    run_one_shot_settings();
                    current_state = state::RUN;
                    current_message_id = 0; // update all messages on the first run
                }
                else if(device->get_sequential_cmds_complete()){
                    device->set_cyclic_data_enabled(true);
                }
                break;

            case state::RUN:
                run_async_updates();
                run_cyclic_updates();
                break;

            case state::RESET:
                device->clear_pending_sequential_cmds();
                device->force_disable_cyclic_data();
                current_state = state::DISABLE_CYCLIC_MODE;
                break;

            default:
                break;
            }

            cycles++;

            return 0;
        }

        void run_one_shot_settings(){
            // run the one-shot settings
            for(auto& reg : set_regs){
                device->sequential_write(reg.address, &reg.data, reg.size);
            }
        }

        void run_api_calls(){

            // run the writes
            bool writes_done = true;
            for(auto& write : api_write_regs){
                if(write.sent){
                    continue;
                }
                if(device->sequential_write(write.address, write.data, write.size) == 0){
                    write.sent = true;
                    writes_done = false;
                }
            }

            if(writes_done && device->get_sequential_cmds_complete()){
                // clear the done writes
                api_write_regs.clear();
            }

            // run the reads
            for(auto& read : api_read_regs){
                device->sequential_read(dev_desc.registers[read].index, &(dev_desc.registers[read].data), dev_desc.registers[read].size);
            }
            api_read_regs.clear();
        }

        void run_async_updates(){

            run_firmware_update();

            // skip if too many cmds are outstanding
            if(device->get_pending_sequential_cmds() > 5){
                return;
            }

            run_api_calls();

            // run async command if their update count has been reached

            for(auto it = async_read_regs.begin(); it != async_read_regs.end(); ++it){
                async_reg* reg = &(*it);
                if(reg->next_update_cycles <= cycles){
                    // run the async read
                    device->sequential_read(reg->address, reg->out->data_pointer, reg->size);
                    reg->next_update_cycles = reg->update_rate_cycles + cycles;
                    return;
                }
            }

            for(auto it = async_write_regs.begin(); it != async_write_regs.end(); ++it){
                async_reg* reg = &(*it);
                if(reg->next_update_cycles <= cycles){
                    // run the async write
                    device->sequential_write(reg->address, reg->in->data_pointer, reg->size);
                    reg->next_update_cycles = reg->update_rate_cycles + cycles;
                    return;
                }
            }


            // if no async updates ran, update messages if needed
            if(current_message_id != -1){
                if(device->get_pending_sequential_cmds() == 0){

                    if(current_message_id < dev_desc.message_count){
                        // select the next message
                        device->sequential_write(dev_desc.message_control_address, &current_message_id, sizeof(uint16_t));

                        message* msg = &(dev_desc.messages[current_message_id]);

                        uint32_t* lower = &(msg->last_trigger_time_us.u32[0]);
                        uint32_t* upper = &(msg->last_trigger_time_us.u32[1]);

                        // read the message time
                        device->sequential_read(dev_desc.message_time_lower_address, lower, sizeof(uint32_t));
                        device->sequential_read(dev_desc.message_time_upper_address, upper, sizeof(uint32_t));

                        current_message_id++;
                    }
                    else{
                        current_message_id = -1;
                        dump_messages();
                    }
                    
                }

            }

            return;
        }

        void run_firmware_update(){
            if(firmware_buffer == nullptr){
                return;
            }

            if(device->get_pending_sequential_cmds() == 0){
                if(firmware_write_wait){
                    // waiting for last send data to finish
                    // it is now complete and we can signal to write it to flash
                    device->sequential_write(dev_desc.firmware_write_address, (void*)&firmware_offset, sizeof(uint32_t));
                    firmware_offset += 8*4; // 8 32-bit words
                    firmware_write_wait = false;
                }
                else if(firmware_offset < firmware_size){
                    // not waitng on a write, so we can send the next chunk of data
                    uint32_t* data = reinterpret_cast<uint32_t*>(&firmware_buffer[firmware_offset]);
                    device->sequential_write(dev_desc.firmware_data_0, data++, sizeof(uint32_t));
                    device->sequential_write(dev_desc.firmware_data_1, data++, sizeof(uint32_t));
                    device->sequential_write(dev_desc.firmware_data_2, data++, sizeof(uint32_t));
                    device->sequential_write(dev_desc.firmware_data_3, data++, sizeof(uint32_t));
                    device->sequential_write(dev_desc.firmware_data_4, data++, sizeof(uint32_t));
                    device->sequential_write(dev_desc.firmware_data_5, data++, sizeof(uint32_t));
                    device->sequential_write(dev_desc.firmware_data_6, data++, sizeof(uint32_t));
                    device->sequential_write(dev_desc.firmware_data_7, data++, sizeof(uint32_t));

                    firmware_write_wait = true;

                    if((firmware_offset*100)/firmware_size >= firmware_total_percent){
                        std::cout << "Firmware update progress: " << static_cast<int>(firmware_total_percent) << "%" << std::endl;
                        firmware_total_percent++;
                    }
                }
                else{
                    // firmware update is complete, trigger the restart
                    std::cout << "Firmware update restarting device..." << std::endl;
                    device->sequential_write(dev_desc.firmware_write_address, (void*)&firmware_update_restart_val, sizeof(uint32_t));
                    firmware_offset = 0;
                    firmware_total_percent = 0;
                    firmware_write_wait = false;
                    delete[] firmware_buffer;
                    firmware_buffer = nullptr;
                }
            }
        }

        void run_cyclic_updates(){
            // copy data from the input ports to the global vars and from the global vars to the output ports
            for(auto& cpy : cyclic_node_copies){
                uint16_t test = *(reinterpret_cast<uint16_t*>(cpy.src));
                memcpy(cpy.dst, cpy.src, cpy.size);
            }
        }

        void dump_messages(){
            // print all messages to terminal

            std::cout << "\nEM Serial device " << device->get_full_name() << " messages:" << std::endl;
            for(auto& msg : dev_desc.messages){
                std::cout << "Trigger time (us): " << msg.second.last_trigger_time_us.u64 << "\t" << msg.second.str_severity << "\t" << msg.second.path << " - " << msg.second.message << std::endl;
            }
        }

        uint32_t configure_settings(json* json_settings) override{
            // parse  settings from the json file

            /*
            {
            "config":
                {
                    "comm":{
                        "device_address": 0,
                        "timeout_tries": 100
                    },
                    "device":{
                        "device_descriptor": "file/path/to/descriptor.json",    // this must be loaded before any other settings

                        // configuring an async reg will create an input/output on the node, its value will be updated at the specified rate
                        "async_read_regs":[
                            {
                                "name": "register_name",
                                "update_rate_cycles": 100
                            },
                            {
                                "name": "register_name2",
                                "update_rate_cycles": 10
                            }
                        ],
                        "async_write_regs":[
                            {
                                "name": "register_name",
                                "update_rate_cycles": 100
                            },
                            {
                                "name": "register_name2",
                                "update_rate_cycles": 10
                            }
                        ],

                        // configuring a cyclic reg will create an input/output on the node if configured to do so, its value will be updated every cycle
                        "cyclic_read_regs":[
                            {
                                "name": "register_name",
                                "sync_with_node": true,
                                "cyclic_index": 3
                            },
                            {
                                "name": "register_name2",
                                "sync_with_node": true,
                                "cyclic_index": 4
                            }
                        ],
                        "cyclic_write_regs":[
                            {
                                "name": "register_name",
                                "sync_with_node": true,
                                "cyclic_index": 3
                            },
                            {
                                "name": "register_name2",
                                "sync_with_node": true,
                                "cyclic_index": 4
                            }
                        ],
                        "set": [    // one-shot write a value to a register, these will be run on each device reconnect
                            {
                                "name": "register_name",
                                "value": 0.5
                            }
                        ]
                    },
                    "write":[   // async writes, these occur once when sent
                        "register_name": 0.5,
                        "register_name2": 0.5,
                        "register_name3": 0.5
                    ],
                    "read":[    // async reads, these occur once when sent, but will lag 1 read cycle due to how updates are done
                        "register_name",
                        "register_name2",
                        "register_name3"
                    ],
                    "print_errors": true,   // print errors to the console
                    "firmware_update": "file/path/to/firmware.bin"   // firmware update binary, this will immediately start the update process
                }
            
            }
            
            */

            // print messages to the console
            if(json_settings->contains("print_errors")){
                current_message_id = 0;
            }

            // load firmware update
            if(json_settings->contains("firmware_update")){
                std::string firmware_file = (*json_settings)["firmware_update"].get<std::string>();
                load_firmware_bin_file(firmware_file);
            }

            // load api write registers
            for(auto& reg : (*json_settings)["write"].items()){
                set_reg r;
                std::string name = reg.key();
                if(dev_desc.registers.find(name) == dev_desc.registers.end()){
                    std::cerr << "Error: write register name: " << name << " not found in device descriptor" << std::endl;
                    (*json_settings)["error"].emplace("write", "register name: " + name + " not found in device descriptor");
                    return 1;
                }
                if(dev_desc.registers[name].writable == false){
                    std::cerr << "Error: write register: " << name << " is not writable" << std::endl;
                    (*json_settings)["error"].emplace("write", "register: " + name + " is not writable");
                    return 1;
                }
                
                r.size = dev_desc.registers[name].size;   // get the size from the device descriptor
                r.address = dev_desc.registers[name].index;   // get the address from the device descriptor


                // TODO: really need to make a good io class to handle all of these conversions and data types instead of this mess
                switch (dev_desc.registers[name].type){
                    case io_type::BOOL:
                        r.data[0] = reg.value().get<bool>() ? 1 : 0;
                        break;
                    case io_type::UINT8:
                        r.data[0] = reg.value().get<std::uint8_t>();
                        break;
                    case io_type::UINT16:
                        {
                        uint16_t t = reg.value().get<std::uint16_t>();
                        memcpy(r.data, &t, sizeof(uint16_t));
                        }
                        break;
                    case io_type::UINT32:
                        {
                        uint32_t t = reg.value().get<std::uint32_t>();
                        memcpy(r.data, &t, sizeof(uint32_t));
                        }
                        break;
                    case io_type::INT8:
                        r.data[0] = reg.value().get<std::int8_t>();
                        break;
                    case io_type::INT16:
                        {
                        int16_t t = reg.value().get<std::int16_t>();
                        memcpy(r.data, &t, sizeof(int16_t));
                        }
                        break;
                    case io_type::INT32:
                        {
                        int32_t t = reg.value().get<std::int32_t>();
                        memcpy(r.data, &t, sizeof(int32_t));
                        }
                        break;
                    case io_type::FLOAT:
                        {
                        float t = reg.value().get<float>();
                        memcpy(r.data, &t, sizeof(float));
                        }
                        break;
                    default:
                        std::cerr << "Error: write register: " << name << " has an invalid type" << std::endl;
                        (*json_settings)["error"].emplace("write", "register: " + name + " has an invalid type");
                        return 1;
                }

                api_write_regs.push_back(r);
            }

            // load api read registers into a temporary JSON object
            json read_values = json::object();
            for(auto& reg : (*json_settings)["read"]){
                std::string name = reg.get<std::string>();
                if(dev_desc.registers.find(name) == dev_desc.registers.end()){
                    std::cerr << "Error: read register name: " << name << " not found in device descriptor" << std::endl;
                    (*json_settings)["error"].emplace("read", "register name: " + name + " not found in device descriptor");
                    return 1;
                }
                if(dev_desc.registers[name].readable == false){
                    std::cerr << "Error: read register: " << name << " is not readable" << std::endl;
                    (*json_settings)["error"].emplace("read", "register: " + name + " is not readable");
                    return 1;
                }
    
                // add current known value of the reg to the temporary json object
                switch (dev_desc.registers[name].type){
                    case io_type::BOOL:
                        read_values[name] = dev_desc.registers[name].data[0] == 1;
                        break;
                    case io_type::UINT8:
                        read_values[name] = dev_desc.registers[name].data[0];
                        break;
                    case io_type::UINT16:
                        {
                        uint16_t t = 0;
                        memcpy(&t, dev_desc.registers[name].data, sizeof(uint16_t));
                        read_values[name] = t;
                        }
                        break;
                    case io_type::UINT32:
                        {
                        uint32_t t = 0;
                        memcpy(&t, dev_desc.registers[name].data, sizeof(uint32_t));
                        read_values[name] = t;
                        }
                        break;
                    case io_type::INT8:
                        read_values[name] = dev_desc.registers[name].data[0];
                        break;
                    case io_type::INT16:
                        {
                        int16_t t = 0;
                        memcpy(&t, dev_desc.registers[name].data, sizeof(int16_t));
                        read_values[name] = t;
                        }
                        break;
                    case io_type::INT32:
                        {
                        int32_t t = 0;
                        memcpy(&t, dev_desc.registers[name].data, sizeof(int32_t));
                        read_values[name] = t;
                        }
                        break;
                    case io_type::FLOAT:
                        {
                        float t = 0;
                        memcpy(&t, dev_desc.registers[name].data, sizeof(float));
                        read_values[name] = t;
                        }
                        break;
                    default:
                        std::cerr << "Error: read register: " << name << " has an invalid type" << std::endl;
                        (*json_settings)["error"].emplace("read", "register: " + name + " has an invalid type");
                        return 1;
                }
    
                api_read_regs.push_back(name);
            }
            // replace the original "read" array with the constructed object containing register values
            (*json_settings)["read"] = read_values;

            if(driver_setup_done){  // skip if the driver has already been setup, TODO: allow reconfiguration later through api config
                return 0;
            }

            if(json_settings->contains("comm")){
                if(json_settings->at("comm").contains("device_address")){
                    comm.device_address = json_settings->at("comm")["device_address"].get<uint32_t>();
                }
                if(json_settings->at("comm").contains("timeout_tries")){
                    comm.timeout_tries = json_settings->at("comm")["timeout_tries"].get<uint32_t>();
                }
            }

            if(!dev_desc.loaded){   // load the device descriptor if it has not been loaded yet
                if(load_device_descriptor_file((*json_settings)["device"]["device_descriptor"].get<std::string>()) != 0){
                    std::cerr << "Error: failed to load device descriptor" << std::endl;
                    return 1;
                }
                dev_desc.loaded = true;
            }


            // the device driver object is not connected yet, so just save the startup config for later use
            
            // load async read registers
            for(auto& reg : (*json_settings)["device"]["async_read_regs"]){
                async_reg r;
                r.name = reg["name"].get<std::string>();
                if(dev_desc.registers.find(r.name) == dev_desc.registers.end()){
                    std::cerr << "Error: async read register name not found in device descriptor" << std::endl;
                    return 1;
                }
                r.update_rate_cycles = reg["update_rate_cycles"].get<uint32_t>();
                r.address = dev_desc.registers[r.name].index;   // get the address from the device descriptor
                r.size = dev_desc.registers[r.name].size;   // get the size from the device descriptor
                async_read_regs.push_back(r);
            }

            // load async write registers
            for(auto& reg : (*json_settings)["device"]["async_write_regs"]){
                async_reg r;
                r.name = reg["name"].get<std::string>();
                if(dev_desc.registers.find(r.name) == dev_desc.registers.end()){
                    std::cerr << "Error: async write register name not found in device descriptor" << std::endl;
                    return 1;
                }
                r.update_rate_cycles = reg["update_rate_cycles"].get<uint32_t>();
                r.address = dev_desc.registers[r.name].index;   // get the address from the device descriptor
                r.size = dev_desc.registers[r.name].size;   // get the size from the device descriptor
                async_write_regs.push_back(r);
            }

            // load cyclic read registers
            for(auto& reg : (*json_settings)["device"]["cyclic_read_regs"]){
                cyclic_reg r;
                r.name = reg["name"].get<std::string>();
                if(dev_desc.registers.find(r.name) == dev_desc.registers.end()){
                    std::cerr << "Error: cyclic read register name not found in device descriptor" << std::endl;
                    return 1;
                }
                r.sync_with_node = reg["sync_with_node"].get<bool>();
                r.address = dev_desc.registers[r.name].index;   // get the address from the device descriptor
                r.size = dev_desc.registers[r.name].size;   // get the size from the device descriptor
                r.cyclic_index = reg["cyclic_index"].get<uint8_t>();
                highest_cyclic_read_index = std::max(highest_cyclic_read_index, r.cyclic_index);
                cyclic_read_regs.emplace(r.cyclic_index, r);
            }

            // load cyclic write registers
            for(auto& reg : (*json_settings)["device"]["cyclic_write_regs"]){
                cyclic_reg r;
                r.name = reg["name"].get<std::string>();
                if(dev_desc.registers.find(r.name) == dev_desc.registers.end()){
                    std::cerr << "Error: cyclic write register name not found in device descriptor" << std::endl;
                    return 1;
                }
                r.sync_with_node = reg["sync_with_node"].get<bool>();
                r.address = dev_desc.registers[r.name].index;   // get the address from the device descriptor
                r.size = dev_desc.registers[r.name].size;   // get the size from the device descriptor
                r.cyclic_index = reg["cyclic_index"].get<uint8_t>();
                highest_cyclic_write_index = std::max(highest_cyclic_write_index, r.cyclic_index);
                cyclic_write_regs.emplace(r.cyclic_index, r);
            }

            // load set registers
            for(auto& reg : (*json_settings)["device"]["set"]){
                set_reg r;
                std::string name = reg["name"].get<std::string>();
                if(dev_desc.registers.find(name) == dev_desc.registers.end()){
                    std::cerr << "Error: set register name: " << name << " not found in device descriptor" << std::endl;
                    return 1;
                }
                if(dev_desc.registers[name].writable == false){
                    std::cerr << "Error: set register: " << name << " is not writable" << std::endl;
                    return 1;
                }

                r.size = dev_desc.registers[name].size;   // get the size from the device descriptor
                r.address = dev_desc.registers[name].index;   // get the address from the device descriptor


                // TODO: really need to make a good io class to handle all of these conversions and data types instead of this mess
                switch (dev_desc.registers[name].type){
                    case io_type::BOOL:
                        r.data[0] = reg["value"].get<bool>() ? 1 : 0;
                        break;
                    case io_type::UINT8:
                        r.data[0] = reg["value"].get<std::uint8_t>();
                        break;
                    case io_type::UINT16:
                        {
                        uint16_t t = reg["value"].get<std::uint16_t>();
                        memcpy(r.data, &t, sizeof(uint16_t));
                        }
                        break;
                    case io_type::UINT32:
                        {
                        uint32_t t = reg["value"].get<std::uint32_t>();
                        memcpy(r.data, &t, sizeof(uint32_t));
                        }
                        break;
                    case io_type::INT8:
                        r.data[0] = reg["value"].get<std::int8_t>();
                        break;
                    case io_type::INT16:
                        {
                        int16_t t = reg["value"].get<std::int16_t>();
                        memcpy(r.data, &t, sizeof(int16_t));
                        }
                        break;
                    case io_type::INT32:
                        {
                        int32_t t = reg["value"].get<std::int32_t>();
                        memcpy(r.data, &t, sizeof(int32_t));
                        }
                        break;
                    case io_type::FLOAT:
                        {
                        float t = reg["value"].get<float>();
                        memcpy(r.data, &t, sizeof(float));
                        }
                        break;
                    default:
                        std::cerr << "Error: set register: " << name << " has an invalid type" << std::endl;
                        return 1;
                }

                set_regs.push_back(r);
            }

            // load enabled
            if(json_settings->contains("enabled")){
                enabled = json_settings->at("enabled").get<bool>();
            }


            // configure async updates and add inputs/outputs

            // outputs from the device
            for(auto& reg : async_read_regs){
                uint8_t* data = new uint8_t[reg.size];  // create a new place to store the data
                memset(data, 0, reg.size);  // initialize the data to zero (TODO: might want special values for some types)

                outputs.emplace(reg.name, output(dev_desc.registers[reg.name].type, data, &execution_number));    // create an output for the async read
                reg.out = &outputs[reg.name]; // save the output pointer for later use
            }

            // inputs to the device
            for(auto& reg : async_write_regs){
                inputs.emplace(reg.name, input(dev_desc.registers[reg.name].type, nullptr));    // create an input for the async write
                reg.in = &inputs[reg.name]; // save the input pointer for later use
            }


            // configure cyclic updates and add inputs/outputs

            // outputs from the device
            for(auto& r : cyclic_read_regs){
                cyclic_reg& reg = r.second;

                if(reg.sync_with_node){ // we need to move the data to the node system and create an output
                    uint8_t* data = new uint8_t[reg.size];  // create a new place to store the data
                    memset(data, 0, reg.size);  // initialize the data to zero (TODO: might want special values for some types)
                    reg.data_ptr = data;

                    outputs.emplace(reg.name, output(dev_desc.registers[reg.name].type, data, &execution_number));
                    reg.out = &outputs[reg.name]; // save the output pointer for later use
                }
            }

            // inputs to the device
            for(auto& r : cyclic_write_regs){
                cyclic_reg& reg = r.second;

                if(reg.sync_with_node){ // we need to move the data from the node system and create an input
                    inputs.emplace(reg.name, input(dev_desc.registers[reg.name].type, nullptr));    // create an input for the async write
                    reg.in = &inputs[reg.name]; // save the input pointer for later use
                }
            }

            return 0;
        }

    private:

        em_serial_controller::em_serial_device *device = nullptr;

        uint64_t cycles = 0;

        bool enabled = false;

        enum class state{
            INITIAL_CONFIG,
            DISABLE_CYCLIC_MODE,
            CONFIGURE_CYCLIC_MODE,
            ENABLE_CYCLIC_MODE,
            RUN,
            RESET
        }current_state = state::INITIAL_CONFIG;

        uint32_t current_message_id = -1;    // used to track the current message when reading all messages from the device

        //uint32_t comm_fault_count = 0;  // faults from packet failure
        //uint32_t device_protocol_fault_count = 0;   // faults from the device protocol (tried to do something that is not allowed)

        uint8_t used_cyclic_read_node_vars = 0;
        uint8_t used_cyclic_write_node_vars = 0;

        struct comm_info{
            uint32_t device_address = 0;
            uint32_t timeout_tries = 100;
        };
        comm_info comm;

        struct hw_info{
            uint32_t hardware_type = 0;
            uint32_t hardware_version = 0;
            std::string name = "";
            std::string description = "";
        };

        struct fw_info{
            uint32_t firmware_version = 0;
            std::string release_date = "";
            std::string description = "";
        };

        struct reg{
            std::string name = "";
            uint32_t index = 0;
            io_type type = io_type::UNDEFINED;   //type of data supported on the node system
            std::string unit = "";   // unit of the data
            std::string description = "";
            uint8_t size = 0; // size of the data in bytes
            bool readable = false;
            bool writable = false;
            uint8_t data[4] = {0, 0, 0, 0};
        };

        struct message_severities{  // these values may be changed based on the device descriptor
            uint8_t none = 0;
            uint8_t info = 1;
            uint8_t warning = 2;
            uint8_t error = 3;
            uint8_t critical = 4;
        }severities;

        struct message{
            uint8_t severity = 0;
            std::string str_severity = "";
            uint16_t id = 0;
            std::string path = "";
            std::string message = "";
            std::string description = "";
            union last_trigger_time_us{
                uint32_t u32[2];
                uint64_t u64;
            }last_trigger_time_us;
        };

        struct device_descriptor{
            hw_info hardware;
            fw_info firmware;
            std::map<std::string, reg> registers;   // name, register
            std::map<uint32_t, message> messages;  // identifier, message
            bool loaded = false;
            uint16_t message_count = 0;
            uint16_t message_control_address = 0;
            uint16_t message_time_lower_address = 0;
            uint16_t message_time_upper_address = 0;
            uint16_t dummy_reg_address = 0;
            uint16_t firmware_write_address = 0;
            uint16_t firmware_data_0 = 0;
            uint16_t firmware_data_1 = 0;
            uint16_t firmware_data_2 = 0;
            uint16_t firmware_data_3 = 0;
            uint16_t firmware_data_4 = 0;
            uint16_t firmware_data_5 = 0;
            uint16_t firmware_data_6 = 0;
            uint16_t firmware_data_7 = 0;
        } dev_desc;

        struct async_reg{
            std::string name = "";
            uint32_t update_rate_cycles = 0;
            uint64_t next_update_cycles = 0;
            uint16_t address = 0;   // register address to read/write
            uint8_t size = 0; // size of the data in bytes
            input* in = nullptr;   // pointer to the input
            output* out = nullptr; // pointer to the output
        };

        struct cyclic_reg{
            std::string name = "";
            uint8_t cyclic_index = 0; // index of the cyclic memory in the FPGA module
            bool sync_with_node = false;
            uint8_t size = 0; // size of the data in bytes
            uint16_t address = 0;   // register address to read/write
            input* in = nullptr;   // pointer to the input
            output* out = nullptr; // pointer to the output
            void* data_ptr = nullptr;   // pointer to the data
        };

        struct set_reg{
            uint8_t size = 0; // size of the data in bytes
            uint16_t address = 0;   // register address
            uint8_t data[4] = {0, 0, 0, 0}; // data to write
            bool sent = false;   // if the data has been sent
        };

        struct cyclic_node_copy{
            void* src = nullptr;
            void* dst = nullptr;
            uint8_t size = 0;
        };
        std::vector<cyclic_node_copy> cyclic_node_copies;

        std::vector<async_reg> async_read_regs;
        std::vector<async_reg> async_write_regs;
        std::map<uint8_t, cyclic_reg> cyclic_read_regs;
        std::map<uint8_t, cyclic_reg> cyclic_write_regs;

        std::vector<set_reg> api_write_regs;
        std::vector<std::string> api_read_regs;

        uint8_t highest_cyclic_read_index = 0;
        uint8_t highest_cyclic_write_index = 0;

        std::vector<set_reg> set_regs;

        uint8_t* firmware_buffer = nullptr;
        uint32_t firmware_size = 0;
        uint32_t firmware_offset = 0;
        bool firmware_write_wait = false;
        uint32_t firmware_update_restart_val = 0xFFFFFFFF; // value to write to the firmware update register to trigger a restart
        uint8_t firmware_1_percent = 0;
        uint8_t firmware_total_percent = 0;

        uint32_t load_firmware_bin_file(std::string file_path){

            if(firmware_buffer != nullptr){
                std::cerr << "Error: firmware buffer is not null" << std::endl;
                return 2; 
            }

            // load the firmware binary file and parse it
            std::ifstream i(file_path, std::ios::binary);
            if(!i.is_open()){
                std::cerr << "Error: Unable to open firmware binary file" << std::endl;
                return 1;
            }

            // get the size of the file
            i.seekg(0, std::ios::end);
            firmware_size = i.tellg();
            i.seekg(0, std::ios::beg);

            // ensure the size is a multiple of 8
            if(firmware_size % 8 != 0){
                firmware_size += 8 - (firmware_size % 8);
            }

            // read the file into a buffer
            if(firmware_buffer != nullptr){
                delete[] firmware_buffer;
            }
            firmware_buffer = new uint8_t[firmware_size];
            i.read(reinterpret_cast<char*>(firmware_buffer), firmware_size);
            i.close();

            firmware_offset = 0;
            firmware_1_percent = firmware_size / 100;

            return 0;
        }

        uint32_t load_device_descriptor_file(std::string file_path){

            // empty the device descriptor
            dev_desc = device_descriptor();
            
            // load the device descriptor file and parse it
            std::ifstream i(file_path);
            if(!i.is_open()){
                std::cerr << "Error: Unable to open node config file" << std::endl;
                return 1;
            }

            json j;
            try{
                i >> j;
            }
            catch(json::parse_error& e){
                std::cerr << "Error: unable to parse node config file" << std::endl;
                return 2;
            }
            i.close();

            // load hw info
            dev_desc.hardware.hardware_type = j["hw_info"]["type"].get<uint32_t>();
            dev_desc.hardware.hardware_version = j["hw_info"]["version"].get<uint32_t>();
            dev_desc.hardware.name = j["hw_info"]["readable_name"].get<std::string>();
            dev_desc.hardware.description = j["hw_info"]["description"].get<std::string>();

            // load fw info
            dev_desc.firmware.firmware_version = j["fw_info"]["version"].get<uint32_t>();
            dev_desc.firmware.release_date = j["fw_info"]["release_date"].get<std::string>();
            dev_desc.firmware.description = j["fw_info"]["description"].get<std::string>();

            // load message severities
            for(auto& msg : j["message_severities"].items()){
                if(msg.key() == "none"){
                    severities.none = msg.value().get<uint32_t>();
                }
                else if(msg.key() == "info"){
                    severities.info = msg.value().get<uint32_t>();
                }
                else if(msg.key() == "warning"){
                    severities.warning = msg.value().get<uint32_t>();
                }
                else if(msg.key() == "error"){
                    severities.error = msg.value().get<uint32_t>();
                }
                else if(msg.key() == "critical"){
                    severities.critical = msg.value().get<uint32_t>();
                }
                else{
                    std::cerr << "Error: unknown message severity" << std::endl;
                    return 3;
                }
            }


            // load registers
            for(auto& reg_j : j["registers"].items()){
                reg r;
                r.name = reg_j.key();
                r.index = reg_j.value()["index"].get<uint32_t>();

                r.type = get_io_type(reg_j.value()["type"].get<std::string>());

                r.unit = reg_j.value()["unit"].get<std::string>();
                r.description = reg_j.value()["description"].get<std::string>();
                std::string perms = reg_j.value()["permissions"].get<std::string>();
                if(perms.find("read") != std::string::npos){
                    r.readable = true;
                }
                if(perms.find("write") != std::string::npos){
                    r.writable = true;
                }

                r.size = io_type_size.at(r.type);

                dev_desc.registers.emplace(r.name, r);
            }


            // load messages
            dev_desc.message_count = 0;
            for(auto& msg_class_j : j["messages"].items()){

                std::string msg_class = msg_class_j.key();

                for(auto& msg_j : msg_class_j.value().items()){
                    message msg;
                    std::string severity = msg_j.value()["severity"].get<std::string>();
                    if(severity == "none"){
                        msg.severity = severities.none;
                    }
                    else if(severity == "info"){
                        msg.severity = severities.info;
                    }
                    else if(severity == "warning"){
                        msg.severity = severities.warning;
                    }
                    else if(severity == "error"){
                        msg.severity = severities.error;
                    }
                    else if(severity == "critical"){
                        msg.severity = severities.critical;
                    }
                    else{
                        std::cerr << "Error: unknown message severity found in device descriptor: " << file_path << std::endl;
                        return 4;
                    }
                    msg.str_severity = severity;
                    msg.id = msg_j.value()["id"].get<uint16_t>();
                    msg.path = msg_class + "." + msg_j.key();
                    msg.message = msg_j.value()["readable_name"].get<std::string>();
                    msg.description = msg_j.value()["description"].get<std::string>();
                    msg.last_trigger_time_us.u64 = 0;

                    dev_desc.message_count++;

                    dev_desc.messages.emplace(msg.id, msg);

                }
            }

            // set message control addresses
            if(dev_desc.registers.find("message_control") == dev_desc.registers.end() ||
                dev_desc.registers.find("message_time_lower") == dev_desc.registers.end() ||
                dev_desc.registers.find("message_time_upper") == dev_desc.registers.end() ||
                dev_desc.registers.find("dummy_register") == dev_desc.registers.end()){
                throw std::runtime_error("Error: missing required message control registers in device descriptor");
            }

            dev_desc.message_control_address = dev_desc.registers["message_control"].index;
            dev_desc.message_time_lower_address = dev_desc.registers["message_time_lower"].index;
            dev_desc.message_time_upper_address = dev_desc.registers["message_time_upper"].index;
            dev_desc.dummy_reg_address = dev_desc.registers["dummy_register"].index;

            // set firmware write addresses
            if(dev_desc.registers.find("firmware_update_data_address") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_0") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_1") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_2") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_3") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_4") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_5") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_6") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_update_data_7") == dev_desc.registers.end()){
                throw std::runtime_error("Error: missing required firmware write registers in device descriptor");
            }
            dev_desc.firmware_write_address = dev_desc.registers["firmware_update_data_address"].index;
            dev_desc.firmware_data_0 = dev_desc.registers["firmware_update_data_0"].index;
            dev_desc.firmware_data_1 = dev_desc.registers["firmware_update_data_1"].index;
            dev_desc.firmware_data_2 = dev_desc.registers["firmware_update_data_2"].index;
            dev_desc.firmware_data_3 = dev_desc.registers["firmware_update_data_3"].index;
            dev_desc.firmware_data_4 = dev_desc.registers["firmware_update_data_4"].index;
            dev_desc.firmware_data_5 = dev_desc.registers["firmware_update_data_5"].index;
            dev_desc.firmware_data_6 = dev_desc.registers["firmware_update_data_6"].index;
            dev_desc.firmware_data_7 = dev_desc.registers["firmware_update_data_7"].index;

            dev_desc.loaded = true;

            return 0;
        }

        bool driver_setup_done = false;
        uint32_t initial_driver_setup(){
            // runs once at startup to setup the driver once it has been connected in the node system
            if(driver_setup_done){
                std::cerr << "Error: initial driver setup already done" << std::endl;
                return 0;
            }
            if(!dev_desc.loaded){
                std::cerr << "Error: device descriptor not loaded" << std::endl;
                return 1;
            }
            if(inputs["em_device"].data_pointer == nullptr){
                std::cerr << "Error: em_device input not connected" << std::endl;
                return 2;
            }
            device = (em_serial_controller::em_serial_device*)inputs["em_device"].data_pointer;  // TODO: add a way to reconnect the device if the node system layout is changed
            
            // TODO: add a way to modify these later

            // make sure they all exist
            if(dev_desc.registers.find("hardware_type") == dev_desc.registers.end() ||
                dev_desc.registers.find("hardware_version") == dev_desc.registers.end() ||
                dev_desc.registers.find("firmware_version") == dev_desc.registers.end() ||
                dev_desc.registers.find("enable_cyclic_data") == dev_desc.registers.end() ||
                dev_desc.registers.find("cyclic_write_address_0") == dev_desc.registers.end() ||
                dev_desc.registers.find("cyclic_read_address_0") == dev_desc.registers.end()){
                std::cerr << "Error: missing required communication setup registers in device descriptor" << std::endl;
                return 3;
            }

            device->dev_info.hardware_type_addr = dev_desc.registers["hardware_type"].index;
            device->dev_info.hardware_version_addr = dev_desc.registers["hardware_version"].index;
            device->dev_info.firmware_version_addr = dev_desc.registers["firmware_version"].index;
            device->dev_info.enable_cyclic_data_addr = dev_desc.registers["enable_cyclic_data"].index;
            device->dev_info.cyclic_write_address_0_addr = dev_desc.registers["cyclic_write_address_0"].index;
            device->dev_info.cyclic_read_address_0_addr = dev_desc.registers["cyclic_read_address_0"].index;
            device->dev_info.cyclic_addresses = device->dev_info.cyclic_write_address_0_addr - device->dev_info.cyclic_read_address_0_addr + 1; // TODO: get this directly from the device descriptor
            
            // TODO: should read and verify these match the device
            device->dev_info.hardware_type = dev_desc.hardware.hardware_type;
            device->dev_info.hardware_version = dev_desc.hardware.hardware_version;
            device->dev_info.firmware_version = dev_desc.firmware.firmware_version;

            if(!enabled){
                return 0;   // no need to continue if the device is disabled
            }

            device->set_address(comm.device_address);

            bool skipped_index = false;

            // configure cyclic reads
            for(uint8_t i = 0; i <= highest_cyclic_read_index; i++){

                // reserved for device communication
                if(i < 3){
                    if(cyclic_read_regs.find(i) != cyclic_read_regs.end()){
                        throw std::runtime_error("Error: cyclic read register index " + std::to_string(i) + " is reserved for device communication");
                    }
                    continue;
                }

                // reserved for node vars
                if(i < device->cyclic_read_node_var_regs.size() + 3){
                    if(cyclic_read_regs.find(i) != cyclic_read_regs.end()){
                        auto reg = cyclic_read_regs.find(i)->second;
                        if(reg.sync_with_node){
                            cyclic_node_copy c;
                            c.src = device->cyclic_read_node_var_regs[used_cyclic_read_node_vars]->get_raw_data_ptr<uint32_t>();
                            c.dst = reg.data_ptr;
                            c.size = reg.size;
                            cyclic_node_copies.push_back(c);
                            device->configure_cyclic_read(reg.address, reg.size);
                        }
                        else{
                            throw std::runtime_error("Error: cyclic read register index " + std::to_string(i) + " is reserved for synced node vars");
                        }
                    }
                    else{
                        // no user config, set it to the dummy register
                        device->configure_cyclic_read(dev_desc.dummy_reg_address, sizeof(uint8_t));
                    }
                    continue;
                }

                // any remaining indexes must be used for FPGA direct access
                if(cyclic_read_regs.find(i) != cyclic_read_regs.end()){
                    auto reg = cyclic_read_regs.find(i)->second;
                    if(reg.sync_with_node){
                        throw std::runtime_error("Error: cyclic read register index " + std::to_string(i) + " is reserved for FPGA direct access");
                    }
                    if(skipped_index){
                        throw std::runtime_error("Error: cyclic read register index " + std::to_string(i) + " skipped an index, configs must be contiguous");
                    }
                    device->configure_cyclic_read(reg.address, reg.size);
                }
                else{
                    skipped_index = true;
                }
            }

            // configure cyclic writes
            for(uint8_t i = 0; i <= highest_cyclic_write_index; i++){

                // reserved for device communication
                if(i < 3){
                    if(cyclic_write_regs.find(i) != cyclic_write_regs.end()){
                        throw std::runtime_error("Error: cyclic write register index " + std::to_string(i) + " is reserved for device communication");
                    }
                    continue;
                }

                // reserved for node vars
                if(i < device->cyclic_write_node_var_regs.size() + 3){
                    if(cyclic_write_regs.find(i) != cyclic_write_regs.end()){
                        auto reg = cyclic_write_regs.find(i)->second;
                        if(reg.sync_with_node){
                            cyclic_node_copy c;
                            c.dst = device->cyclic_write_node_var_regs[used_cyclic_write_node_vars]->get_raw_data_ptr<uint32_t>();
                            c.src = reg.in->data_pointer;
                            c.size = reg.size;
                            cyclic_node_copies.push_back(c);
                            device->configure_cyclic_write(reg.address, reg.size);
                        }
                        else{
                            throw std::runtime_error("Error: cyclic write register index " + std::to_string(i) + " is reserved for synced node vars");
                        }
                    }
                    else{
                        // no user config, set it to the dummy register
                        device->configure_cyclic_write(dev_desc.dummy_reg_address, sizeof(uint8_t));
                    }
                    continue;
                }

                // any remaining indexes must be used for FPGA direct access
                if(cyclic_write_regs.find(i) != cyclic_write_regs.end()){
                    auto reg = cyclic_write_regs.find(i)->second;
                    if(reg.sync_with_node){
                        throw std::runtime_error("Error: cyclic write register index " + std::to_string(i) + " is reserved for FPGA direct access");
                    }
                    if(skipped_index){
                        throw std::runtime_error("Error: cyclic write register index " + std::to_string(i) + " skipped an index, configs must be contiguous");
                    }
                    device->configure_cyclic_write(reg.address, reg.size);
                }
                else{
                    skipped_index = true;
                }
            }

            driver_setup_done = true;
            return 0;
        }
};