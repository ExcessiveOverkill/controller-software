from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from amaranth.lib.crc.catalog import CRC32_MPEG_2
from enum import IntEnum, auto
import numpy as np
from amaranth.back import verilog
import math


class drive_serial_port(Elaboratable):
    """
    Specific serial interface for communicating with our own devices at high speeds
    """

    """
    Registers (32 bit):

    
    """

    def __init__(self, clock:int, max_packet_size:int) -> None:
        """
        clock: input clock frequency (Hz)

        max_packet_size: maximum serial packet to send or receive (32 bit words)
        """
        
        self.clock = clock
        self.maxPacketSize = max_packet_size

        self.txStartBits = 1
        self.rxStartBits = 1
        self.txStopBits = 1
        self.rxStopBits = 1

        # Ports
        self.address = Signal(16)
        self.writeData = Signal(32)
        self.readData = Signal(32)
        self.writeEnable = Signal()

        # physical pins
        self.rx = Signal()
        self.tx = Signal(reset=1)
        self.txen = Signal(reset=1)     # TX is always enabled since the bus is full duplex (rs422)


        class registers(IntEnum):
            control = 0x0
            bitLength = 0x1
            status = 0x2
            rxDataBaseAddr = 0x3
            txDataBaseAddr = 0x3 + self.maxPacketSize + 1

        self.registers = registers


    class txStates(IntEnum):
        IDLE = 0
        START_BITS_DELAY = auto()
        STOP_BITS_DELAY = auto()
        SEND = auto()

    class rxStates(IntEnum):
        IDLE = 0
        START_BITS_DELAY = auto()
        STOP_BITS_DELAY = auto()
        RECEIVE = auto()

    def elaborate(self, platform):
        m = Module()

        m.submodules.memory = self.memory = Memory(shape=unsigned(32), depth=(2*(self.maxPacketSize) + 3), init=[])   # rx+tx serial data + bitLength + status + control registers

        m.submodules.crc32_ethernet1 = self.txCRC = CRC32_MPEG_2(32).create()
        m.submodules.crc32_ethernet2 = self.rxCRC = CRC32_MPEG_2(32).create()


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

        # config
        self.txPacketSize = Signal(range(self.maxPacketSize), reset=1)
        self.rxPacketSize = Signal(range(self.maxPacketSize), reset=1)
        self.bitLength = Signal(range(clock // 115200 + 1), reset=clock // 125000000)
        
        # status
        self.txBusy = Signal()
        self.rxBusy = Signal()
        self.txDone = Signal(reset=1)
        self.rxDone = Signal(reset=1)
        self.rxCRCvalid = Signal()
        self.rxData = Signal(32)
        self.rxNewData = Signal(32)
        self.rxNewDataIndex = Signal(8)
        self.txReadData = Signal(32)

        self.rxWriteDataRequest = Signal()
        self.updateStatusDataRequest = Signal()
        self.txReadDataRequest = Signal()
        self.txReadDataRequestWait = Signal(2)

        self.txTimer = Signal(range(int(self.clock // (115200 // 2)) + 1))     # a timer that can count up to 2 bits at the lowest baud rate (115200)
        self.txState = Signal(Shape.cast(self.txStates), reset=self.txStates.IDLE)
        self.txCurrentBit = Signal(4)
        self.txCurrentByte = Signal(2)
        self.txCurrentWord = Signal(8)

        self.rxTimer = Signal(range(int(self.clock // (115200 // 2)) + 1))     # a timer that can count up to 2 bits at the lowest baud rate (115200)
        self.rxState = Signal(Shape.cast(self.rxStates), reset=self.rxStates.IDLE)
        self.rxCurrentBit = Signal(4)
        self.rxCurrentByte = Signal(2)
        self.rxCurrentWord = Signal(8)


        with m.If((self.writeEnable) & (self.address == 0x0)):    # control register was written to

            with m.If(self.writeData[0] == 1):  # TX start was written
                # Start transmit

                m.d.sync += self.txDone.eq(0)
                m.d.sync += self.txBusy.eq(1)
                m.d.sync += self.tx.eq(0)
                #m.d.sync += self.txen.eq(1)
                m.d.sync += self.txCurrentBit.eq(0)
                m.d.sync += self.txCurrentByte.eq(0)
                m.d.sync += self.txCurrentWord.eq(0)
                m.d.sync += self.txTimer.eq(self.bitLength)    # set timer to width of start bits
                m.d.sync += self.txState.eq(self.txStates.START_BITS_DELAY)
                m.d.sync += self.txReadDataRequest.eq(1)
                m.d.sync += self.updateStatusDataRequest.eq(1)
                m.d.sync += self.txCRC.start.eq(1)
                m.d.sync += self.txReadDataRequestWait.eq(0)

            
            with m.If(self.writeData[1] == 1):  # RX start was written
                # Start receive

                m.d.sync += self.rxDone.eq(0)
                m.d.sync += self.rxCRCvalid.eq(0)
                m.d.sync += self.rxCurrentBit.eq(0)
                m.d.sync += self.rxCurrentByte.eq(0)
                m.d.sync += self.rxCurrentWord.eq(0)
                m.d.sync += self.rxState.eq(self.rxStates.START_BITS_DELAY)
                m.d.sync += self.updateStatusDataRequest.eq(1)
                m.d.sync += self.rxCRC.start.eq(1)

            # set packet sizes providing the size is greater than zero
            with m.If((self.writeData.bit_select(16, 8) != 0) & (self.txDone == 1)):
                m.d.sync += self.txPacketSize.eq(self.writeData.bit_select(16, 8))
            
            with m.If((self.writeData.bit_select(24, 8) != 0) & (self.rxDone == 1)):
                m.d.sync += self.rxPacketSize.eq(self.writeData.bit_select(24, 8))

        with m.If((self.writeEnable) & (self.address == 0x1)):    # bitLength register was written to
            m.d.sync += self.bitLength.eq(self.writeData)

        with m.If(self.txReadDataRequestWait != 0):
            m.d.sync += self.txReadDataRequestWait.eq(self.txReadDataRequestWait - 1)
            with m.If(self.txReadDataRequestWait == 1):
                m.d.sync += self.txReadData.eq(self.internalReadPort.data)

        with m.If(self.txReadDataRequest):
            m.d.sync += self.internalReadWriteAddress.eq((0x3 + self.maxPacketSize + 1) + self.txCurrentWord)
            m.d.sync += self.txReadDataRequestWait.eq(2)
            m.d.sync += self.txReadDataRequest.eq(0)

        with m.Elif(self.rxWriteDataRequest):
            m.d.sync += self.internalReadWriteAddress.eq(0x3 + self.rxNewDataIndex)
            m.d.sync += self.internalWritePort.data.eq(self.rxNewData)
            m.d.sync += self.internalWritePort.en.eq(1)
            m.d.sync += self.rxWriteDataRequest.eq(0)

        with m.Elif(self.updateStatusDataRequest):
            m.d.sync += self.internalReadWriteAddress.eq(0x2)
            m.d.sync += self.internalWritePort.data.bit_select(0, 1).eq(self.txDone)
            m.d.sync += self.internalWritePort.data.bit_select(1, 1).eq(self.txBusy)
            m.d.sync += self.internalWritePort.data.bit_select(2, 1).eq(self.rxDone)
            m.d.sync += self.internalWritePort.data.bit_select(3, 1).eq(self.rxBusy)
            m.d.sync += self.internalWritePort.data.bit_select(4, 1).eq(self.rxCRCvalid)
            m.d.sync += self.internalWritePort.en.eq(1)
            m.d.sync += self.updateStatusDataRequest.eq(0)

        with m.If(self.internalWritePort.en):
            m.d.sync += self.internalWritePort.en.eq(0)

        with m.If(self.txCRC.valid == 1):
            m.d.sync += self.txCRC.valid.eq(0)

        with m.If(self.txCRC.start == 1):
            m.d.sync += self.txCRC.start.eq(0)

        with m.If(self.rxCRC.valid == 1):
            m.d.sync += self.rxCRC.valid.eq(0)

        with m.If(self.rxCRC.start == 1):
            m.d.sync += self.rxCRC.start.eq(0)

        # Send start bits
        with m.If(self.txState == self.txStates.START_BITS_DELAY):
            with m.If(self.txTimer == 0):
                with m.If(self.txCurrentWord == self.txPacketSize):     # send CRC word
                    m.d.sync += self.tx.eq(self.txCRC.crc.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit
                with m.Else():  # send data word
                    m.d.sync += self.tx.eq(self.txReadData.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit
                m.d.sync += self.txCurrentBit.eq(1)
                m.d.sync += self.txState.eq(self.txStates.SEND)
                m.d.sync += self.txTimer.eq(self.bitLength - 1)   # set timer to 1 bit width

            with m.Else():
                m.d.sync += self.txTimer.eq(self.txTimer - 1)

        # Send data bits
        with m.If(self.txState == self.txStates.SEND):
            with m.If(self.txTimer == 0):

                with m.If(self.txCurrentBit == 8):   # word is complete, send stop bits
                    m.d.sync += self.txState.eq(self.txStates.STOP_BITS_DELAY)    
                    m.d.sync += self.tx.eq(1)
                    m.d.sync += self.txTimer.eq(self.bitLength - 1)     # set timer to width of stop bits

                with m.Else():      # continue sending bits
                    with m.If(self.txCurrentWord == self.txPacketSize):     # send CRC word
                        m.d.sync += self.tx.eq(self.txCRC.crc.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit
                    with m.Else():  # send data word
                        m.d.sync += self.tx.eq(self.txReadData.bit_select(self.txCurrentBit + (self.txCurrentByte*8), 1))    # set tx to next data bit

                    m.d.sync += self.txCurrentBit.eq(self.txCurrentBit + 1)     # increment next bit to send
                    m.d.sync += self.txTimer.eq(self.bitLength - 1)   # set timer to 1 bit width

            with m.Else():
                m.d.sync += self.txTimer.eq(self.txTimer - 1)

        # Send stop bits
        with m.If(self.txState == self.txStates.STOP_BITS_DELAY):
            with m.If(self.txTimer == 0):
                with m.If((self.txCurrentByte == 3) & (self.txCurrentWord == self.txPacketSize)):   # end of all data and CRC
                    m.d.sync += self.txCurrentBit.eq(0)
                    m.d.sync += self.txBusy.eq(0)
                    m.d.sync += self.txDone.eq(1)
                    m.d.sync += self.tx.eq(1)
                    #m.d.sync += self.txen.eq(0)
                    m.d.sync += self.txState.eq(self.txStates.IDLE)
                    m.d.sync += self.updateStatusDataRequest.eq(1)
                
                with m.Else():  # start next word if not at end of packet

                    m.d.sync += self.tx.eq(0)
                    m.d.sync += self.txCurrentBit.eq(0)

                    with m.If(self.txCurrentByte == 3):
                        m.d.sync += self.txCurrentByte.eq(0)
                        m.d.sync += self.txCurrentWord.eq(self.txCurrentWord + 1)
                        m.d.sync += self.txCRC.data.eq(self.txReadData)
                        m.d.sync += self.txCRC.valid.eq(1)
                        with m.If(self.txCurrentWord != self.txPacketSize-1):
                            m.d.sync += self.txReadDataRequest.eq(1)

                    with m.Else():
                        m.d.sync += self.txCurrentByte.eq(self.txCurrentByte + 1)

                    m.d.sync += self.txTimer.eq(self.bitLength)    # set timer to width of start bits
                    m.d.sync += self.txState.eq(self.txStates.START_BITS_DELAY)


            with m.Else():
                m.d.sync += self.txTimer.eq(self.txTimer - 1)
                


        # Wait for start bit edge
        with m.If(self.rxState == self.rxStates.START_BITS_DELAY):
            with m.If(self.rx == 0):
                m.d.sync += self.rxCurrentBit.eq(0)
                m.d.sync += self.rxState.eq(self.rxStates.RECEIVE)
                m.d.sync += self.rxTimer.eq(self.bitLength + (self.bitLength // 2) - 1)   # set timer to number of start bits + 1/2 bit
                m.d.sync += self.rxBusy.eq(1)
                m.d.sync += self.updateStatusDataRequest.eq(1)

            with m.Else():
                m.d.sync += self.rxTimer.eq(self.rxTimer - 1)

        # receive data bits
        with m.If(self.rxState == self.rxStates.RECEIVE):
            with m.If(self.rxTimer == 0):
                m.d.sync += self.rxData.bit_select(self.rxCurrentBit + (self.rxCurrentByte*8), 1).eq(self.rx)    # save rx bit

                with m.If(self.rxCurrentBit == 8 - 1):   # word is complete, receive stop bits
                    m.d.sync += self.rxState.eq(self.rxStates.STOP_BITS_DELAY)    
                    m.d.sync += self.rxTimer.eq(self.bitLength)     # set timer to width of stop bits + 1/2

                with m.Else():      # continue receiving bits
                    m.d.sync += self.rxCurrentBit.eq(self.rxCurrentBit + 1)     # increment next bit to read
                    m.d.sync += self.rxTimer.eq(self.bitLength - 1)   # set timer to 1/2 bit width


            with m.Else():
                m.d.sync += self.rxTimer.eq(self.rxTimer - 1)

        # Wait for stop bits
        with m.If(self.rxState == self.rxStates.STOP_BITS_DELAY):
            with m.If(self.rxTimer == 0):
                
                with m.If((self.rxCurrentByte == 3) & (self.rxCurrentWord == self.rxPacketSize)):    # packet length + CRC reached
                    m.d.sync += self.rxCurrentBit.eq(0)
                    m.d.sync += self.rxCurrentByte.eq(0)
                    m.d.sync += self.rxCurrentWord.eq(0)
                    m.d.sync += self.rxBusy.eq(0)
                    m.d.sync += self.rxDone.eq(1)
                    m.d.sync += self.rxState.eq(self.rxStates.IDLE)
                    m.d.sync += self.updateStatusDataRequest.eq(1)
                    with m.If(self.rxCRC.crc == self.rxData):   # valid CRC received
                        m.d.sync += self.rxCRCvalid.eq(1)
                    with m.Else():
                        m.d.sync += self.rxCRCvalid.eq(0)
                    
                    
                with m.Else():
                    m.d.sync += self.rxBusy.eq(1)
                    m.d.sync += self.rxCurrentBit.eq(0)

                    with m.If(self.rxCurrentByte == 3):
                        m.d.sync += self.rxCurrentByte.eq(0)
                        m.d.sync += self.rxCurrentWord.eq(self.rxCurrentWord + 1)
                        m.d.sync += self.rxNewData.eq(self.rxData)
                        m.d.sync += self.rxCRC.data.eq(self.rxData)
                        m.d.sync += self.rxCRC.valid.eq(1)
                        m.d.sync += self.rxNewDataIndex.eq(self.rxCurrentWord)
                        m.d.sync += self.rxWriteDataRequest.eq(1)

                    with m.Else():
                        m.d.sync += self.rxCurrentByte.eq(self.rxCurrentByte + 1)
                
                    m.d.sync += self.rxState.eq(self.rxStates.START_BITS_DELAY)

            with m.Else():
                m.d.sync += self.rxTimer.eq(self.rxTimer - 1)
                

        return m

clock = int(100e6) # 100 Mhz
dut = drive_serial_port(clock, 64)

testTXpacket = [
    0x3020100,
    0x7060504,
    0xB0A0908,
    0xF0E0D0C
]

async def uartBench(ctx):
    for e, index in enumerate(range(0x3 + 64 + 1, 0x3 + 64 + 1+4)):
        ctx.set(dut.memory.data[index], testTXpacket[e])
    ctx.set(dut.rx, ctx.get(dut.tx))
    ctx.set(dut.address, 0x1)
    ctx.set(dut.writeData, int(clock / 12.5e6))
    ctx.set(dut.writeEnable, True)
    await ctx.tick()
    ctx.set(dut.writeEnable, False)

    ctx.set(dut.address, 0x0)
    ctx.set(dut.writeData, 0b11 + 0x4040000)     # trigger tx/rx and set tx/rx packet size to 4x32bit
    ctx.set(dut.writeEnable, True)
    await ctx.tick()
    ctx.set(dut.writeEnable, False)
    for i in range(2000):
        ctx.set(dut.rx, ctx.get(dut.tx))
        await ctx.tick()

    # verify tx and rx are working properly
    for e, index in enumerate(range(0x3 + 64 + 1, 0x3 + 64 + 1+len(testTXpacket))):
        print(ctx.get(dut.memory.data[index]), ctx.get(dut.memory.data[e+0x03]))
        assert(ctx.get(dut.memory.data[index]) == ctx.get(dut.memory.data[e+0x03]))

    assert(ctx.get(dut.rxCRCvalid))

    ctx.set(dut.writeEnable, True)
    await ctx.tick()
    ctx.set(dut.writeEnable, False)
    for i in range(2000):
        ctx.set(dut.rx, ctx.get(dut.tx))
        await ctx.tick()

    # verify tx and rx are working properly
    for e, index in enumerate(range(0x3 + 64 + 1, 0x3 + 64 + 1+len(testTXpacket))):
        print(ctx.get(dut.memory.data[index]), ctx.get(dut.memory.data[e+0x03]))
        assert(ctx.get(dut.memory.data[index]) == ctx.get(dut.memory.data[e+0x03]))
    
    assert(ctx.get(dut.rxCRCvalid))

if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock)
    sim.add_testbench(uartBench)
    with sim.write_vcd("drive_serial_port.vcd"):
        sim.run()

    if (True):  # export
        top = drive_serial_port(int(100e6), 64)
        with open("controller-firmware/src/amaranth sources/serial_port.v", "w") as f:
            f.write(verilog.convert(top, name="serial_port", ports=[      top.address,
                                                                          top.writeData,
                                                                          top.writeEnable,
                                                                          top.readData,
                                                                          top.rx,
                                                                          top.tx
                                                                          ]))