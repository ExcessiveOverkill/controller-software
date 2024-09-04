from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from enum import IntEnum, auto
import numpy as np
from amaranth.back import verilog
import math
from src.drive_serial_port import drive_serial_port


class serial_controller(Elaboratable):
    """
    Specific serial interface for managing communication with our own devices at high speeds
    """

    """
    Registers (32 bit):

    0x00: Control
        0: Start update start sequential write/read from all configured devices and update internal memory
        1-31: reserved


    0x01: Bit length
        0-31: Bit length in clock cycles (minimum is equal to 115200 baud)    DO NOT MODIFY WHILE UPDATE IS IN PROGRESS

    0x02: Status
        0: Update busy
        1: Update done
        2: Update error     one or more devices did not correctly respond
        3-31: reserved

    0x03-0x0F: reserved

    PER-DEVICE REGISTERS: DO NOT MODIFY WHILE UPDATE IS IN PROGRESS
    base address is 0x10 + device_number*(total_device_registers)

    0x00: Control
        0: enable
        1: enable cyclic data
        2-31: reserved

    0x01: Status
        0: no rx response fault    (no start of rx packet detected)
        1: rx not finished fault   (rx packet was not long enough) 
        2: invalid rx CRC fault    (CRC did not match)
        3-31: reserved

    0x02: RX cyclic packet size
        0-7:   expected cyclic RX packet size in 32bit words not including CRC.
        9-31: reserved

    0x03-0x04: reserved

    note: the first 3 cyclic registers are always enabled and must be configured/used for device address and sequential transfers
    : Cyclic config
        0-3: cyclic read register data size (bytes)
        4-11: cyclic read register starting byte index in 32bit word
        12-15: cyclic write register data size (bytes)
        16-23: cyclic read register starting byte index in 32bit word
        24-31: reserved

    : Read cyclic data
        0-31: cyclic register data

    : Write cyclic data
        0-31: cyclic register data

    """

    def __init__(self, clock:int, max_cyclic_registers:int, max_number_of_devices:int, debug:bool = False) -> None:
        """
        clock: input clock frequency (Hz)

        max_cyclic_registers: maximum number of cyclic rx/tx registers per device (up to 32 bit each) (255 absolute max)

        max_number_of_devices: maximum number of different devices allowed to be connected to a single port (address limit is 255, but smaller values will consume less FPGA memory)
        """
        self.debug = debug
        self.registerTable = {
             "control":{
                 "address":0x00,
                 "start_transfers_offset":0
             },
             "bit_length":{
                 "address":0x01,
                 "bit_length_offset":0
             },
             "status":{
                 "address":0x02,
                 "update_busy_offset":0,
                 "update_done_offset":1,
                 "update_error_offset":2
             },
             "devices":{}
        }

        self.deviceRXdelay = 2  # how long in word transfers in addition to wait for RX packets to finish
        
        self.clock = clock
        self.maxCyclicRegisters = max_cyclic_registers+3    # cyclic + address + sequential control + sequential data
        self.maxNumDevices = max_number_of_devices

        self.deviceRegisterCount = 2 + max_cyclic_registers*3  # control, status, cyclic config, cyclic read, cyclic write

        self.maxSerialPacket = self.maxCyclicRegisters
        self.serialPort = drive_serial_port(self.clock, self.maxSerialPacket)

        if(self.debug):
            self.debugSerialPort = drive_serial_port(self.clock, self.maxSerialPacket)

        self.totalRequiredMemory = 0xF + self.deviceRegisterCount * self.maxNumDevices
        
        self.devicesBaseAddr = 0x10

        self.deviceControlAddrOffset = 0x00
        self.deviceStatusAddrOffset = 0x01
        self.deviceRXsizeAddrOffset = 0x02
        self.deviceCyclicConfigAddrOffset = 0x03
        self.deviceCyclicReadDataAddrOffset = None
        self.deviceCyclicWriteDataAddrOffset = None

        for device_number in range(self.maxNumDevices):
            baseAddr = self.devicesBaseAddr + self.deviceRegisterCount*device_number
            deviceRegisters = {
                "control":{
                 "address":0x00+baseAddr,
                 "enable_offset":0,
                 "enable_cyclic_data_offset":1
             },
                "status":{
                 "address":0x01+baseAddr,
                 "no_rx_response_fault_offset":0,
                 "rx_not_finished_fault_offset":0,
                 "invalid_rx_crc_fault_offset":0,
             },
                "rx_packet_size":{
                 "address":0x02+baseAddr,
                 "size_offset":0,
             }
             }
            
            baseAddr += self.deviceCyclicConfigAddrOffset

            
            for cyclic_index in range(self.maxCyclicRegisters):
                deviceRegisters[f"cyclic_config_{cyclic_index}"] = {
                    "address":baseAddr+cyclic_index,
                    "cyclic_read_data_size_offset":0,
                    "cyclic_read_data_starting_byte_index_offset":4,
                    "cyclic_write_data_size_offset":12,
                    "cyclic_write_data_starting_byte_index_offset":16
                }
            baseAddr += self.maxCyclicRegisters
            if(self.deviceCyclicReadDataAddrOffset is None):
                self.deviceCyclicReadDataAddrOffset = baseAddr - self.devicesBaseAddr

            for cyclic_index in range(self.maxCyclicRegisters):
                deviceRegisters[f"cyclic_read_data_{cyclic_index}"] = {
                    "address":baseAddr+cyclic_index,
                    "cyclic_read_data_offset":0
                }
            baseAddr += self.maxCyclicRegisters
            if(self.deviceCyclicWriteDataAddrOffset is None):
                self.deviceCyclicWriteDataAddrOffset = baseAddr - self.devicesBaseAddr

            for cyclic_index in range(self.maxCyclicRegisters):
                deviceRegisters[f"cyclic_write_data_{cyclic_index}"] = {
                    "address":baseAddr+cyclic_index,
                    "cyclic_write_data_offset":0
                }
            baseAddr += self.maxCyclicRegisters

            self.registerTable["devices"][device_number] = deviceRegisters

        #print(self.registerTable)


        # Ports
        self.address = Signal(16)
        self.writeData = Signal(32)
        self.readData = Signal(32)
        self.writeEnable = Signal()

        

    class states(IntEnum):
        IDLE = 0
        UPDATE_STATUS_START_WAIT = auto()
        GET_TX_DEVICE_CONFIG_WAIT = auto()
        GET_TX_DEVICE_CONFIG = auto()
        START_UNPACK_RX_PACKET_WAIT = auto()
        GET_RX_DEVICE_PACKET_SIZE = auto()
        GET_TX_REGISTER_CONFIG = auto()
        GET_TX_REGISTER_DATA = auto()
        COMBINE_TX_REGISTER_DATA = auto()
        WRITE_TX_PACKET_SIZE = auto() # 9   # write required packet size to serial port
        WAIT_TX_DELAY = auto()         # wait before sending TX data to device to make sure previous device has time to finish sending RX data before next device starts
        WAIT_TX_START = auto()
        WAIT_TX_START_READ_STATUS = auto()
        WAIT_FOR_FINAL_RX_PACKET_WAIT = auto()
        WAIT_TX_PACKET_FINISH = auto()  #14  # wait for tx packet to finish sending before 
        WAIT_FOR_FINAL_RX_PACKET = auto()   #15
        START_UNPACK_RX_PACKET = auto()
        GET_RX_REGISTER_CONFIG = auto() #17
        GET_RX_REGISTER_CONFIG_WAIT = auto()
        SLICE_RX_REGISTER_DATA = auto()
        PARTIAL_SLICE_RX_REGISTER_DATA_WAIT = auto()
        PARTIAL_SLICE_RX_REGISTER_DATA = auto()
        START_RX = auto()   #22
        WAIT_RX_PACKET_START = auto()

    def elaborate(self, platform):
        m = Module()

        m.submodules.drive_serial_port = self.serialPort

        if(self.debug):
            m.submodules.debug_drive_serial_port = self.debugSerialPort
            m.d.comb += self.debugSerialPort.rx.eq(self.serialPort.tx)
            m.d.comb += self.serialPort.rx.eq(self.debugSerialPort.tx)

        self.address_ToSerialPort = self.serialPort.address
        self.writeData_ToSerialPort = self.serialPort.writeData
        self.readData_ToSerialPort = self.serialPort.readData
        self.writeEnable_ToSerialPort = self.serialPort.writeEnable

        m.submodules.memory = self.memory = Memory(shape=unsigned(32), depth=self.totalRequiredMemory, init=[])     # memory for all registers

        self.externalReadPort = self.memory.read_port()
        self.externalWritePort = self.memory.write_port()
        self.internalReadPort = self.memory.read_port()
        self.internalWritePort = self.memory.write_port()

        # connect external memory interfaces
        m.d.comb += self.externalReadPort.addr.eq(self.address)
        m.d.comb += self.externalWritePort.addr.eq(self.address)
        m.d.comb += self.externalWritePort.data.eq(self.writeData)
        m.d.comb += self.externalWritePort.en.eq(self.writeEnable)
        m.d.comb += self.readData.eq(self.externalReadPort.data)

        self.internalReadWriteAddress = Signal(16)
        m.d.comb += self.internalReadPort.addr.eq(self.internalReadWriteAddress)
        m.d.comb += self.internalWritePort.addr.eq(self.internalReadWriteAddress)

        # status signals
        self.updateBusy = Signal()
        self.updateDone = Signal()
        self.updateError = Signal()

        # config
        self.wordTime = Signal(range(int(clock/115200 * 10 * 4) + 1))

        # per-update signals    (reset after each update cycle)
        self.currentDeviceIndex = Signal(range(self.maxNumDevices + 1))
        self.state = Signal(Shape.cast(self.states), reset=self.states.IDLE)
        self.timer = Signal(range(self.maxSerialPacket+1))    # long enough to count up to the biggest expected packet 
        self.preTimer = Signal(range(int(clock/115200 * 10 * 4) + 1))     # long enough to count up to 1 word

        # per-device signals    (reset after each device update)
        self.currentTxWord = Signal(range(self.maxSerialPacket))
        self.currentRxWord = Signal(range(self.maxSerialPacket))
        self.currentTxByte = Signal(range(8))
        self.currentRxByte = Signal(range(8))
        self.txData = Signal(32)
        self.rxData = Signal(32)
        self.cylicDataEnabled = Signal()
        self.currentCyclicRegister = Signal(range(self.maxSerialPacket))    # current register index
        self.cyclicRegisterSize = Signal(range(5))  # current register size in bytes
        self.cyclicRegisterStartingByte = Signal(range(4))  # current register starting byte address in the 32bit word
        self.rxResponseFault = Signal()
        self.rxNotFinishedFault = Signal()
        self.rxInvalidCRCFault = Signal()
        self.txPacketSize = Signal(range(self.maxSerialPacket))
        self.rxPacketSize = Signal(range(self.maxSerialPacket))
        self.previousRxPacketSize = Signal(range(self.maxSerialPacket+1))
        
        

        with m.If((self.writeEnable) & (self.address == 0x0)):    # main control register was written to
            with m.If(self.writeData.bit_select(0, 1)):     # start update bit was written to
                m.d.sync += self.updateBusy.eq(1)
                m.d.sync += self.updateDone.eq(0)
                m.d.sync += self.updateError.eq(0)
                m.d.sync += self.txPacketSize.eq(0)
                m.d.sync += self.rxPacketSize.eq(3)
                m.d.sync += self.previousRxPacketSize.eq(3)
                m.d.sync += self.currentDeviceIndex.eq(0)
                m.d.sync += self.currentTxByte.eq(0)
                m.d.sync += self.timer.eq(0)
                m.d.sync += self.preTimer.eq(0)
                m.d.sync += self.currentRxWord.eq(0)
                m.d.sync += self.currentRxByte.eq(0)
                m.d.sync += self.state.eq(self.states.UPDATE_STATUS_START_WAIT)

                # update status register
                # write
                m.d.sync += self.internalReadWriteAddress.eq(0x02)
                m.d.sync += self.internalWritePort.data.eq(self.updateBusy | self.updateDone.shift_left(1) | self.updateError.shift_left(2))
                m.d.sync += self.internalWritePort.en.eq(1)

                
        self.bitTime = Signal(32)
        self.updateWordTime = Signal()

        with m.If((self.writeEnable) & (self.address == 0x1) & (self.updateBusy == 0)):    # bit length register was written to while not busy
            # pass bit length to serial port
            m.d.sync += self.address_ToSerialPort.eq(0x01)
            m.d.sync += self.writeData_ToSerialPort.eq(self.writeData)
            m.d.sync += self.writeEnable_ToSerialPort.eq(1)

            m.d.sync += self.bitTime.eq(self.writeData)
            m.d.sync += self.updateWordTime.eq(1)

        with m.If(self.updateWordTime):
            # set word time in clock cycles for later use
            m.d.sync += self.wordTime.eq(self.bitTime * (10*4))
            m.d.sync += self.updateWordTime.eq(0)



        with m.If(self.state == self.states.IDLE):
            m.d.sync += self.txData.eq(0)
            m.d.sync += self.currentCyclicRegister.eq(0)
            m.d.sync += self.rxInvalidCRCFault.eq(0)
            m.d.sync += self.rxNotFinishedFault.eq(0)
            m.d.sync += self.rxResponseFault.eq(0)
            m.d.sync += self.currentTxWord.eq(0)
            m.d.sync += self.internalWritePort.en.eq(0)


        with m.If(self.state == self.states.UPDATE_STATUS_START_WAIT):
            m.d.sync += self.writeEnable_ToSerialPort.eq(0)
            m.d.sync += self.internalWritePort.en.eq(0)
            # read from device config register
            m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + self.currentDeviceIndex*self.deviceRegisterCount + self.deviceControlAddrOffset)
            m.d.sync += self.state.eq(self.states.GET_TX_DEVICE_CONFIG_WAIT)


        with m.If(self.state == self.states.GET_TX_DEVICE_CONFIG_WAIT):
            m.d.sync += self.writeEnable_ToSerialPort.eq(0)
            m.d.sync += self.state.eq(self.states.GET_TX_DEVICE_CONFIG)


        with m.If(self.state == self.states.GET_TX_DEVICE_CONFIG):
            with m.If(self.internalReadPort.data.bit_select(0, 1)):     # check if device is enabled
                m.d.sync += self.cylicDataEnabled.eq(self.internalReadPort.data.bit_select(1, 1))

                # read from rx packet size register
                m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + self.currentDeviceIndex*self.deviceRegisterCount + self.deviceRXsizeAddrOffset)

                m.d.sync += self.state.eq(self.states.START_UNPACK_RX_PACKET_WAIT)

            with m.Elif(self.currentDeviceIndex < self.maxNumDevices-1):    # skip next device if the current one is disabled and we still have devices left to try
                m.d.sync += self.currentDeviceIndex.eq(self.currentDeviceIndex+1)
                m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + (self.currentDeviceIndex+1)*self.deviceRegisterCount + self.deviceControlAddrOffset)
                m.d.sync += self.state.eq(self.states.GET_TX_DEVICE_CONFIG_WAIT)

            with m.Else():  # no more devices to send packets for, receive the last RX packet
                m.d.sync += self.timer.eq(self.rxPacketSize+self.deviceRXdelay)
                m.d.sync += self.preTimer.eq(self.wordTime)
                m.d.sync += self.state.eq(self.states.WAIT_FOR_FINAL_RX_PACKET)


        with m.If(self.state == self.states.START_UNPACK_RX_PACKET_WAIT):
            m.d.sync += self.state.eq(self.states.GET_RX_DEVICE_PACKET_SIZE)


        with m.If(self.state == self.states.GET_RX_DEVICE_PACKET_SIZE):
            
            with m.If(self.cylicDataEnabled):   # only use the rx size register if cyclic mode is enabled, otherwise the side should always be 3
                m.d.sync += self.rxPacketSize.eq(self.internalReadPort.data.bit_select(0, 8))
            with m.Else():
                m.d.sync += self.rxPacketSize.eq(3)
            
            m.d.sync += self.previousRxPacketSize.eq(self.rxPacketSize)
            
            # read from cylic config register
            m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + self.currentDeviceIndex*self.deviceRegisterCount + self.currentCyclicRegister + self.deviceCyclicConfigAddrOffset)

            m.d.sync += self.state.eq(self.states.GET_TX_REGISTER_CONFIG)


        with m.If(self.state == self.states.GET_TX_REGISTER_CONFIG):
            m.d.sync += self.writeEnable_ToSerialPort.eq(0)
            m.d.sync += self.address_ToSerialPort.eq(self.serialPort.registers.txDataBaseAddr + self.currentTxWord)

            # read from cylic data register
            m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + self.currentDeviceIndex*self.deviceRegisterCount + self.currentCyclicRegister + self.deviceCyclicWriteDataAddrOffset)

            m.d.sync += self.state.eq(self.states.GET_TX_REGISTER_DATA)
            

        with m.If(self.state == self.states.GET_TX_REGISTER_DATA):

            with m.If((self.internalReadPort.data.bit_select(12, 3) != 0) & ((self.currentCyclicRegister < 3) | (self.cylicDataEnabled))):
                # save config data
                m.d.sync += self.cyclicRegisterSize.eq(self.internalReadPort.data.bit_select(12, 3))
                m.d.sync += self.cyclicRegisterStartingByte.eq(self.internalReadPort.data.bit_select(16, 3))

                m.d.sync += self.state.eq(self.states.COMBINE_TX_REGISTER_DATA)

            with m.Else():  # invalid config means we reached an unconfigured register, packet is done
                # send to serial port if there is partial data left
                with m.If(self.currentTxByte != 0):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData)
                    m.d.sync += self.writeEnable_ToSerialPort.eq(1)
                    m.d.sync += self.currentTxWord.eq(self.currentTxWord+1)
                m.d.sync += self.state.eq(self.states.WRITE_TX_PACKET_SIZE)
        

        with m.If(self.state == self.states.COMBINE_TX_REGISTER_DATA):
            
            with m.If(self.currentTxByte + self.cyclicRegisterSize > 4):    # 32bit word is full, sent it to the serial port
                
                with m.If(self.cyclicRegisterSize == 1):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 8)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 8)>>(abs(3-self.currentTxByte)*8))
                with m.Elif(self.cyclicRegisterSize == 2):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 16)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 16)>>(abs(3-self.currentTxByte)*8))
                with m.Elif(self.cyclicRegisterSize == 3):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 24)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 24)>>(abs(3-self.currentTxByte)*8))
                with m.Elif(self.cyclicRegisterSize == 4):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 32)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 32)>>(abs(3-self.currentTxByte)*8))
                
                m.d.sync += self.currentTxWord.eq(self.currentTxWord + 1)
                m.d.sync += self.writeEnable_ToSerialPort.eq(1)
                m.d.sync += self.currentTxByte.eq(self.cyclicRegisterSize - (4-self.currentTxByte))
            
            with m.Elif(self.currentTxByte + self.cyclicRegisterSize == 4):    # 32bit word is full, sent it to the serial port
                
                with m.If(self.cyclicRegisterSize == 1):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 8)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 8)>>(abs(4-self.currentTxByte)*8))
                with m.Elif(self.cyclicRegisterSize == 2):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 16)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 16)>>(abs(4-self.currentTxByte)*8))
                with m.Elif(self.cyclicRegisterSize == 3):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 24)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 24)>>(abs(4-self.currentTxByte)*8))
                with m.Elif(self.cyclicRegisterSize == 4):
                    m.d.sync += self.writeData_ToSerialPort.eq(self.txData | self.internalReadPort.data.bit_select(0, 32)<<(self.currentTxByte*8))
                    m.d.sync += self.txData.eq(self.internalReadPort.data.bit_select(self.cyclicRegisterStartingByte*8, 32)>>(abs(4-self.currentTxByte)*8))
                
                m.d.sync += self.currentTxWord.eq(self.currentTxWord + 1)
                m.d.sync += self.writeEnable_ToSerialPort.eq(1)
                m.d.sync += self.currentTxByte.eq(self.cyclicRegisterSize - (4-self.currentTxByte))

            
            with m.Else():  # word not full yet, get next register

                with m.If(self.cyclicRegisterSize == 1):
                    m.d.sync += self.txData.eq(self.txData | self.internalReadPort.data.bit_select(0, 8)<<(self.currentTxByte*8))
                with m.Elif(self.cyclicRegisterSize == 2):
                    m.d.sync += self.txData.eq(self.txData | self.internalReadPort.data.bit_select(0, 16)<<(self.currentTxByte*8))
                with m.Elif(self.cyclicRegisterSize == 3):
                    m.d.sync += self.txData.eq(self.txData | self.internalReadPort.data.bit_select(0, 24)<<(self.currentTxByte*8))
                with m.Elif(self.cyclicRegisterSize == 4):
                    m.d.sync += self.txData.eq(self.txData | self.internalReadPort.data.bit_select(0, 32)<<(self.currentTxByte*8))

                m.d.sync += self.currentTxByte.eq(self.currentTxByte + self.cyclicRegisterSize)
                
            with m.If((self.currentCyclicRegister < 3) | (self.cylicDataEnabled)):  # get next register
                # read from cylic config register
                m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + self.currentDeviceIndex*self.deviceRegisterCount + self.currentCyclicRegister+1 + self.deviceCyclicConfigAddrOffset)
                m.d.sync += self.currentCyclicRegister.eq(self.currentCyclicRegister+1)
                m.d.sync += self.state.eq(self.states.GET_TX_REGISTER_CONFIG)

            with m.Else():
                m.d.sync += self.state.eq(self.states.WRITE_TX_PACKET_SIZE)


        with m.If(self.state == self.states.WRITE_TX_PACKET_SIZE):
            m.d.sync += self.address_ToSerialPort.eq(0x00)
            m.d.sync += self.writeData_ToSerialPort.eq(self.currentTxWord.shift_left(16))
            m.d.sync += self.writeEnable_ToSerialPort.eq(1)

            with m.If(self.currentDeviceIndex != 0):
                with m.If(self.previousRxPacketSize >= self.currentTxWord):
                    m.d.sync += self.timer.eq(self.previousRxPacketSize-self.currentTxWord + self.deviceRXdelay)
                with m.Else():
                    m.d.sync += self.timer.eq(self.deviceRXdelay)

                m.d.sync += self.preTimer.eq(self.wordTime)

            m.d.sync += self.state.eq(self.states.WAIT_TX_DELAY)
            

        with m.If(self.state == self.states.WAIT_TX_DELAY):
            with m.If(self.timer == 0):
                # write
                m.d.sync += self.address_ToSerialPort.eq(0x00)
                m.d.sync += self.writeData_ToSerialPort.eq(0b1)     # trigger TX start
                m.d.sync += self.writeEnable_ToSerialPort.eq(1)
                m.d.sync += self.state.eq(self.states.WAIT_TX_START_READ_STATUS)
                
            with m.Else():
                with m.If(self.preTimer == 0):
                    m.d.sync += self.timer.eq(self.timer - 1)
                    m.d.sync += self.preTimer.eq(self.wordTime)
                with m.Else():
                    m.d.sync += self.preTimer.eq(self.preTimer - 1)
                m.d.sync += self.writeEnable_ToSerialPort.eq(0)


        with m.If(self.state == self.states.WAIT_TX_START_READ_STATUS):
            m.d.sync += self.writeEnable_ToSerialPort.eq(0)
            # read
            m.d.sync += self.address_ToSerialPort.eq(0x02)  # serial port status register
            with m.If(self.address_ToSerialPort == 0x02):
                m.d.sync += self.state.eq(self.states.WAIT_TX_START)


        with m.If(self.state == self.states.WAIT_TX_START):
            m.d.sync += self.writeEnable_ToSerialPort.eq(0)
            with m.If(self.readData_ToSerialPort.bit_select(1, 1)):     # wait for TX transfer to start before checking to see if it has ended
                m.d.sync += self.state.eq(self.states.WAIT_TX_PACKET_FINISH)


        with m.If(self.state == self.states.WAIT_TX_PACKET_FINISH):
            with m.If(self.readData_ToSerialPort.bit_select(0, 1)):     # TX send is done, this means the previous RX packet should also be done

                with m.If(self.currentDeviceIndex != 0):  # dont unpack RX packet on device 0 index as there is no previous RX packet yet

                    with m.If(self.readData_ToSerialPort.bit_select(2, 1) & self.readData_ToSerialPort.bit_select(4, 1)):   # RX done and CRC valid
                        # prepare to unpack RX packet
                        
                        m.d.sync += self.currentCyclicRegister.eq(0)

                        # read from serial port rx register
                        m.d.sync += self.address_ToSerialPort.eq(self.serialPort.registers.rxDataBaseAddr + 0)

                        m.d.sync += self.currentRxWord.eq(0)

                        # read from cylic config register
                        # read
                        m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + (self.currentDeviceIndex-1)*self.deviceRegisterCount + self.deviceCyclicConfigAddrOffset)
                        m.d.sync += self.state.eq(self.states.GET_RX_REGISTER_CONFIG)

                    with m.Else():
                        with m.If((self.readData_ToSerialPort.bit_select(4, 1) == 0) & self.readData_ToSerialPort.bit_select(3, 1)):   # done but invalid CRC
                            m.d.sync += self.rxInvalidCRCFault.eq(1)    # packet was at least as large as expected but has bit error(s)

                        with m.If(self.readData_ToSerialPort.bit_select(3, 1) & (self.readData_ToSerialPort.bit_select(2, 1) == 0)):   # not done but busy
                            m.d.sync += self.rxNotFinishedFault.eq(1)   # packet was smaller than expected

                        with m.If((self.readData_ToSerialPort.bit_select(2, 1) == 0) & (self.readData_ToSerialPort.bit_select(3, 1) == 0)):   # not done and not busy
                            m.d.sync += self.rxResponseFault.eq(1)      # no packet was detected at all

                        # a packet error has occured, update global error bit and skip interpreting packet
                        m.d.sync += self.updateError.eq(1)

                        # receive next device packet
                        with m.If(self.currentDeviceIndex < self.maxNumDevices-1):
                            m.d.sync += self.state.eq(self.states.START_RX)
                        
                        with m.Else():  # update complete

                            m.d.sync += self.updateBusy.eq(0)
                            m.d.sync += self.updateDone.eq(1)

                            # update status register
                            m.d.sync += self.internalReadWriteAddress.eq(0x02)
                            m.d.sync += self.internalWritePort.data.eq(0b10 | self.updateError.shift_left(2))
                            m.d.sync += self.internalWritePort.en.eq(1)

                            m.d.sync += self.state.eq(self.states.IDLE)

                with m.Else():
                    m.d.sync += self.state.eq(self.states.START_RX)

            with m.Else():
                m.d.sync += self.writeEnable_ToSerialPort.eq(0)


        with m.If(self.state == self.states.WAIT_FOR_FINAL_RX_PACKET):
            with m.If(self.timer == 0):
                m.d.sync += self.state.eq(self.states.WAIT_FOR_FINAL_RX_PACKET_WAIT)

            with m.Else():
                with m.If(self.preTimer == 0):
                    m.d.sync += self.timer.eq(self.timer - 1)
                    m.d.sync += self.preTimer.eq(self.wordTime)
                with m.Else():
                    m.d.sync += self.preTimer.eq(self.preTimer - 1)

        
        with m.If(self.state == self.states.WAIT_FOR_FINAL_RX_PACKET_WAIT):
            m.d.sync += self.writeEnable_ToSerialPort.eq(0)
            # read
            m.d.sync += self.address_ToSerialPort.eq(0x02)  # serial port status register
            with m.If(self.address_ToSerialPort == 0x02):
                m.d.sync += self.state.eq(self.states.WAIT_TX_PACKET_FINISH)


        with m.If(self.state == self.states.GET_RX_REGISTER_CONFIG_WAIT):
            m.d.sync += self.internalWritePort.en.eq(0)

            # move to next cyclic register
            m.d.sync += self.currentCyclicRegister.eq(self.currentCyclicRegister + 1)
            # read address
            m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + (self.currentDeviceIndex-1)*self.deviceRegisterCount + self.deviceCyclicConfigAddrOffset + self.currentCyclicRegister + 1)
            m.d.sync += self.state.eq(self.states.GET_RX_REGISTER_CONFIG)


        with m.If(self.state == self.states.GET_RX_REGISTER_CONFIG):
            m.d.sync += self.state.eq(self.states.SLICE_RX_REGISTER_DATA)
            

        with m.If(self.state == self.states.SLICE_RX_REGISTER_DATA):

            with m.If((self.internalReadPort.data.bit_select(0, 3) != 0) & ((self.currentCyclicRegister < 3) | (self.cylicDataEnabled))):
                # save config data
                m.d.sync += self.cyclicRegisterSize.eq(self.internalReadPort.data.bit_select(0, 3))
                m.d.sync += self.cyclicRegisterStartingByte.eq(self.internalReadPort.data.bit_select(4, 3))

                with m.If(self.internalReadPort.data.bit_select(4, 3) + self.internalReadPort.data.bit_select(0, 3) <= 4):   # entire cyclic register value is available from memory
                    
                    # slice data and write to memory
                    
                    m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + (self.currentDeviceIndex-1)*self.deviceRegisterCount + self.deviceCyclicReadDataAddrOffset + self.currentCyclicRegister)
                    # write address
                    m.d.sync += self.internalWritePort.data.eq((self.readData_ToSerialPort >> (self.internalReadPort.data.bit_select(4, 3)*8)) & (0xFFFFFFFF >> (abs(4 - self.internalReadPort.data.bit_select(0, 3)) * 8)))
                    m.d.sync += self.internalWritePort.en.eq(1)

                    m.d.sync += self.state.eq(self.states.GET_RX_REGISTER_CONFIG_WAIT)


                    with m.If(self.internalReadPort.data.bit_select(4, 3) + self.internalReadPort.data.bit_select(0, 3) == 4):

                        # entire register was read, increment to next
                        m.d.sync += self.address_ToSerialPort.eq(self.serialPort.registers.rxDataBaseAddr + self.currentRxWord + 1)
                        m.d.sync += self.currentRxWord.eq(self.currentRxWord + 1)

                
                with m.Else():  # cyclic data was not entirely available from serial port memory, unpack partial data

                    # save partial data
                    m.d.sync += self.rxData.eq((self.readData_ToSerialPort >> (self.internalReadPort.data.bit_select(4, 3)*8)) & (0xFFFFFFFF >> (abs(4 - self.internalReadPort.data.bit_select(0, 3)) * 8)))

                    # read from next serial port rx register
                    m.d.sync += self.address_ToSerialPort.eq(self.serialPort.registers.rxDataBaseAddr + self.currentRxWord + 1)
                    m.d.sync += self.currentRxWord.eq(self.currentRxWord + 1)

                    # wait a clock cycle for data to become available
                    m.d.sync += self.state.eq(self.states.PARTIAL_SLICE_RX_REGISTER_DATA_WAIT)

            # invalid config means we reached an unconfigured register, packet is done
            with m.Elif(self.currentDeviceIndex < self.maxNumDevices-1):
                m.d.sync += self.state.eq(self.states.START_RX)

            with m.Else():
                m.d.sync += self.updateBusy.eq(0)
                m.d.sync += self.updateDone.eq(1)

                # update status register
                m.d.sync += self.internalReadWriteAddress.eq(0x02)
                m.d.sync += self.internalWritePort.data.eq(0b10 | self.updateError.shift_left(2))
                m.d.sync += self.internalWritePort.en.eq(1)

                m.d.sync += self.state.eq(self.states.IDLE)


        with m.If(self.state == self.states.PARTIAL_SLICE_RX_REGISTER_DATA_WAIT):
            m.d.sync += self.state.eq(self.states.PARTIAL_SLICE_RX_REGISTER_DATA)


        with m.If(self.state == self.states.PARTIAL_SLICE_RX_REGISTER_DATA):
            
            # finish saving partial data

            # slice/combine data and write to memory
            # write
            m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + (self.currentDeviceIndex-1)*self.deviceRegisterCount + self.deviceCyclicReadDataAddrOffset + self.currentCyclicRegister)
            m.d.sync += self.internalWritePort.data.eq(self.rxData |
                                                        (self.readData_ToSerialPort & 
                                                         (0xFFFFFFFF >> (abs(4 - (self.cyclicRegisterStartingByte + self.cyclicRegisterSize)) * 8))) <<
                                                         (abs(self.cyclicRegisterSize - self.cyclicRegisterStartingByte) * 8)
                                                        )
            m.d.sync += self.internalWritePort.en.eq(1)

            m.d.sync += self.state.eq(self.states.GET_RX_REGISTER_CONFIG_WAIT)


        with m.If(self.state == self.states.START_UNPACK_RX_PACKET):
            
            with m.If(self.currentDeviceIndex != 0):
                # update device status bits
                m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + (self.currentDeviceIndex-1)*self.deviceRegisterCount + self.deviceStatusAddrOffset)
                m.d.sync += self.internalWritePort.data.eq(self.rxResponseFault | self.rxNotFinishedFault.shift_left(1) | self.rxInvalidCRCFault.shift_left(2))
                m.d.sync += self.internalWritePort.en.eq(1)
            
            with m.If(self.currentDeviceIndex < self.maxNumDevices-1):
                m.d.sync += self.state.eq(self.states.START_RX)
            
            with m.Else():  # update complete

                m.d.sync += self.updateBusy.eq(0)
                m.d.sync += self.updateDone.eq(1)

                # update status register
                m.d.sync += self.internalReadWriteAddress.eq(0x02)
                m.d.sync += self.internalWritePort.data.eq(self.updateBusy | self.updateDone.shift_left(1) | self.updateError.shift_left(2))
                m.d.sync += self.internalWritePort.en.eq(1)

                m.d.sync += self.state.eq(self.states.IDLE)


        with m.If(self.state == self.states.START_RX):

            m.d.sync += self.address_ToSerialPort.eq(0x00)
            m.d.sync += self.writeData_ToSerialPort.eq(0b10 | self.rxPacketSize.shift_left(24))     # trigger RX start and set rx size
            m.d.sync += self.writeEnable_ToSerialPort.eq(1)

            # reset device signals
            m.d.sync += self.currentTxWord.eq(0)
            m.d.sync += self.currentTxByte.eq(0)
            m.d.sync += self.txData.eq(0)
            m.d.sync += self.cylicDataEnabled.eq(0)
            m.d.sync += self.currentCyclicRegister.eq(0)
            m.d.sync += self.cyclicRegisterSize.eq(0)
            m.d.sync += self.cyclicRegisterStartingByte.eq(0)
            m.d.sync += self.rxResponseFault.eq(0)
            m.d.sync += self.rxNotFinishedFault.eq(0)
            m.d.sync += self.rxInvalidCRCFault.eq(0)

            m.d.sync += self.currentDeviceIndex.eq(self.currentDeviceIndex+1)   # increment to next device
            
            m.d.sync += self.state.eq(self.states.GET_TX_DEVICE_CONFIG_WAIT)
            # read from device config register
            m.d.sync += self.internalReadWriteAddress.eq(self.devicesBaseAddr + (self.currentDeviceIndex+1)*self.deviceRegisterCount + self.deviceControlAddrOffset)

            
        

        return m

clock = int(100e6) # 100 Mhz
dut = serial_controller(clock, 64, 4, True)

regs = dut.registerTable
dev0 = regs["devices"][0]
dev1 = regs["devices"][1]
dev2 = regs["devices"][2]

async def serialBench(ctx):
    ctx.set(dut.memory.data[dev0["control"]["address"]], 0b1)   # enable device 0
    ctx.set(dut.memory.data[dev0["control"]["address"]], 0b11)   # enable device 0 and cyclic mode
    ctx.set(dut.memory.data[dev0["rx_packet_size"]["address"]], 4)   # set rx packet size
    ctx.set(dut.memory.data[dev1["control"]["address"]], 0b1)   # enable device 1
    ctx.set(dut.memory.data[dev1["control"]["address"]], 0b11)   # enable device 1 and cyclic mode
    ctx.set(dut.memory.data[dev1["rx_packet_size"]["address"]], 4)   # set rx packet size
    ctx.set(dut.memory.data[dev2["control"]["address"]], 0b1)   # enable device 2
    ctx.set(dut.memory.data[dev2["control"]["address"]], 0b11)   # enable device 2 and cyclic mode
    ctx.set(dut.memory.data[dev2["rx_packet_size"]["address"]], 4)   # set rx packet size

    # config address and sequential registers
    ctx.set(dut.memory.data[dev0["cyclic_config_0"]["address"]], 0x01001)   # config RX/TX reg0 for 1 byte 0 offset
    ctx.set(dut.memory.data[dev0["cyclic_config_1"]["address"]], 0x14014)   # config RX/TX reg1 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev0["cyclic_config_2"]["address"]], 0x14014)   # config RX/TX reg2 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev1["cyclic_config_0"]["address"]], 0x01001)   # config RX/TX reg0 for 1 byte 0 offset
    ctx.set(dut.memory.data[dev1["cyclic_config_1"]["address"]], 0x14014)   # config RX/TX reg1 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev1["cyclic_config_2"]["address"]], 0x14014)   # config RX/TX reg2 for 4 byte 1 offse1
    ctx.set(dut.memory.data[dev2["cyclic_config_0"]["address"]], 0x01001)   # config RX/TX reg0 for 1 byte 0 offset
    ctx.set(dut.memory.data[dev2["cyclic_config_1"]["address"]], 0x14014)   # config RX/TX reg1 for 4 byte 1 offset
    ctx.set(dut.memory.data[dev2["cyclic_config_2"]["address"]], 0x14014)   # config RX/TX reg2 for 4 byte 1 offset
    

    ctx.set(dut.memory.data[dev0["cyclic_write_data_0"]["address"]], 0xAB)
    ctx.set(dut.memory.data[dev0["cyclic_write_data_1"]["address"]], 0xFFFFFFFF)
    ctx.set(dut.memory.data[dev0["cyclic_write_data_2"]["address"]], 0x12345678)


    ctx.set(dut.memory.data[dev1["cyclic_write_data_0"]["address"]], 0xCD)
    ctx.set(dut.memory.data[dev1["cyclic_write_data_1"]["address"]], 0xFFFFFFFF)
    ctx.set(dut.memory.data[dev1["cyclic_write_data_2"]["address"]], 0x12345678)

    ctx.set(dut.memory.data[dev2["cyclic_write_data_0"]["address"]], 0xEF)
    ctx.set(dut.memory.data[dev2["cyclic_write_data_1"]["address"]], 0xFFFFFFFF)
    ctx.set(dut.memory.data[dev2["cyclic_write_data_2"]["address"]], 0x123456781)


    # load test response into debug serial port to simulate a device
    testTXpacket = [
    0x123456AA,
    0x70605040,
    0xB0A09080
    ]
    for e, index in enumerate(range(dut.debugSerialPort.registers.txDataBaseAddr, dut.debugSerialPort.registers.txDataBaseAddr+len(testTXpacket))):
        ctx.set(dut.debugSerialPort.memory.data[index], testTXpacket[e])

    # set device bit length (baud rate)
    ctx.set(dut.debugSerialPort.address, dut.debugSerialPort.registers.bitLength)
    ctx.set(dut.debugSerialPort.writeData, int(clock / 12.5e6))
    ctx.set(dut.debugSerialPort.writeEnable, True)
    await ctx.tick()
    ctx.set(dut.debugSerialPort.writeEnable, False)

    
    ctx.set(dut.address, regs["bit_length"]["address"])
    ctx.set(dut.writeData, int(clock / 12.5e6))
    ctx.set(dut.writeEnable, True)
    await ctx.tick()
    ctx.set(dut.writeEnable, False)
    await ctx.tick().repeat(4)

    ctx.set(dut.address, regs["control"]["address"])
    ctx.set(dut.writeData, 0b1)     # start transfers
    ctx.set(dut.writeEnable, True)
    await ctx.tick()
    ctx.set(dut.writeEnable, False)

    
    for i in range(3):

        # start RX on test device
        ctx.set(dut.debugSerialPort.address, 0x0)
        ctx.set(dut.debugSerialPort.writeData, 0b10 + 0x3040000)     # trigger rx and set rx/tx packet sizes (RX: 3, TX: 4)
        ctx.set(dut.debugSerialPort.writeEnable, True)
        await ctx.tick()
        ctx.set(dut.debugSerialPort.writeEnable, False)


        # wait for test device to begin receiving packet
        while(not ctx.get(dut.debugSerialPort.rxBusy)):
            await ctx.tick()

        # wait for test device to finish receiving packet
        while(ctx.get(dut.debugSerialPort.rxBusy)):
            await ctx.tick()

        assert ctx.get(dut.debugSerialPort.rxCRCvalid)

        await ctx.tick().repeat(200)

        # start TX on test device
        ctx.set(dut.debugSerialPort.address, 0x0)
        ctx.set(dut.debugSerialPort.writeData, 0b01)
        ctx.set(dut.debugSerialPort.writeEnable, True)
        await ctx.tick()
        ctx.set(dut.debugSerialPort.writeEnable, False)

        await ctx.tick().repeat(10)

    await ctx.tick().repeat(7000)

if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock)
    sim.add_testbench(serialBench)
    with sim.write_vcd("serial_controller.vcd"):
        sim.run()

    if (True):  # export
        top = serial_controller(int(100e6), 64, 4, False)
        with open("controller-firmware/src/amaranth sources/serial_controller.v", "w") as f:
            f.write(verilog.convert(top, name="serial_controller", ports=[
                                                                          top.address,
                                                                          top.writeData,
                                                                          top.writeEnable,
                                                                          top.readData,
                                                                          top.serialPort.rx,
                                                                          top.serialPort.tx
                                                                          ]))
