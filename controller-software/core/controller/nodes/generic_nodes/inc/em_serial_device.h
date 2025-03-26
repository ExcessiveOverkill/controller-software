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
            outputs.emplace("device_error", output(io_type::BOOL, &device_error, &execution_number));    // error from the device
            outputs.emplace("device_warning", output(io_type::BOOL, &device_warning, &execution_number));    // warning from the device
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

            if(device->consecutive_packet_errors > comm.timeout_tries){
                if(communication_error == false){
                    if(current_state == state::RUN){
                        std::cerr << "Error: EM serial device communication error" << std::endl;
                    }
                    if(current_state != state::DISABLE_CYCLIC_MODE && current_state != state::INITIAL_CONFIG){
                        current_state = state::RESET;
                    }
                }
                communication_error = true;
            }
            else{
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
                if(device->get_sequential_cmds_complete() && device->get_cyclic_data_enabled()){
                    device->set_cyclic_data_enabled(false);
                }
                if(device->get_sequential_cmds_complete() && !device->get_cyclic_data_enabled() && device->consecutive_packet_errors == 0){
                    device->rerun_cylic_config();
                    current_state = state::CONFIGURE_CYCLIC_MODE;
                }
                
                break;
            
            case state::CONFIGURE_CYCLIC_MODE:
                if(device->get_cyclic_config_complete()){
                    device->set_cyclic_data_enabled(true);
                    current_state = state::ENABLE_CYCLIC_MODE;
                }
                break;

            case state::ENABLE_CYCLIC_MODE:
                if(device->get_cyclic_data_enabled()){
                    current_state = state::RUN;
                }
                break;

            case state::RUN:
                run_async_updates();
                run_cyclic_updates();
                break;

            case state::RESET:
                device->clear_pending_sequential_cmds();
                current_state = state::DISABLE_CYCLIC_MODE;
                break;

            default:
                break;
            }

            cycles++;

            return 0;
        }

        void run_async_updates(){

            // skip if too many cmds are outstanding
            if(device->get_pending_sequential_cmds() > 5){
                return;
            }

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

            return;
        }

        void run_cyclic_updates(){
            // copy data from the input ports to the global vars and from the global vars to the output ports
            for(auto& cpy : cyclic_node_copies){
                uint16_t test = *(reinterpret_cast<uint16_t*>(cpy.src));
                memcpy(cpy.dst, cpy.src, cpy.size);
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
                                "sync_with_node": true
                            },
                            {
                                "name": "register_name2",
                                "sync_with_node": true
                            }
                        ],
                        "cyclic_write_regs":[
                            {
                                "name": "register_name",
                                "sync_with_node": true
                            },
                            {
                                "name": "register_name2",
                                "sync_with_node": true
                            }
                        ]

                    }
                },

            // one-shot write a value to a register, these will be run on each device reconnect
            "set": [
                {
                    "name": "register_name",
                    "value": 0.5
                }
            ]
            }
            
            */

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
                cyclic_read_regs.push_back(r);
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
                cyclic_write_regs.push_back(r);
            }

            // load set registers
            for(auto& reg : (*json_settings)["set"]){
                set_reg r;
                r.name = reg["name"].get<std::string>();
                if(dev_desc.registers.find(r.name) == dev_desc.registers.end()){
                    std::cerr << "Error: set register name not found in device descriptor" << std::endl;
                    return 1;
                }
                r.value = reg["value"].get<std::string>();
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
            for(auto& reg : cyclic_read_regs){

                if(reg.sync_with_node){ // we need to move the data to the node system and create an output
                    uint8_t* data = new uint8_t[reg.size];  // create a new place to store the data
                    memset(data, 0, reg.size);  // initialize the data to zero (TODO: might want special values for some types)
                    reg.data_ptr = data;

                    outputs.emplace(reg.name, output(dev_desc.registers[reg.name].type, data, &execution_number));
                    reg.out = &outputs[reg.name]; // save the output pointer for later use
                }
            }

            // inputs to the device
            for(auto& reg : cyclic_write_regs){

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
            uint32_t value = 0;
            std::string path = "";
            std::string message = "";
            std::string description = "";
        };

        struct device_descriptor{
            hw_info hardware;
            fw_info firmware;
            std::map<std::string, reg> registers;   // name, register
            std::map<uint32_t, message> messages;  // identifier, message
            bool loaded = false;
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
            bool sync_with_node = false;
            uint8_t size = 0; // size of the data in bytes
            uint16_t address = 0;   // register address to read/write
            input* in = nullptr;   // pointer to the input
            output* out = nullptr; // pointer to the output
            void* data_ptr = nullptr;   // pointer to the data
        };

        struct set_reg{
            std::string name = "";
            std::string value = ""; // string will be converted to the correct type
        };

        struct cyclic_node_copy{
            void* src = nullptr;
            void* dst = nullptr;
            uint8_t size = 0;
        };
        std::vector<cyclic_node_copy> cyclic_node_copies;

        std::vector<async_reg> async_read_regs;
        std::vector<async_reg> async_write_regs;
        std::vector<cyclic_reg> cyclic_read_regs;
        std::vector<cyclic_reg> cyclic_write_regs;

        std::vector<set_reg> set_regs;

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
                    msg.value = msg_j.value()["value"].get<uint32_t>();
                    msg.path = msg_class + msg_j.key();
                    msg.message = msg_j.value()["readable_name"].get<std::string>();
                    msg.description = msg_j.value()["description"].get<std::string>();

                    dev_desc.messages.emplace(msg.value, msg);

                }
            }

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

            
            // configure cyclic reads
            for(auto& reg : cyclic_read_regs){
                if(reg.sync_with_node){
                    if(used_cyclic_read_node_vars >= device->cyclic_read_node_var_regs.size()){
                        std::cerr << "Error: too many node cyclic read registers configured" << std::endl;
                        return 3;
                    }
                    // copy from global var to the note output
                    cyclic_node_copy c;
                    c.src = device->cyclic_read_node_var_regs[used_cyclic_read_node_vars]->get_raw_data_ptr<uint32_t>();
                    c.dst = reg.data_ptr;
                    c.size = reg.size;
                    cyclic_node_copies.push_back(c);
                    used_cyclic_read_node_vars++;
                }
                device->configure_cyclic_read(reg.address, reg.size);
            }

            // configure cyclic writes
            for(auto& reg : cyclic_write_regs){
                if(reg.sync_with_node){
                    if(used_cyclic_write_node_vars >= device->cyclic_write_node_var_regs.size()){
                        std::cerr << "Error: too many node cyclic write registers configured" << std::endl;
                        return 3;
                    }
                    // copy from the node input to the global var
                    cyclic_node_copy c;
                    c.src = reg.in->data_pointer;   // TODO: fix this, if the inpuup changes the pointer will be invalid
                    c.dst = device->cyclic_write_node_var_regs[used_cyclic_write_node_vars]->get_raw_data_ptr<uint32_t>();
                    c.size = reg.size;
                    cyclic_node_copies.push_back(c);
                    used_cyclic_write_node_vars++;
                }
                device->configure_cyclic_write(reg.address, reg.size);
            }

            driver_setup_done = true;
            return 0;
        }
};