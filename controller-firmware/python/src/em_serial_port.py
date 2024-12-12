from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from amaranth.lib.crc.catalog import CRC32_MPEG_2
from amaranth.lib.wiring import Component, In, Out
from amaranth.lib.cdc import FFSynchronizer, DomainRenamer
from registers2 import *


class EM_Serial_Port(Component):
    """
    Specific serial interface for communicating with our own devices at high speeds
    """

    def __init__(self, max_packet_size:int) -> None:
        """
        clock: input clock frequency (Hz)

        max_packet_size: maximum serial packet to send or receive (32 bit words)
        """
        assert(max_packet_size > 0)
        assert(max_packet_size & (max_packet_size - 1)) == 0, "max_packet_size must be a power of 2"
        
        self.maxPacketSize = max_packet_size

        # status
        self.txBusy = Signal()
        self.rxBusy = Signal()
        self.txDone = Signal(reset=1)
        self.rxDone = Signal(reset=1)
        self.rxCRCvalid = Signal()

        super().__init__({
            "bram_address": In(16),
            "bram_write_data": In(32),
            "bram_read_data": Out(32),
            "bram_write_enable": In(1),

            "rx": In(1),
            "tx": Out(1, reset=1),
            "tx_enable": Out(1, reset=1)
        })

        driver_settings = {}
        self.rm = RegisterMapGenerator("em_serial_port", ["em_serial_port"], driver_settings, "Serial port for communicating with other EM devices")
        self.rm.add(Register("rx_data", rw="r", type="unsigned", desc="Received data from the serial port", bank_size=self.maxPacketSize, start_address=0x0))
        self.rm.add(Register("tx_data", rw="w", type="unsigned", desc="Data to send over the serial port", bank_size=self.maxPacketSize, start_address=self.maxPacketSize))
        
        self.rm.add(Register("control", rw="w", type="unsigned", desc="Control register for managing the serial port", start_address=0x8000, sub_registers=[
            Register("tx_start", type="bool", desc="Start the transmit process"),
            Register("rx_start", type="bool", desc="Start the receive process"),
            Register("tx_packet_size", width=8, desc="Size of the transmit packet in 32 bit words", start_address=16),
            Register("rx_packet_size", width=8, desc="Size of the receive packet in 32 bit words", start_address=24)
        ]))
        self.rm.add(Register("bit_length", rw="w", type="unsigned", desc="Number of clock cycles per bit (100MHz)", start_address=0x8001))
        self.rm.add(Register("status", rw="r", type="unsigned", desc="Status register for the serial port", start_address=0x8002, sub_registers=[
            Register("tx_done", type="bool", desc="Transmit process is complete", start_address=0),
            Register("tx_busy", type="bool", desc="Transmit process is busy", start_address=1),
            Register("rx_done", type="bool", desc="Receive process is complete", start_address=2),
            Register("rx_busy", type="bool", desc="Receive process is busy", start_address=3),
            Register("rx_crc_valid", type="bool", desc="Receive CRC received is valid", start_address=4)
            ]))
        
        self.rm.generate()

    def elaborate(self, platform):
        m = Module()

        # control
        self.tx_start = Signal()
        self.rx_start = Signal()
        self.txPacketSize = Signal(8, reset=1)
        self.rxPacketSize = Signal(8, reset=1)

        # bit length
        self.bitLength = Signal(range(int(100e6 / 115200) + 1), reset=(int(100e6 / 12.5e6))) # support baud rates down to 115200


        # crc modules
        m.submodules.crc32_tx = self.txCRC = DomainRenamer("sync_100")(CRC32_MPEG_2(32).create())
        m.submodules.crc32_rx = self.rxCRC = DomainRenamer("sync_100")(CRC32_MPEG_2(32).create())

        self.rx_synced = Signal()
        m.submodules += FFSynchronizer(i=self.rx, o=self.rx_synced, o_domain="sync_100")

        # memory for storing serial data
        m.submodules.memory = self.memory = Memory(shape=unsigned(32), depth=(2*(self.maxPacketSize)), init=[])   # rx+tx serial data

        self.externalReadPort = self.memory.read_port(domain="sync_100")
        self.externalWritePort = self.memory.write_port(domain="sync_100")
        self.internalReadPort = self.memory.read_port(domain="sync_100")
        self.internalWritePort = self.memory.write_port(domain="sync_100")

        self.address = Signal(16)
        self.writeData = Signal(32)
        self.writeEnable = Signal()
        self.readData = Signal(32)

        m.d.comb += self.externalReadPort.addr.eq(self.address)
        m.d.comb += self.externalWritePort.addr.eq(self.address)
        m.d.comb += self.externalWritePort.data.eq(self.writeData)
        m.d.comb += self.externalWritePort.en.eq(self.writeEnable)
        m.d.comb += self.readData.eq(self.externalReadPort.data)

        self.internalReadWriteAddress = Signal(16)
        m.d.comb += self.internalReadPort.addr.eq(self.internalReadWriteAddress)
        m.d.comb += self.internalWritePort.addr.eq(self.internalReadWriteAddress)


        # internal signals
        self.rxData = Signal(32)
        self.rxNewData = Signal(32)
        self.rxNewDataIndex = Signal(8)
        self.txReadData = Signal(32)

        self.rxWriteDataRequest = Signal()
        self.updateStatusDataRequest = Signal()
        self.txReadDataRequest = Signal()
        self.txReadDataRequestWait = Signal(2)

        self.txTimer = Signal(range(int(100e6 / (115200 / 2)) + 1))     # a timer that can count up to 2 bits at the lowest baud rate (115200)
        #self.txState = Signal(Shape.cast(self.txStates), reset=self.txStates.IDLE)
        self.txCurrentBit = Signal(4)
        self.txCurrentByte = Signal(2)
        self.txCurrentWord = Signal(8)

        self.rxTimer = Signal(range(int(100e6 / (115200 / 2)) + 1))     # a timer that can count up to 2 bits at the lowest baud rate (115200)
        #self.rxState = Signal(Shape.cast(self.rxStates), reset=self.rxStates.IDLE)
        self.rxCurrentBit = Signal(4)
        self.rxCurrentByte = Signal(2)
        self.rxCurrentWord = Signal(8)


        # handle bram interface

        self.bram_control_mode = Signal()
        with m.If(self.bram_address[15]):
            m.d.sync_100 += self.bram_control_mode.eq(1)
        with m.Else():
            m.d.sync_100 += self.bram_control_mode.eq(0)

        # handle read/write for control/status registers
        with m.If(self.bram_write_enable):
            with m.Switch(self.bram_address):
                with m.Case(self.rm.control.address_offset):
                    m.d.sync_100 += self.tx_start.eq(self.bram_write_data[self.rm.control.tx_start.starting_bit])
                    m.d.sync_100 += self.rx_start.eq(self.bram_write_data[self.rm.control.rx_start.starting_bit])
                    m.d.sync_100 += self.txPacketSize.eq(self.bram_write_data[self.rm.control.tx_packet_size.starting_bit:self.rm.control.tx_packet_size.starting_bit+8])
                    m.d.sync_100 += self.rxPacketSize.eq(self.bram_write_data[self.rm.control.rx_packet_size.starting_bit:self.rm.control.rx_packet_size.starting_bit+8])
                
                with m.Case(self.rm.bit_length.address_offset):
                    m.d.sync_100 += self.bitLength.eq(self.bram_write_data)

        with m.Switch(self.bram_address):
            with m.Case(self.rm.status.address_offset):
                with m.If(self.bram_control_mode == 1):
                    m.d.comb += self.bram_read_data.eq(Cat(self.txDone, self.txBusy, self.rxDone, self.rxBusy, self.rxCRCvalid))
        
        # handle read/write for data registers
        m.d.comb += self.address.eq(self.bram_address)
        with m.If(~self.bram_control_mode):
            m.d.comb += self.writeData.eq(self.bram_write_data)
            m.d.comb += self.bram_read_data.eq(self.readData)
            m.d.comb += self.writeEnable.eq(self.bram_write_enable)


        # tx read request counter
        with m.If(self.txReadDataRequestWait != 0):
            m.d.sync_100 += self.txReadDataRequestWait.eq(self.txReadDataRequestWait - 1)
            with m.If(self.txReadDataRequestWait == 1):
                m.d.sync_100 += self.txReadData.eq(self.internalReadPort.data)

        with m.If(self.txReadDataRequest):
            m.d.sync_100 += self.internalReadWriteAddress.eq(self.txCurrentWord | self.maxPacketSize)
            m.d.sync_100 += self.txReadDataRequestWait.eq(2)
            m.d.sync_100 += self.txReadDataRequest.eq(0)

        with m.Elif(self.rxWriteDataRequest):
            m.d.sync_100 += self.internalReadWriteAddress.eq(self.rxNewDataIndex)
            m.d.sync_100 += self.internalWritePort.data.eq(self.rxNewData)
            m.d.sync_100 += self.internalWritePort.en.eq(1)
            m.d.sync_100 += self.rxWriteDataRequest.eq(0)

        with m.If(self.internalWritePort.en):
            m.d.sync_100 += self.internalWritePort.en.eq(0)

        with m.If(self.txCRC.valid == 1):
            m.d.sync_100 += self.txCRC.valid.eq(0)

        with m.If(self.txCRC.start == 1):
            m.d.sync_100 += self.txCRC.start.eq(0)

        with m.If(self.rxCRC.valid == 1):
            m.d.sync_100 += self.rxCRC.valid.eq(0)

        with m.If(self.rxCRC.start == 1):
            m.d.sync_100 += self.rxCRC.start.eq(0)

        with m.FSM(init="idle", domain="sync_100", name="tx_fsm") as fsm:
            with m.State("idle"):
                with m.If(self.tx_start):
                    m.d.sync_100 += self.tx_start.eq(0)
                    m.next = "start"

            with m.State("start"):
                m.d.sync_100 += [
                    self.txDone.eq(0),
                    self.txBusy.eq(1),
                    self.tx.eq(0),
                    #self.txen.eq(1),
                    self.txCurrentBit.eq(0),
                    self.txCurrentByte.eq(0),
                    self.txCurrentWord.eq(0),
                    self.txTimer.eq(self.bitLength),    # set timer to width of start bits
                    self.txReadDataRequest.eq(1),
                    self.updateStatusDataRequest.eq(1),
                    self.txCRC.start.eq(1),
                    self.txReadDataRequestWait.eq(0)
                ]
                m.next = "start_bits_delay"

            with m.State("start_bits_delay"):
                with m.If(self.txTimer == 0):
                    with m.If(self.txCurrentWord == self.txPacketSize):     # send CRC word
                        m.d.sync_100 += self.tx.eq(self.txCRC.crc.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit
                    with m.Else():  # send data word
                        m.d.sync_100 += self.tx.eq(self.txReadData.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit
                    m.d.sync_100 += self.txCurrentBit.eq(1)
                    m.d.sync_100 += self.txTimer.eq(self.bitLength - 1)   # set timer to 1 bit width
                    m.next = "send"

                with m.Else():
                    m.d.sync_100 += self.txTimer.eq(self.txTimer - 1)

            with m.State("send"):
                with m.If(self.txTimer == 0):

                    with m.If(self.txCurrentBit == 8):   # word is complete, send stop bits 
                        m.d.sync_100 += self.tx.eq(1)
                        m.d.sync_100 += self.txTimer.eq(self.bitLength - 1)     # set timer to width of stop bits
                        m.next = "stop_bits_delay"

                    with m.Else():      # continue sending bits
                        with m.If(self.txCurrentWord == self.txPacketSize):     # send CRC word
                            m.d.sync_100 += self.tx.eq(self.txCRC.crc.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit
                        with m.Else():  # send data word
                            m.d.sync_100 += self.tx.eq(self.txReadData.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit

                        m.d.sync_100 += self.txCurrentBit.eq(self.txCurrentBit + 1)     # increment next bit to send
                        m.d.sync_100 += self.txTimer.eq(self.bitLength - 1)   # set timer to 1 bit width

                with m.Else():
                    m.d.sync_100 += self.txTimer.eq(self.txTimer - 1)

            with m.State("stop_bits_delay"):
                with m.If(self.txTimer == 0):
                    with m.If((self.txCurrentByte == 3) & (self.txCurrentWord == self.txPacketSize)):   # end of all data and CRC
                        m.d.sync_100 += self.txCurrentBit.eq(0)
                        m.d.sync_100 += self.txBusy.eq(0)
                        m.d.sync_100 += self.txDone.eq(1)
                        m.d.sync_100 += self.tx.eq(1)
                        #m.d.sync_100 += self.txen.eq(0)
                        m.d.sync_100 += self.updateStatusDataRequest.eq(1)
                        m.next = "idle"
                    
                    with m.Else():  # start next word if not at end of packet

                        m.d.sync_100 += self.tx.eq(0)
                        m.d.sync_100 += self.txCurrentBit.eq(0)

                        with m.If(self.txCurrentByte == 3):
                            m.d.sync_100 += self.txCurrentByte.eq(0)
                            m.d.sync_100 += self.txCurrentWord.eq(self.txCurrentWord + 1)
                            m.d.sync_100 += self.txCRC.data.eq(self.txReadData)
                            m.d.sync_100 += self.txCRC.valid.eq(1)
                            with m.If(self.txCurrentWord != self.txPacketSize-1):
                                m.d.sync_100 += self.txReadDataRequest.eq(1)

                        with m.Else():
                            m.d.sync_100 += self.txCurrentByte.eq(self.txCurrentByte + 1)

                        m.d.sync_100 += self.txTimer.eq(self.bitLength)    # set timer to width of start bits
                        m.next = "start_bits_delay"


                with m.Else():
                    m.d.sync_100 += self.txTimer.eq(self.txTimer - 1)


        with m.FSM(init="idle", domain="sync_100", name="rx_fsm") as fsm:
            with m.State("idle"):
                with m.If(self.rx_start):
                    m.d.sync_100 += self.rx_start.eq(0)
                    m.next = "start"

            with m.State("start"):
                m.d.sync_100 += [
                    self.rxDone.eq(0),
                    self.rxCRCvalid.eq(0),
                    self.rxCurrentBit.eq(0),
                    self.rxCurrentByte.eq(0),
                    self.rxCurrentWord.eq(0),
                    self.updateStatusDataRequest.eq(1),
                    self.rxCRC.start.eq(1),
                ]
                m.next = "start_bits_delay"

            with m.State("start_bits_delay"):
                with m.If(self.rx_synced == 0):
                    m.d.sync_100 += self.rxCurrentBit.eq(0)
                    m.d.sync_100 += self.rxTimer.eq(self.bitLength + (self.bitLength // 2) - 1)   # set timer to number of start bits + 1/2 bit
                    m.d.sync_100 += self.rxBusy.eq(1)
                    m.d.sync_100 += self.updateStatusDataRequest.eq(1)
                    m.next = "receive"

                with m.Else():
                    m.d.sync_100 += self.rxTimer.eq(self.rxTimer - 1)

            with m.State("receive"):
                with m.If(self.rxTimer == 0):
                    m.d.sync_100 += self.rxData.bit_select(self.rxCurrentBit + (self.rxCurrentByte*8), 1).eq(self.rx_synced)    # save rx bit

                    with m.If(self.rxCurrentBit == 8 - 1):   # word is complete, receive stop bits
                        m.d.sync_100 += self.rxTimer.eq(self.bitLength)     # set timer to width of stop bits + 1/2
                        m.next = "stop_bits_delay"

                    with m.Else():      # continue receiving bits
                        m.d.sync_100 += self.rxCurrentBit.eq(self.rxCurrentBit + 1)     # increment next bit to read
                        m.d.sync_100 += self.rxTimer.eq(self.bitLength - 1)   # set timer to 1/2 bit width


                with m.Else():
                    m.d.sync_100 += self.rxTimer.eq(self.rxTimer - 1)

            with m.State("stop_bits_delay"):
                with m.If(self.rxTimer == 0):
                
                    with m.If((self.rxCurrentByte == 3) & (self.rxCurrentWord == self.rxPacketSize)):    # packet length + CRC reached
                        m.d.sync_100 += self.rxCurrentBit.eq(0)
                        m.d.sync_100 += self.rxCurrentByte.eq(0)
                        m.d.sync_100 += self.rxCurrentWord.eq(0)
                        m.d.sync_100 += self.rxBusy.eq(0)
                        m.d.sync_100 += self.rxDone.eq(1)
                        m.d.sync_100 += self.updateStatusDataRequest.eq(1)

                        with m.If(self.rxCRC.crc == self.rxData):   # valid CRC received
                            m.d.sync_100 += self.rxCRCvalid.eq(1)
                        with m.Else():
                            m.d.sync_100 += self.rxCRCvalid.eq(0)

                        m.next = "idle"
                        
                        
                    with m.Else():
                        m.d.sync_100 += self.rxBusy.eq(1)
                        m.d.sync_100 += self.rxCurrentBit.eq(0)

                        with m.If(self.rxCurrentByte == 3):
                            m.d.sync_100 += self.rxCurrentByte.eq(0)
                            m.d.sync_100 += self.rxCurrentWord.eq(self.rxCurrentWord + 1)
                            m.d.sync_100 += self.rxNewData.eq(self.rxData)
                            m.d.sync_100 += self.rxCRC.data.eq(self.rxData)
                            m.d.sync_100 += self.rxCRC.valid.eq(1)
                            m.d.sync_100 += self.rxNewDataIndex.eq(self.rxCurrentWord)
                            m.d.sync_100 += self.rxWriteDataRequest.eq(1)

                        with m.Else():
                            m.d.sync_100 += self.rxCurrentByte.eq(self.rxCurrentByte + 1)
                    
                        m.next = "start_bits_delay"

                with m.Else():
                    m.d.sync_100 += self.rxTimer.eq(self.rxTimer - 1)

        # # Wait for start bit edge
        # with m.If(self.rxState == self.rxStates.START_BITS_DELAY):
        #     with m.If(self.rx == 0):
        #         m.d.sync_100 += self.rxCurrentBit.eq(0)
        #         m.d.sync_100 += self.rxState.eq(self.rxStates.RECEIVE)
        #         m.d.sync_100 += self.rxTimer.eq(self.bitLength + (self.bitLength // 2) - 1)   # set timer to number of start bits + 1/2 bit
        #         m.d.sync_100 += self.rxBusy.eq(1)
        #         m.d.sync_100 += self.updateStatusDataRequest.eq(1)

        #     with m.Else():
        #         m.d.sync_100 += self.rxTimer.eq(self.rxTimer - 1)

        # # receive data bits
        # with m.If(self.rxState == self.rxStates.RECEIVE):
        #     with m.If(self.rxTimer == 0):
        #         m.d.sync_100 += self.rxData.bit_select(self.rxCurrentBit + (self.rxCurrentByte*8), 1).eq(self.rx)    # save rx bit

        #         with m.If(self.rxCurrentBit == 8 - 1):   # word is complete, receive stop bits
        #             m.d.sync_100 += self.rxState.eq(self.rxStates.STOP_BITS_DELAY)    
        #             m.d.sync_100 += self.rxTimer.eq(self.bitLength)     # set timer to width of stop bits + 1/2

        #         with m.Else():      # continue receiving bits
        #             m.d.sync_100 += self.rxCurrentBit.eq(self.rxCurrentBit + 1)     # increment next bit to read
        #             m.d.sync_100 += self.rxTimer.eq(self.bitLength - 1)   # set timer to 1/2 bit width


        #     with m.Else():
        #         m.d.sync_100 += self.rxTimer.eq(self.rxTimer - 1)

        # # Wait for stop bits
        # with m.If(self.rxState == self.rxStates.STOP_BITS_DELAY):
        #     with m.If(self.rxTimer == 0):
                
        #         with m.If((self.rxCurrentByte == 3) & (self.rxCurrentWord == self.rxPacketSize)):    # packet length + CRC reached
        #             m.d.sync_100 += self.rxCurrentBit.eq(0)
        #             m.d.sync_100 += self.rxCurrentByte.eq(0)
        #             m.d.sync_100 += self.rxCurrentWord.eq(0)
        #             m.d.sync_100 += self.rxBusy.eq(0)
        #             m.d.sync_100 += self.rxDone.eq(1)
        #             m.d.sync_100 += self.rxState.eq(self.rxStates.IDLE)
        #             m.d.sync_100 += self.updateStatusDataRequest.eq(1)
        #             with m.If(self.rxCRC.crc == self.rxData):   # valid CRC received
        #                 m.d.sync_100 += self.rxCRCvalid.eq(1)
        #             with m.Else():
        #                 m.d.sync_100 += self.rxCRCvalid.eq(0)
                    
                    
        #         with m.Else():
        #             m.d.sync_100 += self.rxBusy.eq(1)
        #             m.d.sync_100 += self.rxCurrentBit.eq(0)

        #             with m.If(self.rxCurrentByte == 3):
        #                 m.d.sync_100 += self.rxCurrentByte.eq(0)
        #                 m.d.sync_100 += self.rxCurrentWord.eq(self.rxCurrentWord + 1)
        #                 m.d.sync_100 += self.rxNewData.eq(self.rxData)
        #                 m.d.sync_100 += self.rxCRC.data.eq(self.rxData)
        #                 m.d.sync_100 += self.rxCRC.valid.eq(1)
        #                 m.d.sync_100 += self.rxNewDataIndex.eq(self.rxCurrentWord)
        #                 m.d.sync_100 += self.rxWriteDataRequest.eq(1)

        #             with m.Else():
        #                 m.d.sync_100 += self.rxCurrentByte.eq(self.rxCurrentByte + 1)
                
        #             m.d.sync_100 += self.rxState.eq(self.rxStates.START_BITS_DELAY)

        #     with m.Else():
        #         m.d.sync_100 += self.rxTimer.eq(self.rxTimer - 1)
                

        return m



clock = 100e6

dut = EM_Serial_Port(64)

testTXpacket = [
    0x3020100,
    0x7060504,
    0xB0A0908,
    0xF0E0D0C
]

async def uartBench(ctx):
    for e, index in enumerate(range(64, 64+len(testTXpacket))):
        ctx.set(dut.memory.data[index], testTXpacket[e])
    
    ctx.set(dut.rx, ctx.get(dut.tx))
    ctx.set(dut.bram_address, dut.rm.bit_length.address_offset)
    ctx.set(dut.bram_write_data, int(clock / 12.5e6))
    ctx.set(dut.bram_write_enable, True)
    await ctx.tick("sync_100")
    ctx.set(dut.bram_write_enable, False)

    ctx.set(dut.bram_address, dut.rm.control.address_offset)

    # trigger tx/rx and set tx/rx packet size to 4x32bit
    data = (0b1 << dut.rm.control.tx_start.starting_bit) | (0b1 << dut.rm.control.rx_start.starting_bit) | (0x4 << dut.rm.control.tx_packet_size.starting_bit) | (0x4 << dut.rm.control.rx_packet_size.starting_bit)
    ctx.set(dut.bram_write_data, data)     
    ctx.set(dut.bram_write_enable, True)
    await ctx.tick("sync_100")
    ctx.set(dut.bram_write_enable, False)
    for i in range(4000):
        ctx.set(dut.rx, ctx.get(dut.tx))
        await ctx.tick("sync_100")

    # verify tx and rx are working properly
    for e, index in enumerate(range(64, 64+len(testTXpacket))):
        print(ctx.get(dut.memory.data[index]), ctx.get(dut.memory.data[e]))
        #assert(ctx.get(dut.memory.data[index]) == ctx.get(dut.memory.data[e]))
        if not ctx.get(dut.memory.data[index]) == ctx.get(dut.memory.data[e]):
            print("Data mismatch")
        

    #assert(ctx.get(dut.rxCRCvalid))
    if not ctx.get(dut.rxCRCvalid):
        print("CRC invalid")



if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock, domain="sync_100")
    sim.add_testbench(uartBench)
    with sim.write_vcd("em_serial_port.vcd"):
        sim.run()