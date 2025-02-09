#include "serial_interface_card.h"

static Driver_Registrar<serial_interface_card> registrar("serial_interface_card");

serial_interface_card::serial_interface_card(){
}

serial_interface_card::~serial_interface_card(){
}

uint32_t serial_interface_card::load_config(json* config, std::string module_name, Node_Core* node_core, fpga_instructions* fpga_instr){
    this->node_core = node_core;
    this->fpga_instr = fpga_instr;
    this->config = config;

    loader.setup(module_name, config, &base_mem);

    node_address = loader.get_node_index();

    if(custom_load_config() != 0){
        std::cerr << "Failed to load custom config" << std::endl;
        return 1;
    }

    return 0;
}


uint32_t serial_interface_card::custom_load_config(){

    // load register from config json
    auto port_mode_enable = loader.get_register("port_mode_enable", 0);

    // create PS/PL node varaiable to sync with
    /* 
    
    TODO: make these objects

    should be accessible like global variables in the node editor

    take in the register object and a name when creating

    support dynamic register objects

    contains a copy object for fpga instructions

    copy object may update the related module's copy memory address directly in the instruction vector once compiled (for dynamic registers)

    node object replaces how registers currently handle assigning and accessing PS memory

    */

    //loader.sync_with_PS(port_mode_enable);
    // auto cpy = loader.sync_with_PS_new(port_mode_enable);
    // cpy->set_time_reference(0);
    // cpy->set_execution_window(0, 1000);
    // cpy->set_write_priority();
    regs2.rs485_mode_enable = port_mode_enable->get_register("rs485_mode_enable");
    regs2.rs422_mode_enable = port_mode_enable->get_register("rs422_mode_enable");
    regs2.quadrature_mode_enable = port_mode_enable->get_register("rs422_mode_enable");

    auto i2c_config = loader.get_register("i2c_config", 0);
    //loader.sync_with_PS(i2c_config);
    regs2.i2c_config_read_mode = i2c_config->get_register("read");
    regs2.i2c_config_device_address = i2c_config->get_register("device_address");
    regs2.i2c_config_reg_address = i2c_config->get_register("register_address");
    regs2.i2c_config_data_length = i2c_config->get_register("byte_count");
    regs2.i2c_config_start = i2c_config->get_register("start");

    regs2.i2c_data_tx = loader.get_register("i2c_data_tx", 0);
    //loader.sync_with_PS(regs2.i2c_data_tx);
    regs2.i2c_data_rx = loader.get_register("i2c_data_rx", 0);
    //loader.sync_with_PS(regs2.i2c_data_rx);

    auto i2c_status = loader.get_register("i2c_status", 0);
    //loader.sync_with_PS(i2c_status);
    regs2.i2c_status_busy = i2c_status->get_register("busy");
    regs2.i2c_status_error = i2c_status->get_register("error");
    

    // test print instructions
    // for (auto& inst : *instructions){
    //     std::cout << "Instruction: " << inst << std::endl;
    // }

    #define TOP_DEVICE_ADDRESS 0x20
    #define MIDDLE_DEVICE_ADDRESS 0x21
    #define BOTTOM_DEVICE_ADDRESS 0x22

    #define INPUT_PORT_0 0x0
    #define INPUT_PORT_1 0x1
    #define OUTPUT_PORT_0 0x2
    #define OUTPUT_PORT_1 0x3
    #define CONFIG_PORT_0 0x6
    #define CONFIG_PORT_1 0x7

    // setup i2c transfers that will continuously run
    i2c_transfer transfer;

    // write to configuration ports for all devices/ports
    transfer.read = false;
    transfer.reg_address = CONFIG_PORT_0;
    transfer.data_length = 2;

    transfer.device_address = TOP_DEVICE_ADDRESS;
    transfer.data = reinterpret_cast<uint32_t*>(&io_expander_data[0].configuration);
    i2c_transfers.push_back(transfer);
    transfer.device_address = MIDDLE_DEVICE_ADDRESS;
    transfer.data = reinterpret_cast<uint32_t*>(&io_expander_data[1].configuration);
    i2c_transfers.push_back(transfer);
    transfer.device_address = BOTTOM_DEVICE_ADDRESS;
    transfer.data = reinterpret_cast<uint32_t*>(&io_expander_data[2].configuration);
    i2c_transfers.push_back(transfer);

    // write to output ports for all devices/ports
    transfer.read = false;
    transfer.reg_address = OUTPUT_PORT_0;
    transfer.data_length = 2;

    transfer.device_address = TOP_DEVICE_ADDRESS;
    transfer.data = reinterpret_cast<uint32_t*>(&io_expander_data[0].outputs);
    i2c_transfers.push_back(transfer);
    transfer.device_address = MIDDLE_DEVICE_ADDRESS;
    transfer.data = reinterpret_cast<uint32_t*>(&io_expander_data[1].outputs);
    i2c_transfers.push_back(transfer);
    transfer.device_address = BOTTOM_DEVICE_ADDRESS;
    transfer.data = reinterpret_cast<uint32_t*>(&io_expander_data[2].outputs);
    i2c_transfers.push_back(transfer);

    // read from input port for last port of last device
    transfer.read = true;
    transfer.reg_address = INPUT_PORT_1;
    transfer.data_length = 1;
    transfer.data = reinterpret_cast<uint32_t*>(&io_expander_data[2].inputs);
    i2c_transfers.push_back(transfer);


    // configure IO expander ports
    io_expander_data[0].configuration = 0x0; // all outputs
    io_expander_data[1].configuration = 0x0; // all outputs
    io_expander_data[2].configuration = 0b00111000 << 8; // all outputs except for status inputs

    // for(int i = 0; i < 10; i++){
    //     configure_port_mode(i, port_mode::RS485);
    // }

    //////////////// HARDCODED SECTION ////////////////
    led_modes[0] = led_mode::BLINK_SLOW;
    led_modes[1] = led_mode::BLINK_MED;
    led_modes[2] = led_mode::BLINK_FAST;

    power_out_enable = true;

    configure_port_mode(9, port_mode::RS422);   // servo drive
    configure_port_mode(0, port_mode::RS422);   // encoder

    //////////////// END HARDCODED SECTION ////////////////


    return 0;
}

uint32_t serial_interface_card::run(){
    // run stuff

    // update leds
    uint8_t led_states = 0;
    for(int i = 0; i < 3; i++){
        switch (led_modes[i])
        {
        case led_mode::OFF:
            led_states |= 0b0 << i;
            break;

        case led_mode::ON:
            led_states |= 0b1 << i;
            break;

        case led_mode::BLINK_SLOW:
            led_states |= (*microseconds >> 21 & 0b1) << i;   // ~ .25 Hz
            break;

        case led_mode::BLINK_MED:
            led_states |= (*microseconds >> 19 & 0b1) << i;   // ~ 1 Hz
            break; 

        case led_mode::BLINK_FAST:
            led_states |= (*microseconds >> 17 & 0b1) << i;   // ~ 4 Hz
            break; 
        
        default:
            led_states |= 0b1 << i;
            break;
        }
    }

    io_expander_data[2].outputs = ((~led_states & 0b111) << 8) | (power_out_enable << 14);

    // update power ok signals
    power_out_5v5_1_ok = io_expander_data[2].inputs >> 13 & 0b1;
    power_out_5v5_2_ok = io_expander_data[2].inputs >> 15 & 0b1;
    power_out_24v_ok = io_expander_data[2].inputs >> 14 & 0b1; 

    run_i2c_transfers();
    return 0;
}

void serial_interface_card::run_i2c_transfers(){
    // should be called in the run function

    if(regs2.i2c_status_busy->get_value()){ // busy bit set
        // TODO: might want to add a timeout here
        return;
    }


    if(regs2.i2c_status_error->get_value()){ // error bit set
        i2c_consecutive_error_count++;
    }
    else{

        if(i2c_transfers[last_i2c_transfer].read){  // if a read was just completed, save the data from the rx register
            *i2c_transfers[last_i2c_transfer].data = regs2.i2c_data_rx->get_value();
        }

        last_i2c_transfer++;
        if(last_i2c_transfer >= i2c_transfers.size()){
            last_i2c_transfer = 0;
        }
        i2c_consecutive_error_count = 0;
    }

    i2c_transfer transfer = i2c_transfers[last_i2c_transfer];

    regs2.i2c_config_read_mode->set_value(transfer.read);
    regs2.i2c_config_data_length->set_value(transfer.data_length);
    regs2.i2c_config_device_address->set_value(transfer.device_address);
    regs2.i2c_config_reg_address->set_value(transfer.reg_address);
    regs2.i2c_config_start->set_value(1);

    
    if(!transfer.read){ // if its a write, copy the data to the tx register
        regs2.i2c_data_tx->set_value(*transfer.data);
    }

}

uint32_t serial_interface_card::configure_port_mode(uint8_t port, port_mode mode){
    if(port > 9){
        std::cerr << "Error: Port out of range" << std::endl;
        return 1;
    }

    // configs are in the order: 3, 1, 2, 4
    #define RS485_CONFIG 0b0001
    #define RS422_CONFIG 0b1110
    #define BOTH_INPUTS_CONFIG 0b0010

    uint8_t config = 0;

    if(mode == port_mode::RS485){
        config = RS485_CONFIG;
    }
    else if(mode == port_mode::RS422){
        config = RS422_CONFIG;
    }
    else{
        config = BOTH_INPUTS_CONFIG;
    }

    switch (port)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        io_expander_data[0].configuration = (io_expander_data[0].configuration & ~(0xf << 4*(3-port))) | (config << 4*(3-port));
        break;

    case 4:
    case 5:
    case 6:
    case 7:
        io_expander_data[1].configuration = (io_expander_data[1].configuration & ~(0xf << 4*(7-port))) | (config << 4*(7-port));
        break;

    case 8:
    case 9:
        io_expander_data[2].configuration = (io_expander_data[2].configuration & ~(0xf << 4*(9-port))) | (config << 4*(9-port));
        break;
    
    default:
        break;
    }

    port_modes[port] = mode;

    uint32_t hdl_config = 0;
    for(int i = 0; i < 10; i++){
        if(port_modes[i] == port_mode::RS485){
            hdl_config |= 0b1 << i;
        }
        else if(port_modes[i] == port_mode::RS422){
            hdl_config |= 0b1 << (i+10);
        }

        // else{   // both inputs (not yet implemented)
        //     hdl_config |= 0b1 << (i+20);
        // }
    }

    *(regs2.rs485_mode_enable->get_raw_data_ptr<uint32_t>()) = hdl_config;

    return 0;
}