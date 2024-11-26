#include "serial_interface_card.h"

static Driver_Registrar<serial_interface_card> registrar("serial_interface_card");

serial_interface_card::serial_interface_card(){
}

serial_interface_card::~serial_interface_card(){
}

uint32_t serial_interface_card::load_config_pre(json config){
    base_mem.PS_PL_size = 3 * 4;
    base_mem.PL_PS_size = 2 * 4;

    if(!load_json_value(config, "node_address", &node_address)){
        std::cerr << "Failed to load node address" << std::endl;
        return 1;
    }

    return 0;
}

uint32_t serial_interface_card::load_config_post(json config){

    uint32_t offset = 0;
    bool ret = true;
    json reg_data;

    // TODO: load these based on the config file instead of hardcoding (these offsets are NOT based on the config, theyre just incrementing)

    regs.port_mode_enable = reinterpret_cast<uint32_t*>(base_mem.PS_PL_ptr) + 0x0;
    regs.i2c_config = reinterpret_cast<uint32_t*>(base_mem.PS_PL_ptr) + 0x1;
    regs.i2c_data_tx = reinterpret_cast<uint32_t*>(base_mem.PS_PL_ptr) + 0x2;

    regs.i2c_data_rx = reinterpret_cast<uint32_t*>(base_mem.PL_PS_ptr) + 0x0;
    regs.i2c_status = reinterpret_cast<uint32_t*>(base_mem.PL_PS_ptr) + 0x1;


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

    for(int i = 0; i < 10; i++){
        configure_port_mode(i, port_mode::RS485);
    }

    // for(int i = 0; i < 3; i++){
    //     led_modes[i] = led_mode::BLINK_MED;
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

uint32_t serial_interface_card::get_base_instructions(std::vector<uint64_t>* instructions){
    uint64_t instruction = 0;

    // TODO: make this more automated based on config file
    
    // TODO: make sure this actually works....

    // the first address is based on the above offsets made in load_config_post
    // the secondary address is from the node config file
    // copy from PS to PL
    instruction = create_instruction_COPY(0, base_mem.PS_PL_mem_offset + 0x0, node_address, 0x0); // copy port_mode_enable to PL
    instructions->push_back(instruction);
    instruction = create_instruction_COPY(0, base_mem.PS_PL_mem_offset + 0x1, node_address, 0x1); // copy i2c_config to PL
    instructions->push_back(instruction);
    instruction = create_instruction_COPY(0, base_mem.PS_PL_mem_offset + 0x2, node_address, 0x3); // copy i2c_data_tx to PL
    instructions->push_back(instruction);

    // copy from PL to PS
    instruction = create_instruction_COPY(node_address, 0x0, 0, base_mem.PL_PS_mem_offset + 0x0); // copy i2c_data_rx to PS
    instructions->push_back(instruction);
    instruction = create_instruction_COPY(node_address, 0x1, 0, base_mem.PL_PS_mem_offset + 0x1); // copy i2c_status to PS
    instructions->push_back(instruction);


    // test copy
    //instruction = create_instruction_COPY(0, base_mem.PS_PL_mem_offset + 0x0, 0, base_mem.PL_PS_mem_offset + 0x0);
    //instructions->push_back(instruction);

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

    if(*regs.i2c_status & 0b1){ // busy bit set
        // TODO: might want to add a timeout here
        return;
    }


    if(*regs.i2c_status & 0b10){ // error bit set
        i2c_consecutive_error_count++;
    }
    else{

        if(i2c_transfers[last_i2c_transfer].read){  // if a read was just completed, save the data from the rx register
            *i2c_transfers[last_i2c_transfer].data = *regs.i2c_data_rx;
        }

        last_i2c_transfer++;
        if(last_i2c_transfer >= i2c_transfers.size()){
            last_i2c_transfer = 0;
        }
        i2c_consecutive_error_count = 0;
    }

    i2c_transfer transfer = i2c_transfers[last_i2c_transfer];

    *regs.i2c_config = transfer.read | ((uint32_t)transfer.device_address << 1) | ((uint32_t)transfer.reg_address << 8) | ((uint32_t)transfer.data_length << 16) | ((uint32_t)1 << 19);
    
    if(!transfer.read){ // if its a write, copy the data to the tx register
        *regs.i2c_data_tx = *transfer.data;
    }

}

uint32_t serial_interface_card::configure_port_mode(uint8_t port, port_mode mode){
    if(port > 9){
        std::cerr << "Error: Port out of range" << std::endl;
        return 1;
    }

    // configs are in the order: 3, 1, 2, 4
    #define RS485_CONFIG 0b1000
    #define RS422_CONFIG 0b0111
    #define BOTH_INPUTS_CONFIG 0b0100

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

    *regs.port_mode_enable = hdl_config;

    return 0;
}