from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from enum import IntEnum, auto
from amaranth.lib.wiring import Component, In, Out
from em_serial_port import EM_Serial_Port
from amaranth.lib.cdc import FFSynchronizer
from registers2 import *
import numpy as np


class em_serial_controller(Component):
    """
    Specific serial interface for managing communication with our own devices at high speeds
    """

    def __init__(self, max_packet_size:int, max_number_of_devices:int, debug:bool = False) -> None:
        """
        max_cyclic_registers: maximum number of cyclic rx/tx registers per device (up to 32 bit each) (255 absolute max)

        max_number_of_devices: maximum number of different devices allowed to be connected to a single port (address limit is 255, but smaller values will consume less FPGA memory)
        """

        assert max_packet_size <= 255, "max_cyclic_registers must be less than or equal to 255"
        assert max_number_of_devices <= 255, "max_number_of_devices must be less than or equal to 255"

        self.debug = debug
        self.clock = 100e6

        self.max_packet_size = max_packet_size
        self.max_number_of_devices = max_number_of_devices

        super().__init__({
            "bram_address": In(16),
            "bram_write_data": In(32),
            "bram_read_data": Out(32),
            "bram_write_enable": In(1),

            "rx": In(1),
            "tx": Out(1)
        })

        driver_settings = {}
        self.rm = RegisterMapGenerator("em_serial_controller", ["em_serial_controller"], driver_settings, "Serial controller for managing communication with our own devices at high speeds")

        # main registers are placed at the end of the memory range (outside of the actual bram)
        self.rm.add(Register("control", rw="w", desc="Global control register", sub_registers=[
            Register("start_transfers", type="bool", desc="Start sequential write/read from all configured devices and update internal memory")
        ]))
        self.rm.add(Register("bit_length", rw="w", desc="Bit length in clock cycles (minimum allowed is equal to 115200 baud)"))
        self.rm.add(Register("status", rw="r", desc="Status register", sub_registers=[
            Register("update_busy", type="bool", desc="Update busy"),
            Register("update_done", type="bool", desc="Update done"),
            Register("update_error", type="bool", desc="Update error")
        ]))

        # device registers
        g = Group("devices", desc="Per-device registers", count=max_number_of_devices, start_address=0x0)
        g.add(Register("control", rw="w", desc="Device control register", start_address=0x0, sub_registers=[
            Register("enable", type="bool", desc="Enable device"),
            Register("enable_cyclic_data", type="bool", desc="Enable cyclic data"),
            Register("rx_cyclic_packet_size", width=8, desc="Expected cyclic RX packet size in 32bit words not including CRC") # (must be at least 3 fro header data)
        ]))
        g.add(Register("status", rw="r", desc="Device status register", start_address=0x1, sub_registers=[
            Register("no_rx_response_fault", type="bool", desc="No rx response fault"),
            Register("rx_not_finished_fault", type="bool", desc="Rx not finished fault"),
            Register("invalid_rx_crc_fault", type="bool", desc="Invalid rx CRC fault")
        ]))
        g.add(Register("cyclic_config", rw="w", desc="Cyclic config register", start_address=self.max_packet_size, bank_size=max_packet_size, sub_registers=[
            Register("cyclic_read_data_size", width=3, desc="Cyclic read register data size (bytes)"),  # 0 = 1 byte, 1 = 2 bytes, 2 = 3 bytes, 3 = 4 bytes
            Register("cyclic_read_data_starting_byte_index", width=2, desc="Cyclic read register starting byte index in 32bit word"),
            Register("cyclic_write_data_size", width=3, desc="Cyclic write register data size (bytes)"),
            Register("cyclic_write_data_starting_byte_index", width=2, desc="Cyclic read register starting byte index in 32bit word")
        ]))
        g.add(Register("cyclic_read_data", rw="r", desc="Cyclic read data register", start_address=self.max_packet_size*2, bank_size=max_packet_size))
        g.add(Register("cyclic_write_data", rw="w", desc="Cyclic write data register", start_address=self.max_packet_size*3, bank_size=max_packet_size))

        self.rm.add(g)
        self.rm.generate()

        self.deviceRXdelay = 2  # how long in word transfers in addition to expected transfer to wait for RX packets to finish

        # ensure max packet size is a power of 2
        if(self.max_packet_size & (self.max_packet_size - 1) != 0):
            # if not a power of 2, round up to the next power of 2
            self.max_packet_size = 2**int(self.max_packet_size.bit_length())
            print(f"max_packet_size must be a power of 2, rounding up to {self.max_packet_size}")

        self.serialPort = EM_Serial_Port(self.max_packet_size)

        if(self.debug):
            self.debugSerialPort = EM_Serial_Port(self.max_packet_size)
        

    def elaborate(self, platform):
        m = Module()

        m.submodules.drive_serial_port = self.serialPort
        m.d.comb += self.serialPort.rx.eq(self.rx)
        m.d.comb += self.tx.eq(self.serialPort.tx)

        if(self.debug):
            m.submodules.debug_drive_serial_port = self.debugSerialPort
            m.d.comb += self.debugSerialPort.rx.eq(self.serialPort.tx)
            m.d.comb += self.serialPort.rx.eq(self.debugSerialPort.tx)

        self.address_ToSerialPort = self.serialPort.bram_address
        self.writeData_ToSerialPort = self.serialPort.bram_write_data
        self.readData_ToSerialPort = self.serialPort.bram_read_data
        self.writeEnable_ToSerialPort = self.serialPort.bram_write_enable

        total_memory = self.max_packet_size*4*self.max_number_of_devices
        m.submodules.memory = self.memory = Memory(shape=unsigned(32), depth=total_memory, init=[])     # memory for all devices

        self.externalReadPort = self.memory.read_port(domain="sync_100")
        self.externalWritePort = self.memory.write_port(domain="sync_100")
        self.internalReadPort = self.memory.read_port(domain="sync_100")
        self.internalWritePort = self.memory.write_port(domain="sync_100")

        # connect internal memory interface
        self.internalBramAddress = Signal(16)
        self.internalBramReadData = Signal(32)
        self.internalBramWriteData = Signal(32)
        self.internalBramWriteEnable = Signal()
        m.d.comb += self.internalReadPort.addr.eq(self.internalBramAddress)
        m.d.comb += self.internalWritePort.addr.eq(self.internalBramAddress)
        m.d.comb += self.internalWritePort.data.eq(self.internalBramWriteData)
        m.d.comb += self.internalWritePort.en.eq(self.internalBramWriteEnable)
        m.d.comb += self.internalBramReadData.eq(self.internalReadPort.data)


        self.start_transfers = Signal()
        self.bit_time = Signal(32)
        self.update_busy = Signal()
        self.update_done = Signal()
        self.update_error = Signal()

        self.rx_invalid_crc_fault = Signal()
        self.rx_not_finished_fault = Signal()
        self.rx_no_response_fault = Signal()

        self.tx_data = Signal(32)
        self.rx_data = Signal(32)

        self.current_device_index = Signal(8)

        self.tx_packet_size = Signal(range(self.max_packet_size))
        self.rx_packet_size = Signal(range(self.max_packet_size))
        self.previous_rx_packet_size = Signal(range(self.max_packet_size+1))

        self.current_tx_byte_index = Signal(range(4))
        self.current_rx_byte_index = Signal(range(4))
        self.current_rx_word_index = Signal(range(8))
        self.current_tx_word_index = Signal(range(8))

        self.timer = Signal(range(self.max_packet_size+1))    # long enough to count up to the biggest expected packet 
        self.pre_timer = Signal(range(int(self.clock/115200 * 10 * 4) + 1))     # long enough to count up to 1 word
        self.update_word_time = Signal()
        self.word_time = Signal(range(int(clock/115200 * 10 * 4) + 1))

        self.current_cyclic_register = Signal(range(self.max_packet_size))

        self.cyclic_register_size = Signal(3)
        self.cyclic_register_starting_byte_index = Signal(range(4))
        self.cyclic_register_starting_bit_index = Signal(range(32))
        m.d.comb += self.cyclic_register_starting_bit_index.eq(self.cyclic_register_starting_byte_index<<3)

        self.cyclic_data_enabled = Signal()


        device_group_offset = int(np.log2(self.rm.devices.alignment))   # offset for device group sections

        cyclic_read_data_size = Signal(3)
        cyclic_read_data_starting_byte_index = Signal(2)
        cyclic_write_data_size = Signal(3)
        cyclic_write_data_starting_byte_index = Signal(2)

        m.d.comb += [
            cyclic_read_data_size.eq(self.internalBramReadData[self.rm.devices.cyclic_config.cyclic_read_data_size.starting_bit:self.rm.devices.cyclic_config.cyclic_read_data_size.starting_bit+self.rm.devices.cyclic_config.cyclic_read_data_size.width]),
            cyclic_read_data_starting_byte_index.eq(self.internalBramReadData[self.rm.devices.cyclic_config.cyclic_read_data_starting_byte_index.starting_bit:self.rm.devices.cyclic_config.cyclic_read_data_starting_byte_index.starting_bit+self.rm.devices.cyclic_config.cyclic_read_data_starting_byte_index.width]),
            cyclic_write_data_size.eq(self.internalBramReadData[self.rm.devices.cyclic_config.cyclic_write_data_size.starting_bit:self.rm.devices.cyclic_config.cyclic_write_data_size.starting_bit+self.rm.devices.cyclic_config.cyclic_write_data_size.width]),
            cyclic_write_data_starting_byte_index.eq(self.internalBramReadData[self.rm.devices.cyclic_config.cyclic_write_data_starting_byte_index.starting_bit:self.rm.devices.cyclic_config.cyclic_write_data_starting_byte_index.starting_bit+self.rm.devices.cyclic_config.cyclic_write_data_starting_byte_index.width]),
        ]

        with m.If(self.update_word_time):
            # set word time in clock cycles for later use
            m.d.sync_100 += self.word_time.eq(self.bit_time * (10*4))
            m.d.sync_100 += self.update_word_time.eq(0)

        use_previous_device_bram = Signal()
        with m.If(use_previous_device_bram):
            m.d.sync_100 += self.internalBramAddress[device_group_offset:].eq(self.current_device_index-1)
        with m.Else():
            m.d.sync_100 += self.internalBramAddress[device_group_offset:].eq(self.current_device_index)            
        device_register_address = self.internalBramAddress[:device_group_offset]


        self.current_tx_bit_index = Signal(range(32))
        m.d.comb += self.current_tx_bit_index.eq(self.current_tx_byte_index<<3)

        self.internal_read_port_masked_bytes = Signal(32)
        m.d.comb += self.internal_read_port_masked_bytes.eq(self.internalReadPort.data & (0xFFFFFFFF >> (32 - (self.cyclic_register_size<<3)).as_unsigned()))

        self.internal_read_port_shifted_masked_bytes = Signal(32)
        m.d.comb += self.internal_read_port_shifted_masked_bytes.eq(((self.internalReadPort.data >> self.cyclic_register_starting_bit_index) & (0xFFFFFFFF >> (32 - (self.cyclic_register_size<<3)).as_unsigned())))
        
        serial_port_control_data = Signal(32)
        serial_port_tx_trigger = Signal()
        serial_port_rx_trigger = Signal()
        m.d.comb += serial_port_control_data.eq(
            (serial_port_tx_trigger << self.serialPort.rm.control.tx_start.starting_bit) |
            (serial_port_rx_trigger << self.serialPort.rm.control.rx_start.starting_bit) |
            (self.tx_packet_size << self.serialPort.rm.control.tx_packet_size.starting_bit) |
            (self.rx_packet_size << self.serialPort.rm.control.rx_packet_size.starting_bit))
        
        serial_port_tx_done = Signal()
        serial_port_rx_done = Signal()
        serial_port_tx_busy = Signal()
        serial_port_rx_busy = Signal()
        serial_port_rx_crc_valid = Signal()
        m.d.comb += serial_port_tx_done.eq(self.readData_ToSerialPort[self.serialPort.rm.status.tx_done.starting_bit])
        m.d.comb += serial_port_rx_done.eq(self.readData_ToSerialPort[self.serialPort.rm.status.rx_done.starting_bit])
        m.d.comb += serial_port_tx_busy.eq(self.readData_ToSerialPort[self.serialPort.rm.status.tx_busy.starting_bit])
        m.d.comb += serial_port_rx_busy.eq(self.readData_ToSerialPort[self.serialPort.rm.status.rx_busy.starting_bit])
        m.d.comb += serial_port_rx_crc_valid.eq(self.readData_ToSerialPort[self.serialPort.rm.status.rx_crc_valid.starting_bit])
        
        self.memory_read_delay = Signal()
        with m.If(self.memory_read_delay):
            m.d.sync_100 += self.memory_read_delay.eq(0)

        
        # handle bram interface

        self.bram_control_mode = Signal()
        with m.If((self.bram_address == self.rm.control.address_offset) | (self.bram_address == self.rm.bit_length.address_offset) | (self.bram_address == self.rm.status.address_offset)):
            m.d.sync_100 += self.bram_control_mode.eq(1)
        with m.Else():
            m.d.sync_100 += self.bram_control_mode.eq(0)

        # handle read/write for control/status registers
        with m.If(self.bram_write_enable):
            with m.Switch(self.bram_address):
                with m.Case(self.rm.control.address_offset):
                    m.d.sync_100 += self.start_transfers.eq(self.bram_write_data[self.rm.control.start_transfers.starting_bit])
                
                with m.Case(self.rm.bit_length.address_offset):
                    m.d.sync_100 += self.bit_time.eq(self.bram_write_data)
                    m.d.sync_100 += self.update_word_time.eq(1)

        with m.Switch(self.bram_address):
            with m.Case(self.rm.status.address_offset):
                with m.If(self.bram_control_mode):
                    m.d.comb += self.bram_read_data.eq(Cat(self.update_busy, self.update_done, self.update_error))
        
        # handle read/write for data registers
        m.d.comb += self.externalReadPort.addr.eq(self.bram_address)
        m.d.comb += self.externalWritePort.addr.eq(self.bram_address)
        with m.If(~self.bram_control_mode):
            m.d.comb += self.externalWritePort.data.eq(self.bram_write_data)
            m.d.comb += self.externalWritePort.en.eq(self.bram_write_enable)
            m.d.comb += self.bram_read_data.eq(self.externalReadPort.data)
        
        


        with m.FSM(init="idle", domain="sync_100", name="controller_fsm") as fsm:

            with m.State("idle"):
                with m.If(self.start_transfers):
                    m.next = "reset_states"

            with m.State("reset_states"):
                m.d.sync_100 += [
                    self.start_transfers.eq(0),
                    self.update_busy.eq(1),
                    self.update_done.eq(0),
                    self.update_error.eq(0),
                    self.current_device_index.eq(0),
                    self.current_tx_byte_index.eq(0),
                    self.current_rx_byte_index.eq(0),
                    self.current_rx_word_index.eq(0),
                    self.current_tx_word_index.eq(0),
                    self.current_cyclic_register.eq(0),
                    self.timer.eq(0),
                    self.pre_timer.eq(0),
                    
                    # TODO: reset timers
                ]
                m.next = "get_tx_device_config_wait"

            with m.State("get_tx_device_config_wait"):
                
                
                m.d.sync_100 += device_register_address.eq(self.rm.devices.control.address_offset)
                m.d.sync_100 += self.memory_read_delay.eq(1)
                
                with m.If(self.memory_read_delay):
                    m.next = "get_tx_device_config"


            with m.State("get_tx_device_config"):
                with m.If(self.internalBramReadData[self.rm.devices.control.enable.starting_bit]):  # if device is enabled
                    # read configuration
                    m.d.sync_100 += [
                        self.cyclic_data_enabled.eq(self.internalBramReadData[self.rm.devices.control.enable_cyclic_data.starting_bit]),
                        self.previous_rx_packet_size.eq(self.rx_packet_size),
                        device_register_address.eq(self.rm.devices.cyclic_config.address_offset + self.current_cyclic_register),
                    ]

                    with m.If(self.cyclic_data_enabled):
                        m.d.sync_100 += self.rx_packet_size.eq(self.internalBramReadData[self.rm.devices.control.rx_cyclic_packet_size.starting_bit:self.rm.devices.control.rx_cyclic_packet_size.starting_bit+self.rm.devices.control.rx_cyclic_packet_size.width]),
                    with m.Else():
                        m.d.sync_100 += self.rx_packet_size.eq(3)    # minimum packet size for cyclic data
                    
                    m.next = "get_tx_register_config"

                with m.Elif(self.current_device_index != (self.max_number_of_devices-1)):   # skip next device if the current one is disabled and we still have devices left to try
                    m.d.sync_100 += [
                        self.current_device_index.eq(self.current_device_index + 1),
                        self.memory_read_delay.eq(1)
                    ]
                    m.next = "get_tx_device_config_wait"

                with m.Else():  # no more devices to send packets for, receive the last RX packet
                    m.d.sync_100 += self.timer.eq(self.rx_packet_size+self.deviceRXdelay)
                    m.d.sync_100 += self.pre_timer.eq(self.word_time)
                    m.next = "wait_for_final_rx_packet"
                    pass

            
            with m.State("get_tx_register_config"):
                m.d.sync_100 += self.writeEnable_ToSerialPort.eq(0)
                m.d.sync_100 += self.address_ToSerialPort.eq(self.serialPort.rm.tx_data.address_offset + self.current_tx_word_index)

                m.d.sync_100 += device_register_address.eq(self.rm.devices.cyclic_write_data.address_offset + self.current_cyclic_register)

                m.next = "get_tx_register_data"


            with m.State("get_tx_register_data"):
                with m.If((cyclic_write_data_size != 0) & ((self.current_cyclic_register < 3) | (self.cyclic_data_enabled))):
                    m.d.sync_100 += self.cyclic_register_size.eq(cyclic_write_data_size)
                    m.d.sync_100 += self.cyclic_register_starting_byte_index.eq(cyclic_write_data_starting_byte_index)
                    m.next = "combine_tx_register_data"

                with m.Else():
                    with m.If(self.current_tx_byte_index != 0):
                        m.d.sync_100 += self.writeData_ToSerialPort.eq(self.tx_data)
                        m.d.sync_100 += self.writeEnable_ToSerialPort.eq(1)
                        m.d.sync_100 += self.current_tx_word_index.eq(self.current_tx_word_index + 1)
                        m.d.sync_100 += self.tx_packet_size.eq(self.current_tx_word_index+1)
                    m.next = "set_tx_delay_timer"

            
            with m.State("combine_tx_register_data"):
                self.bytes_used = Signal(4)
                m.d.comb += self.bytes_used.eq(self.current_tx_byte_index + self.cyclic_register_size)

                with m.If(self.bytes_used >= 4):    # 32bit word is full, sent it to the serial port
                    offset = Signal(3)
                    with m.If(self.bytes_used == 4):
                        m.d.comb += offset.eq(4)
                    with m.Else():
                        m.d.comb += offset.eq(3)

                    m.d.sync_100 += self.writeData_ToSerialPort.eq(self.tx_data | (self.internal_read_port_masked_bytes << self.current_tx_bit_index))
                    m.d.sync_100 += self.tx_data.eq(self.internal_read_port_shifted_masked_bytes >> ((offset - self.current_tx_byte_index)<<3).as_unsigned())
                
                    m.d.sync_100 += self.writeEnable_ToSerialPort.eq(1)
                    m.d.sync_100 += self.current_tx_word_index.eq(self.current_tx_word_index + 1)
                    m.d.sync_100 += self.current_tx_byte_index.eq(self.cyclic_register_size - (4 - self.current_tx_byte_index))

                with m.Else():  # word not full yet, get next register

                    m.d.sync_100 += self.tx_data.eq(self.tx_data | (self.internal_read_port_masked_bytes << self.current_tx_bit_index))
                    m.d.sync_100 += self.current_tx_byte_index.eq(self.bytes_used)

                with m.If((self.current_cyclic_register < 3) | (self.cyclic_data_enabled)):
                    m.d.sync_100 += device_register_address.eq(self.rm.devices.cyclic_config.address_offset + self.current_cyclic_register+1)
                    m.d.sync_100 += self.current_cyclic_register.eq(self.current_cyclic_register + 1)
                    m.next = "get_tx_register_config"

                with m.Else():
                    m.d.sync_100 += self.tx_packet_size.eq(self.current_tx_word_index)
                    m.next = "set_tx_delay_timer"

            with m.State("set_tx_delay_timer"):
                with m.If(self.current_device_index != 0):
                    with m.If(self.previous_rx_packet_size >= self.tx_packet_size):
                        m.d.sync_100 += self.timer.eq(self.previous_rx_packet_size-self.tx_packet_size + self.deviceRXdelay)
                    with m.Else():
                        m.d.sync_100 += self.timer.eq(self.deviceRXdelay)

                m.d.sync_100 += self.pre_timer.eq(self.word_time)
                m.next = "wait_tx_delay"

            with m.State("wait_tx_delay"):  # wait before sending TX data to device to make sure previous device has time to finish sending RX data before next device starts
                with m.If(self.timer == 0):
                    # trigger tx
                    m.d.sync_100 += self.address_ToSerialPort.eq(self.serialPort.rm.control.address_offset)
                    m.d.comb += serial_port_tx_trigger.eq(1)
                    m.d.sync_100 += self.writeData_ToSerialPort.eq(serial_port_control_data)
                    m.d.sync_100 += self.writeEnable_ToSerialPort.eq(1)
                    m.next = "wait_tx_start_read_status"
                    
                with m.Else():
                    with m.If(self.pre_timer == 0):
                        m.d.sync_100 += self.timer.eq(self.timer - 1)
                        m.d.sync_100 += self.pre_timer.eq(self.word_time)
                    with m.Else():
                        m.d.sync_100 += self.pre_timer.eq(self.pre_timer - 1)
                    m.d.sync_100 += self.writeEnable_ToSerialPort.eq(0)


            with m.State("wait_tx_start_read_status"):
                m.d.sync_100 += self.writeEnable_ToSerialPort.eq(0)
                m.d.sync_100 += self.address_ToSerialPort.eq(self.serialPort.rm.status.address_offset)
                m.d.sync_100 += self.memory_read_delay.eq(1)
                
                with m.If(self.memory_read_delay == 0):
                    m.next = "wait_tx_start"


            with m.State("wait_tx_start"):
                #m.d.sync_100 += self.writeData_ToSerialPort.eq(0)
                with m.If(serial_port_tx_busy):  # wait for tx to start before checking to see if it's done
                    m.next = "wait_tx_packet_finish"

            
            with m.State("wait_tx_packet_finish"):
                with m.If(serial_port_tx_done):  # wait for tx to finish, this also means the previous rx packet should be done

                    with m.If(self.current_device_index != 0):  # dont unpack RX packet on device 0 as there is no previous RX packet
                        
                        with m.If(serial_port_rx_done & serial_port_rx_crc_valid):  # rx done and crc valid
                            # prepare to unpack RX packet

                            m.d.sync_100 += self.current_cyclic_register.eq(0)
                            m.d.sync_100 += self.current_rx_word_index.eq(0)
                            m.d.sync_100 += self.address_ToSerialPort.eq(self.serialPort.rm.rx_data.address_offset)

                            m.d.comb += use_previous_device_bram.eq(1)
                            m.d.sync_100 += device_register_address.eq(self.rm.devices.cyclic_config.address_offset)
                            m.next = "slice_rx_register_data_wait"

                        with m.Else():
                            with m.If(serial_port_rx_done & (~serial_port_rx_crc_valid)):   # rx done but crc invalid
                                m.d.sync_100 += self.rx_invalid_crc_fault.eq(1) # packet was at least as large as expected but has bit errors

                            with m.If((~serial_port_rx_done) & serial_port_rx_busy):    # rx not done but busy
                                m.d.sync_100 += self.rx_not_finished_fault.eq(1)    # packet was smaller than expected

                            with m.If((~serial_port_rx_done) & (~serial_port_rx_busy)):    # rx not done and not busy
                                m.d.sync_100 += self.rx_no_response_fault.eq(1)   # no packet detected at all

                            # a packet error has occured, update global error bit and skip interpreting packet
                            m.d.sync_100 += self.update_error.eq(1)

                            # receive next device packet
                            with m.If(self.current_device_index != (self.max_number_of_devices-1)):
                                m.next = "start_rx"

                            with m.Else():    # update complete
                                m.d.sync_100 += self.update_busy.eq(0)
                                m.d.sync_100 += self.update_done.eq(1)
                                m.next = "idle"

                    with m.Else():
                        m.next = "start_rx"

                with m.Else():
                    m.d.sync_100 += self.writeEnable_ToSerialPort.eq(0)


            with m.State("wait_for_final_rx_packet"):
                with m.If(self.timer == 0):
                    m.d.sync_100 += self.address_ToSerialPort.eq(self.serialPort.rm.status.address_offset)
                    #m.d.sync_100 += self.writeEnable_ToSerialPort.eq(0)
                    m.d.sync_100 += self.memory_read_delay.eq(1)
                    with m.If(self.memory_read_delay):
                        m.next = "wait_tx_packet_finish"

                with m.Else():
                    with m.If(self.pre_timer == 0):
                        m.d.sync_100 += self.timer.eq(self.timer - 1)
                        m.d.sync_100 += self.pre_timer.eq(self.word_time)
                    with m.Else():
                        m.d.sync_100 += self.pre_timer.eq(self.pre_timer - 1)


            with m.State("get_rx_register_config_wait"):
                m.d.sync_100 += self.internalBramWriteEnable.eq(0)
                with m.If(self.memory_read_delay):
                    m.next = "slice_rx_register_data"
                with m.Else():
                    m.d.sync_100 += self.current_cyclic_register.eq(self.current_cyclic_register + 1)
                    m.d.comb += use_previous_device_bram.eq(1)
                    m.d.sync_100 += device_register_address.eq(self.rm.devices.cyclic_config.address_offset + self.current_cyclic_register + 1)
                    m.d.sync_100 += self.memory_read_delay.eq(1)

            
            with m.State("slice_rx_register_data_wait"):
                m.next = "slice_rx_register_data"


            with m.State("slice_rx_register_data"):
                """
                0-3: cyclic read register data size (bytes)
                4-11: cyclic read register starting byte index in 32bit word
                12-15: cyclic write register data size (bytes)
                16-23: cyclic read register starting byte index in 32bit word
                """
                with m.If((cyclic_read_data_size != 0) & ((self.current_cyclic_register < 3) | (self.cyclic_data_enabled))):
                    
                    m.d.sync_100 += self.cyclic_register_size.eq(cyclic_read_data_size)
                    m.d.sync_100 += self.cyclic_register_starting_byte_index.eq(cyclic_read_data_starting_byte_index)
                    
                    with m.If((cyclic_read_data_starting_byte_index + cyclic_read_data_size) <= 4):

                        # slice data and write to memory
                        m.d.comb += use_previous_device_bram.eq(1)
                        m.d.sync_100 += device_register_address.eq(self.rm.devices.cyclic_read_data.address_offset + self.current_cyclic_register)
                        
                        m.d.sync_100 += self.internalBramWriteData.eq((self.readData_ToSerialPort >> (cyclic_read_data_starting_byte_index<<3)) & (0xFFFFFFFF >> ((4 - cyclic_read_data_size)<<3).as_unsigned()))
                        m.d.sync_100 += self.internalBramWriteEnable.eq(1)

                        with m.If((cyclic_read_data_starting_byte_index + cyclic_read_data_size) == 4):

                            # entire word has been read, move to next
                            m.d.sync_100 += self.current_rx_word_index.eq(self.current_rx_word_index + 1)
                            m.d.sync_100 += self.address_ToSerialPort.eq(self.serialPort.rm.rx_data.address_offset + self.current_rx_word_index + 1)
                    

                    with m.Else():  # cyclic data was not entirely available from serial port memory, unpack partial data

                        # save partial data
                        m.d.sync_100 += self.rx_data.eq((self.readData_ToSerialPort >> (cyclic_read_data_starting_byte_index<<3)) & (0xFFFFFFFF >> ((4 - cyclic_read_data_size)<<3).as_unsigned()))
                        
                        # read from next serial port address
                        m.d.sync_100 += self.address_ToSerialPort.eq(self.serialPort.rm.rx_data.address_offset + self.current_rx_word_index + 1)
                        m.d.sync_100 += self.current_rx_word_index.eq(self.current_rx_word_index + 1)

                        m.next = "partial_slice_rx_register_data_wait"  # wait for next word to be available

                # invalid config means we reached an unconfigured register, packet is done
                with m.Elif(self.current_device_index < (self.max_number_of_devices-1)):
                    m.next = "start_rx"

                with m.Else():
                    m.d.sync_100 += self.update_busy.eq(0)
                    m.d.sync_100 += self.update_done.eq(1)
                    m.next = "idle"


            with m.State("partial_slice_rx_register_data_wait"):    # just a single clock delay to wait for the next word to be available
                m.next = "partial_slice_rx_register_data"


            with m.State("partial_slice_rx_register_data"):
                # finish saving partial data

                # slice/combine data and write to memory
                m.d.comb += use_previous_device_bram.eq(1)
                m.d.sync_100 += device_register_address.eq(self.rm.devices.cyclic_read_data.address_offset + self.current_cyclic_register)
                m.d.sync_100 += self.internalBramWriteData.eq(self.rx_data |
                                                        (self.readData_ToSerialPort & 
                                                         (0xFFFFFFFF >> ((4 - (self.cyclic_register_starting_byte_index + self.cyclic_register_size))<<3).as_unsigned())) <<
                                                         ((self.cyclic_register_size - self.cyclic_register_starting_byte_index)<<3).as_unsigned()
                                                        )
                m.d.sync_100 += self.internalBramWriteEnable.eq(1)
                m.next = "get_rx_register_config_wait"


            with m.State("start_unpacked_rx_packet"):
                with m.If(self.current_device_index < self.max_number_of_devices-1):
                    m.next = "start_rx"

                with m.Else():  # update complete
                    m.d.sync_100 += self.update_busy.eq(0)
                    m.d.sync_100 += self.update_done.eq(1)
                    m.next = "idle"


            with m.State("start_rx"):
                m.d.comb += serial_port_rx_trigger.eq(1)
                m.d.sync_100 += [
                    self.address_ToSerialPort.eq(self.serialPort.rm.control.address_offset),
                    self.writeData_ToSerialPort.eq(serial_port_control_data),   # trigger RX start

                    # reset signals
                    self.current_tx_word_index.eq(0),
                    self.current_tx_byte_index.eq(0),
                    self.tx_data.eq(0),
                    self.cyclic_data_enabled.eq(0),
                    self.current_cyclic_register.eq(0),
                    self.cyclic_register_size.eq(0),
                    self.cyclic_register_starting_byte_index.eq(0),
                    self.rx_no_response_fault.eq(0),
                    self.rx_not_finished_fault.eq(0),
                    self.rx_invalid_crc_fault.eq(0),

                    ]

                m.d.sync_100 += device_register_address.eq(self.rm.devices.control.address_offset)
                m.d.sync_100 += self.memory_read_delay.eq(1)
                with m.If(self.memory_read_delay):
                    m.d.sync_100 += self.writeEnable_ToSerialPort.eq(0)
                    m.next = "get_tx_device_config_wait"
                with m.Else():
                    m.d.sync_100 += self.writeEnable_ToSerialPort.eq(1)
                    m.d.sync_100 += self.current_device_index.eq(self.current_device_index + 1)



        return m
    

clock = int(100e6) # 100 Mhz
dut = em_serial_controller(64, 4, True)

regs = dut.rm

alignment = regs.devices.alignment

def dev_control(enable, enable_cyclic_data, rx_cyclic_packet_size):
    return (enable << regs.devices.control.enable.starting_bit) | (enable_cyclic_data << regs.devices.control.enable_cyclic_data.starting_bit) | (rx_cyclic_packet_size << regs.devices.control.rx_cyclic_packet_size.starting_bit)

def cyclic_config(read_size, read_start, write_size, write_start):
    return (read_size << regs.devices.cyclic_config.cyclic_read_data_size.starting_bit) | (read_start << regs.devices.cyclic_config.cyclic_read_data_starting_byte_index.starting_bit) | (write_size << regs.devices.cyclic_config.cyclic_write_data_size.starting_bit) | (write_start << regs.devices.cyclic_config.cyclic_write_data_starting_byte_index.starting_bit)

# print(cyclic_config(1, 0, 1, 0))
# print(cyclic_config(4, 1, 4, 1))

dev0 = 0
dev1 = alignment
dev2 = alignment*2

cyclic_config_offset = regs.devices.cyclic_config.address_offset
cyclic_write_offset = regs.devices.cyclic_write_data.address_offset
cyclic_read_offset = regs.devices.cyclic_read_data.address_offset

async def serialBench(ctx):

    # enable devices
    ctx.set(dut.memory.data[dev0 + regs.devices.control.address_offset], dev_control(1, 1, 4))   # device 0: enable, cyclic mode, and 4 byte packet size
    ctx.set(dut.memory.data[dev1 + regs.devices.control.address_offset], dev_control(1, 1, 4))   # device 1: enable, cyclic mode, and 4 byte packet size
    ctx.set(dut.memory.data[dev2 + regs.devices.control.address_offset], dev_control(1, 1, 4))   # device 2: enable, cyclic mode, and 4 byte packet size

    # config address and sequential registers
    ctx.set(dut.memory.data[dev0 + cyclic_config_offset], cyclic_config(1, 0, 1, 0))   # config RX/TX reg0 for 1 byte 0 offset
    ctx.set(dut.memory.data[dev0 + cyclic_config_offset+1], cyclic_config(4, 1, 4, 1))   # config RX/TX reg1 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev0 + cyclic_config_offset+2], cyclic_config(4, 1, 4, 1))   # config RX/TX reg2 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev1 + cyclic_config_offset], cyclic_config(1, 0, 1, 0))   # config RX/TX reg0 for 1 byte 0 offset
    ctx.set(dut.memory.data[dev1 + cyclic_config_offset+1], cyclic_config(4, 1, 4, 1))   # config RX/TX reg1 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev1 + cyclic_config_offset+2], cyclic_config(4, 1, 4, 1))   # config RX/TX reg2 for 4 byte 1 offse1
    ctx.set(dut.memory.data[dev2 + cyclic_config_offset], cyclic_config(1, 0, 1, 0))   # config RX/TX reg0 for 1 byte 0 offset
    ctx.set(dut.memory.data[dev2 + cyclic_config_offset+1], cyclic_config(4, 1, 4, 1))   # config RX/TX reg1 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev2 + cyclic_config_offset+2], cyclic_config(4, 1, 4, 1))   # config RX/TX reg2 for 4 byte 1 offset
    

    ctx.set(dut.memory.data[dev0 + cyclic_write_offset], 0xAB)
    ctx.set(dut.memory.data[dev0 + cyclic_write_offset+1], 0xFFFFFFFF)
    ctx.set(dut.memory.data[dev0 + cyclic_write_offset+2], 0x12345678)

    ctx.set(dut.memory.data[dev1 + cyclic_write_offset], 0xCD)
    ctx.set(dut.memory.data[dev1 + cyclic_write_offset+1], 0xFFFFFFFF)
    ctx.set(dut.memory.data[dev1 + cyclic_write_offset+2], 0x12345678)

    ctx.set(dut.memory.data[dev2 + cyclic_write_offset], 0xEF)
    ctx.set(dut.memory.data[dev2 + cyclic_write_offset+1], 0xFFFFFFFF)
    ctx.set(dut.memory.data[dev2 + cyclic_write_offset+2], 0x123456781)


    # load test response into debug serial port to simulate a device
    testTXpacket = [
    0x123456AA,
    0x70605040,
    0xB0A09080
    ]
    offset = dut.debugSerialPort.rm.tx_data.address_offset
    for e, index in enumerate(range(offset, offset+len(testTXpacket))):
        ctx.set(dut.debugSerialPort.memory.data[index], testTXpacket[e])

    await ctx.tick("sync_100")
    # set debug device bit length (baud rate)
    ctx.set(dut.debugSerialPort.bram_address, dut.debugSerialPort.rm.bit_length.address_offset)
    ctx.set(dut.debugSerialPort.bram_write_data, int(clock / 12.5e6))
    ctx.set(dut.debugSerialPort.bram_write_enable, True)
    await ctx.tick("sync_100")
    ctx.set(dut.debugSerialPort.bram_write_enable, False)

    # set main controller bit length (baud rate)
    ctx.set(dut.bram_address, dut.rm.bit_length.address_offset)
    ctx.set(dut.bram_write_data, int(clock / 12.5e6))
    ctx.set(dut.bram_write_enable, True)
    await ctx.tick("sync_100")
    ctx.set(dut.bram_write_enable, False)
    await ctx.tick("sync_100").repeat(4)

    # start transfers
    ctx.set(dut.bram_address, dut.rm.control.address_offset)
    ctx.set(dut.bram_write_data, 0b1 << dut.rm.control.start_transfers.starting_bit)     # start transfers
    ctx.set(dut.bram_write_enable, True)
    await ctx.tick("sync_100")
    ctx.set(dut.bram_write_enable, False)

    
    for i in range(3):

        # start RX on test device
        # trigger rx and set rx/tx packet sizes (RX: 3, TX: 4)
        data = (
            (0b1 << dut.debugSerialPort.rm.control.rx_start.starting_bit) |
            (0b0 << dut.debugSerialPort.rm.control.tx_start.starting_bit) |
            (3 << dut.debugSerialPort.rm.control.rx_packet_size.starting_bit) |
            (4 << dut.debugSerialPort.rm.control.tx_packet_size.starting_bit))
        ctx.set(dut.debugSerialPort.bram_address, dut.debugSerialPort.rm.control.address_offset)
        ctx.set(dut.debugSerialPort.bram_write_data, data)     
        ctx.set(dut.debugSerialPort.bram_write_enable, True)
        await ctx.tick("sync_100")
        ctx.set(dut.debugSerialPort.bram_write_enable, False)


        # wait for test device to begin receiving packet
        x=0
        while(not ctx.get(dut.debugSerialPort.rxBusy)):
            x+=1
            await ctx.tick("sync_100")
            if(x>1500):
                print("test device did not start receiving packet within timeout")
                return
        print("test device started receiving packet")

        # wait for test device to finish receiving packet
        x=0
        while(ctx.get(dut.debugSerialPort.rxBusy)):
            x+=1
            await ctx.tick("sync_100")
            if(x>1500):
                print("test device did not finish receiving packet within timeout")
                return
        print("test device finished receiving packet")

        #assert ctx.get(dut.debugSerialPort.rxCRCvalid)
        if not ctx.get(dut.debugSerialPort.rxCRCvalid):
            print("CRC invalid")

        await ctx.tick("sync_100").repeat(200)

        # start TX on test device
        # trigger tx and set rx/tx packet sizes (RX: 3, TX: 4)
        data = (
            (0b0 << dut.debugSerialPort.rm.control.rx_start.starting_bit) |
            (0b1 << dut.debugSerialPort.rm.control.tx_start.starting_bit) |
            (3 << dut.debugSerialPort.rm.control.rx_packet_size.starting_bit) |
            (4 << dut.debugSerialPort.rm.control.tx_packet_size.starting_bit))
        
        ctx.set(dut.debugSerialPort.bram_address, dut.debugSerialPort.rm.control.address_offset)
        ctx.set(dut.debugSerialPort.bram_write_data, data)
        ctx.set(dut.debugSerialPort.bram_write_enable, True)
        await ctx.tick("sync_100")
        ctx.set(dut.debugSerialPort.bram_write_enable, False)

        await ctx.tick("sync_100").repeat(10)

    await ctx.tick("sync_100").repeat(7000)

if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock, domain="sync_100")
    sim.add_testbench(serialBench)
    with sim.write_vcd("serial_controller.vcd"):
        sim.run()
