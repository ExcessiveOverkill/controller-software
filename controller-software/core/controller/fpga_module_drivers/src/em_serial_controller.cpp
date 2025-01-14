#include "em_serial_controller.h"

static Driver_Registrar<em_serial_controller> registrar("em_serial_controller");

uint32_t em_serial_controller::load_config(json config, std::vector<uint64_t>* instructions){

    if(!load_json_value(config, "node_address", &node_address)){
        std::cerr << "Failed to load node address" << std::endl;
        return 1;
    }

    // configure registers
    Address_Map_Loader loader;
    loader.setup(&config, &base_mem, node_address, instructions);

    // TODO: automatically create all device objects based on the number of devices in the config
    devices.push_back(new em_serial_device(&loader, 0));

    bit_length = loader.get_register("bit_length", 0);
    loader.sync_with_PS(bit_length);

    status = loader.get_register("status", 0);
    loader.sync_with_PS(status);
    status_update_busy = status->get_register("update_busy");
    status_update_done = status->get_register("update_done");
    status_update_error = status->get_register("update_error");

    configure_baud_rate(12.5e6);


    ////////////////// TESTING ///////////////////

    // control current cmd
    //devices[0]->configure_cyclic_write(5, 4);    // commanded q current is milliamps

    void* cmd_ptr = nullptr;
    devices[0]->set_cyclic_write_pointer(5, &cmd_ptr);
    cmd_q_current_milliamps = reinterpret_cast<int32_t*>(cmd_ptr);

    // add instruction to copy commutation from encoder to drive
    uint64_t inst = create_instruction_COPY(2, 2, 4, 195);  // copy encoder commutation to drive cyclic #3

    loader.instructions->push_back(inst);

    devices[0]->set_address(0);

    for(int i = 0; i < 50; i++){    // add some NOPs to ensure all instructions are completed before the transfers start
        loader.instructions->push_back(create_instruction_NOP());
    }

    control = loader.get_register("control", 0);
    loader.sync_with_PS(control);
    start_transfers = control->get_register("start_transfers");

    enable_transfers(true);

    // test print instructions
    for (auto& inst : *instructions){
        std::cout << inst << "," << std::endl;
    }

    return 0;
}

uint32_t em_serial_controller::configure_baud_rate(uint32_t baud_rate){
    if(baud_rate < 115200){
        std::cerr << "Error: Baud rate too low" << std::endl;
        return 1;
    }
    
    uint32_t clock_cycles = 100e6 / baud_rate;
    
    if(clock_cycles * baud_rate != 100e6){
        std::cerr << "Warning: Exact baud rate not possible with 100MHz clock" << std::endl;
    }

    bit_length->set_value(clock_cycles);
    return 0;
}

uint32_t em_serial_controller::enable_transfers(bool enable){
    start_transfers->set_value(enable);
    return 0;
}

uint32_t em_serial_controller::autoconfigure_devices(){
    // TODO: implement
    return 1;
}

uint32_t em_serial_controller::run(){
    
    // update devices
    for(auto& device : devices){
        device->run();
    }

    // enable cyclic mode once its configured
    if(devices[0]->get_sequential_cmds_complete() && devices[0]->get_cyclic_config_complete() && !devices[0]->get_cyclic_data_enabled()){
        devices[0]->set_cyclic_data_enabled(true);
    }

    if(devices[0]->get_cyclic_data_enabled() || 0){
        double us = *microseconds;
        us /= 10e6;
        double value = 32767.0 + (32767.0 * sin(us * 2.0 * 3.14159));
        uint16_t value16 = value;
        value16 /= 2;
        value16 = 10000;
        devices[0]->sequential_write(3, &value16, 2);

        double val = 1.0 * sin(us * 2.0 * 3.14159);
        *cmd_q_current_milliamps = int32_t(val * 1000.0);
        //*cmd_q_current_milliamps = 1000;
    }

    devices[0]->set_enabled(true);

    return 0;
}



//////////////////////// em_serial_device ////////////////////////
em_serial_device::em_serial_device(Address_Map_Loader* loader, uint8_t device_number){

    ///////////////////////
    // TODO: get these values from the device config
    dev_info.hardware_type_addr = 0;
    dev_info.hardware_version_addr = 1;
    dev_info.firmware_version_addr = 2;
    dev_info.enable_cyclic_data_addr = 17;
    dev_info.cyclic_read_address_0_addr = 22;
    dev_info.cyclic_write_address_0_addr = 54;
    dev_info.cyclic_addresses = 32;

    ///////////////////////


    this->loader = loader;

    // create the registers

    auto device_group = loader->get_group("devices", device_number);

    // sync control register
    regs.control = device_group->get_register("control", 0);
    loader->sync_with_PS(regs.control);
    regs.control_enable = regs.control->get_register("enable");
    regs.control_enable_cyclic_data = regs.control->get_register("enable_cyclic_data");
    regs.control_rx_cyclic_packet_size = regs.control->get_register("rx_cyclic_packet_size");
    
    regs.control_rx_cyclic_packet_size->set_value(3);   // size for device address, control, and data (32 bit words not including crc)

    // sync status register
    regs.status = device_group->get_register("status", 0);
    loader->sync_with_PS(regs.status);
    regs.status_no_response = regs.status->get_register("no_rx_response_fault");
    regs.status_response_not_finished = regs.status->get_register("rx_not_finished_fault");
    regs.status_crc_invalid = regs.status->get_register("invalid_rx_crc_fault");


    // create cyclic registers (don't sync yet)
    for(int i = 0; i < max_cyclic_registers; i++){
        regs.cyclic_configs[i] = device_group->get_register("cyclic_config", i);
        regs.cyclic_reads[i] = device_group->get_register("cyclic_read_data", i);
        regs.cyclic_writes[i] = device_group->get_register("cyclic_write_data", i);

        raw_cyclic_configs[i] = raw_cyclic_config();

        cyclic_read_enabled[i] = false;
        cyclic_write_enabled[i] = false;
    }


    // sync cyclic read/write regs required for sequential commands
    loader->sync_with_PS(regs.cyclic_writes[0]);    // tx device address
    loader->sync_with_PS(regs.cyclic_writes[1]);    // tx sequential control
    loader->sync_with_PS(regs.cyclic_writes[2]);    // tx sequential data
    loader->sync_with_PS(regs.cyclic_reads[0]);     // rx device address    // TODO: make sure this only shows a matching address (probably should be done in hardware)
    loader->sync_with_PS(regs.cyclic_reads[1]);     // rx sequential control
    loader->sync_with_PS(regs.cyclic_reads[2]);     // rx sequential data

    // create dynamic register for cyclic config
    // this allows us to only take up one address in the PS and PL memory,
    // but adjust the DMA instruction to change what node/BRAM address is being used
    regs.cyclic_config = new Dynamic_Register(loader, regs.cyclic_configs[0]);
    regs.cyclic_config_reg = regs.cyclic_config->get_register();
    regs.cyclic_config_read_size = regs.cyclic_config_reg->get_register("cyclic_read_data_size");
    regs.cyclic_config_write_size = regs.cyclic_config_reg->get_register("cyclic_write_data_size");
    regs.cyclic_config_read_offset = regs.cyclic_config_reg->get_register("cyclic_read_data_starting_byte_index");
    regs.cyclic_config_write_offset = regs.cyclic_config_reg->get_register("cyclic_write_data_starting_byte_index");

    // regs.cyclic_config_read_size->set_value(1);
    // regs.cyclic_config_write_size->set_value(1);
    // regs.cyclic_config_read_offset->set_value(0);
    // regs.cyclic_config_write_offset->set_value(0);

    // setup cyclic configs for sequential commands
    configure_cyclic_read(0xffff, 1);    // device address
    configure_cyclic_read(0xffff, 4);    // sequential control
    configure_cyclic_read(0xffff, 4);    // sequential data

    configure_cyclic_write(0xffff, 1);    // device address
    configure_cyclic_write(0xffff, 4);    // sequential control
    configure_cyclic_write(0xffff, 4);    // sequential data

    //configure_cyclic_read(10, 2);    // read the DC bus voltage
    configure_cyclic_write(9, 2);    // write commutation data
    configure_cyclic_write(5, 4);

    //configure_cyclic_read(4, 2);

    // test NOPs
    // for(int i = 0; i < 1; i++){
    //     loader->instructions->push_back(create_instruction_NOP());
    // }

    //set_cyclic_read_pointer(10, &test_read);    // DC bus voltage

    // configure_cyclic_reg(0, 1, 0, true);    // 0 is the device address
    // configure_cyclic_reg(1, 4, 1, true);    // 1 is the sequential control
    // configure_cyclic_reg(2, 4, 1, true);    // 2 is the sequential data

    // configure_cyclic_reg(0, 1, 0, false);    // 0 is the device address
    // configure_cyclic_reg(1, 4, 1, false);    // 1 is the sequential control
    // configure_cyclic_reg(2, 4, 1, false);    // 2 is the sequential data

}

uint32_t em_serial_device::set_address(uint32_t address){
    if(address > 255){
        std::cerr << "Error: Address out of range" << std::endl;
        return 1;
    }
    this->address = address;
    regs.cyclic_writes[0]->set_value(address); // cyclic reg 0 is the address
    return 0;
}

uint32_t em_serial_device::get_address(uint16_t* address){
    *address = this->address;
    return 0;
}

uint32_t em_serial_device::set_enabled(bool enabled){
    if(!this->enabled && !cyclic_config_complete){
        return 1;   // cannot enable device until cyclic config is complete
    }

    this->enabled = enabled;
    return 0;
}

uint32_t em_serial_device::get_enabled(bool* enabled){
    *enabled = this->enabled;
    return 0;
}

uint32_t em_serial_device::set_cyclic_data_enabled(bool enabled){
    if(!this->enabled){
        return 1;   // cannot change cyclic data enabled until device is enabled
    }

    if(!cyclic_config_complete){
        return 2;   // cannot enable cyclic data until cyclic config is complete
    }

    if(enabled){
        add_sequential_cmd(dev_info.enable_cyclic_data_addr, &enabled, 1, true, &cyclic_data_enabled, nullptr);
    }
    else{
        add_sequential_cmd(dev_info.enable_cyclic_data_addr, &enabled, 1, true, nullptr, &cyclic_data_enabled);
    }
    return 0;
}

bool em_serial_device::get_cyclic_config_complete(){
    return cyclic_config_complete;
}

uint32_t em_serial_device::sequential_write(uint16_t address, void* data, uint8_t size){
    if(!this->enabled){
        return 1;   // cannot write until device is enabled
    }

    add_sequential_cmd(address, data, size, true, nullptr, nullptr);
    return 0;
}

uint32_t em_serial_device::sequential_read(uint16_t address, void* data, uint8_t size){
    if(!this->enabled){
        return 1;   // cannot read until device is enabled
    }

    add_sequential_cmd(address, data, size, false, nullptr, nullptr);
    return 0;
}

uint32_t em_serial_device::add_sequential_cmd(uint16_t address, void* data, uint8_t size, bool write, bool* complete_flag, bool* complete_flag_inverted){
    if(sequential_cmds.size() >= max_outstanding_cmds){
        std::cerr << "Error: Too many outstanding commands" << std::endl;
        return 1;   // too many outstanding commands
    }

    sequential_cmd cmd;
    cmd.address = address;
    cmd.size = size;
    cmd.write = write;
    cmd.complete_flag = complete_flag;
    cmd.complete_flag_inverted = complete_flag_inverted;

    memcpy(&cmd.data, data, size);

    sequential_cmds.push_back(cmd);
    sequential_cmds_complete = false;

    //std::cout << "Added sequential command: " << address << " " << data << std::endl;

    return 0;
}

uint32_t em_serial_device::configure_cyclic_read(uint16_t address, uint8_t size){
    if(cyclic_data_enabled){
        return 1;   // cannot configure while cyclic data is enabled
    }

    if(size > 4 || size == 0 || size == 3){
        return 2;   // size not supported
    }

    if(read_cyclic_configs.size() >= max_cyclic_registers-1){
        return 3;   // too many cyclic read configs
    }
    
    controller_cyclic_config new_config;

    if(read_cyclic_configs.size() == 0){
        new_config.packet_data_start_byte = 0;
    }
    else{
        new_config.packet_data_start_byte = read_cyclic_configs.back().packet_data_end_byte + 1;
    }

    new_config.address = address;
    new_config.size = size;
    new_config.packet_data_end_byte = new_config.packet_data_start_byte + size - 1;
    new_config.offset = new_config.packet_data_start_byte % 4;
    new_config.cyclic_data_register = regs.cyclic_reads[read_cyclic_configs.size()];

    read_cyclic_configs.push_back(new_config);

    uint8_t index = read_cyclic_configs.size()-1;
    raw_cyclic_configs[index].read_size = size;
    raw_cyclic_configs[index].read_offset = new_config.offset;

    outstanding_cyclic_configs.push_back(index);
    cyclic_config_complete = false;

    if(new_config.address != 0xffff && index > 2){   // if the address is 0xffff, then this is a special case for data that does not need configured
        // inexes 0-2 are always enabled for the device address, sequential control, and sequential data

        add_sequential_cmd(dev_info.cyclic_read_address_0_addr + index - 3, &address, 2, true, nullptr, nullptr); // configure the cyclic read reg in the device
    }

    return 0;
}

// TODO: rewrite this function to not be just a copy of the read function
uint32_t em_serial_device::configure_cyclic_write(uint16_t address, uint8_t size){
    if(cyclic_data_enabled){
        return 1;   // cannot configure while cyclic data is enabled
    }

    if(size > 4 || size == 0 || size == 3){
        return 2;   // size not supported
    }

    if(write_cyclic_configs.size() >= max_cyclic_registers-1){
        return 3;   // too many cyclic write configs
    }
    
    controller_cyclic_config new_config;

    if(write_cyclic_configs.size() == 0){
        new_config.packet_data_start_byte = 0;
    }
    else{
        new_config.packet_data_start_byte = write_cyclic_configs.back().packet_data_end_byte + 1;
    }

    new_config.address = address;
    new_config.size = size;
    new_config.packet_data_end_byte = new_config.packet_data_start_byte + size - 1;
    new_config.offset = new_config.packet_data_start_byte % 4;
    new_config.cyclic_data_register = regs.cyclic_writes[write_cyclic_configs.size()];

    write_cyclic_configs.push_back(new_config);

    uint8_t index = write_cyclic_configs.size()-1;
    raw_cyclic_configs[index].write_size = size;
    raw_cyclic_configs[index].write_offset = new_config.offset;

    outstanding_cyclic_configs.push_back(index);
    cyclic_config_complete = false;

    if(new_config.address != 0xffff && index > 2){   // if the address is 0xffff, then this is a special case for data that does not need configured
        // inexes 0-2 are always enabled for the device address, sequential control, and sequential data

        add_sequential_cmd(dev_info.cyclic_write_address_0_addr + index - 3, &address, 2, true, nullptr, nullptr); // configure the cyclic write reg in the device
    }

    return 0;
}

uint32_t em_serial_device::set_cyclic_read_pointer(uint16_t address, void** data){
    // find the cyclic read config with the matching address
    for(auto& config : read_cyclic_configs){
        if(config.address != address){
            continue;
        }
        
        if(config.cyclic_data_register->get_raw_data_ptr<uint32_t>() == nullptr){
            // not yet synced with the PS, so sync it
            loader->sync_with_PS(config.cyclic_data_register);
        }
        *data = config.cyclic_data_register->get_raw_data_ptr<uint32_t>();
        return 0;
        
    }
    return 1;   // address not found
}

uint32_t em_serial_device::set_cyclic_write_pointer(uint16_t address, void** data){
    // find the cyclic write config with the matching address
    for(auto& config : write_cyclic_configs){
        if(config.address != address){
            continue;
        }
        
        if(config.cyclic_data_register->get_raw_data_ptr<uint32_t>() == nullptr){
            // not yet synced with the PS, so sync it
            loader->sync_with_PS(config.cyclic_data_register);
        }
        *data = config.cyclic_data_register->get_raw_data_ptr<uint32_t>();
        return 0;
        
    }
    return 1;   // address not found
}

uint32_t em_serial_device::run_cylic_config(){
    if(outstanding_cyclic_configs.size() == 0){ // no commands to run, all done
        cyclic_config_complete = true;
        regs.cyclic_config->enable_sync(false);    // disable syncing with the PS
        return 0;
    }
    uint8_t index = outstanding_cyclic_configs[0];
    raw_cyclic_config* cmd = &raw_cyclic_configs[index];

    regs.cyclic_config->set_register(regs.cyclic_configs[index]);   // adjust which cyclic config register is being used
    // NOTE: when using dyamic registers, partial writes may cause issues, so you should write all values after each change
    regs.cyclic_config_read_size->set_value(cmd->read_size);
    regs.cyclic_config_write_size->set_value(cmd->write_size);
    regs.cyclic_config_read_offset->set_value(cmd->read_offset);
    regs.cyclic_config_write_offset->set_value(cmd->write_offset);
    regs.cyclic_config->enable_sync(true);    // enable syncing with the PS

    outstanding_cyclic_configs.erase(outstanding_cyclic_configs.begin());
    
    return 0;
}

uint32_t em_serial_device::run_sequential_cmds(){
    if(sequential_cmds.size() == 0){
        sequential_cmds_complete = true;
        
        regs.cyclic_writes[1]->set_value(0);
        regs.cyclic_writes[2]->set_value(0);

        consecutive_unknown_packet_errors = 0;
        consecutive_packet_errors = 0;

        return 0;   // no commands to run
    }

    // TODO: figure out why it is taking 3 cycles to complete and verify a command

    sequential_cmd* cmd = &sequential_cmds[0];

    // print all cmds for testing
    for(auto& cmd : sequential_cmds){
        //std::cout << "cmd: " << cmd.address << " " << cmd.data << std::endl;
    }

    // check if the last command is complete

    fault.no_response = regs.status_no_response->get_value();
    fault.response_not_finished = regs.status_response_not_finished->get_value();
    fault.crc_invalid = regs.status_crc_invalid->get_value();

    uint32_t status = regs.status->get_value();

    
    if(!fault.no_response && !fault.response_not_finished && !fault.crc_invalid){   // no faults
    //if(1){

        uint32_t control_response = regs.cyclic_reads[1]->get_value();
        if((control_response & 0xffff) == cmd->address && ((control_response >> 24) & 0b1) == 1){   // check if the address matches and the device signaled success
            if(cmd->complete_flag != nullptr){
                *cmd->complete_flag = true;
            }
            if(cmd->complete_flag_inverted != nullptr){
                *cmd->complete_flag_inverted = false;
            }

            consecutive_unknown_packet_errors = 0;
            sequential_cmds.erase(sequential_cmds.begin()); // move onto next command
            //std::cout << "cmd complete" << std::endl;

        }
        else if((control_response & 0xffff) == cmd->address && ((control_response >> 25) & 0b1) == 1){  // address matches, but device signalled failure
            // TODO: handle signalled failure
            //std::cerr << "Error: EM serial device signalled invalid command" << std::endl;
            sequential_cmds.erase(sequential_cmds.begin()); // move onto next command
            //std::cout << "cmd failed" << std::endl;
            consecutive_unknown_packet_errors = 0;
        }
        else{
            // TODO: handle unexpected result
            //std::cerr << "Error: EM serial device unexpected result" << std::endl;

            if(consecutive_unknown_packet_errors == 5){ // attempt to retry a few times
                sequential_cmds.erase(sequential_cmds.begin()); // move onto next command
                consecutive_unknown_packet_errors = 0;
            }
            else{
                consecutive_unknown_packet_errors++;
            }
            //std::cout << "unknown result" << std::endl;

        }

        consecutive_packet_errors = 0;
    }
    else{
        consecutive_packet_errors++;
        if(consecutive_packet_errors == 10){
            std::cerr << "Error: EM serial device has too many consecutive packet errors" << std::endl;
        }
    }


    // run the command

    if(sequential_cmds.size() == 0){
        sequential_cmds_complete = true;
        
        regs.cyclic_writes[1]->set_value(0);
        regs.cyclic_writes[2]->set_value(0);

        consecutive_unknown_packet_errors = 0;
        consecutive_packet_errors = 0;

        return 0;   // no commands to run
    }

    cmd = &sequential_cmds[0];  // get the current command

    uint32_t control = cmd->address;
    control |= cmd->write << 16;    // set the write bit

    regs.cyclic_writes[1]->set_value(control); // cyclic reg 1 is sequential control
    //std::cout << "control: " << (control & 0xffff) << std::endl;

    if(cmd->write){
        regs.cyclic_writes[2]->set_value(cmd->data); // cyclic reg 2 is the sequential data to write
        //std::cout << "write: " << cmd->data << std::endl;
    }
    else{
        regs.cyclic_writes[2]->set_value(0); // send zero when reading
        //std::cout << "read" << std::endl;
    }
    
    return 0;
}

bool em_serial_device::get_sequential_cmds_complete(){
    return sequential_cmds_complete;
}

bool em_serial_device::get_cyclic_data_enabled(){
    return cyclic_data_enabled;
}

uint32_t em_serial_device::run(){
    // run the device, must be called at each FPGA update

    bool old_cyclic_data_enabled = cyclic_data_enabled;

    if(initial_config_complete){
        run_sequential_cmds();
    }

    run_cylic_config();

    regs.control_enable->set_value(enabled);

    if(old_cyclic_data_enabled && !cyclic_data_enabled){
        // cyclic data was just disabled
        regs.control_enable_cyclic_data->set_value(0);
    }
    else if(!old_cyclic_data_enabled && cyclic_data_enabled){
        // cyclic data was just enabled
        regs.control_enable_cyclic_data->set_value(1);

        uint16_t rx_cyclic_packet_size = read_cyclic_configs.back().packet_data_end_byte + 1;
        rx_cyclic_packet_size += 4 - rx_cyclic_packet_size % 4;    // round up to the nearest 32 bit word
        rx_cyclic_packet_size /= 4; // convert to 32 bit words
        regs.control_rx_cyclic_packet_size->set_value(rx_cyclic_packet_size);
    }

    if(cyclic_config_complete){
        initial_config_complete = true;
    }

    return 0;
}
