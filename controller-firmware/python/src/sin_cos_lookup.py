from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog
from amaranth.lib.memory import Memory
from src.convergent_round import convergentRound
import matplotlib.pyplot as plt
import numpy as np


class sin_cos_lookup_32(Elaboratable):
    """
    high precision 32bit sin/cos lookup, commonly used for convering wrapping signals such as encoders to a continuous signal for processing
    """

    def __init__(self, tableSizePower = 8) -> None:

        """
        tableSize: number of lookup values to store in memory, intermediate values are found through linear interpolation. must be a power of 2
        """

        self.tableSize = 2**tableSizePower
        self.tableSizePower = tableSizePower

        # ports
        self.dataIn = Signal(32)
        self.sinOut = Signal(shape=signed(32))
        self.cosOut = Signal(shape=signed(32))
        
        self.outputReady = Signal()

        self.ports = [
            self.outputReady,
            self.dataIn,
            self.sinOut,
            self.cosOut,
        ]
        # internal signals
        self.oldDataIn = Signal(32)
        self.sinSign = Signal(shape=signed(2))
        self.cosSign = Signal(shape=signed(2))

        self.state = Signal(6)


    def elaborate(self, platform):
        m = Module()

        self.sinTable = []
        for i in range(self.tableSize+1):     # fill table with sin/cos values
            self.sinTable.append( int(np.ceil(np.sin(np.pi/2 * (i/(self.tableSize))) * 2**31-1)) | int(np.ceil(np.cos(np.pi/2 * (i/(self.tableSize))) * 2**31-1)) << 32)

        self.sinTable[0] = (2**31-1)<<32

        m.submodules.sinTable = self.sinTableMemory = Memory(shape=signed(64), depth=self.tableSize+1, init=self.sinTable)

        self.readPort = self.sinTableMemory.read_port()
        self.readPort2 = self.sinTableMemory.read_port()

        with m.If((self.dataIn[-1]==0) & (self.dataIn[-2]==0)):   # 0-25%
            m.d.comb += self.readPort.addr.eq(self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower))
            m.d.comb += self.readPort2.addr.eq(self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower) + 1)
            m.d.comb += self.sinSign.eq(1)
            m.d.comb += self.cosSign.eq(1)
        
        with m.Elif((self.dataIn[-1]==0) & (self.dataIn[-2]==1)):   # 25-50%
            m.d.comb += self.readPort.addr.eq(self.tableSize - self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower))
            m.d.comb += self.readPort2.addr.eq(self.tableSize - self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower)-1)
            m.d.comb += self.sinSign.eq(1)
            m.d.comb += self.cosSign.eq(-1)

        with m.Elif((self.dataIn[-1]==1) & (self.dataIn[-2]==0)):   # 50-75%
            m.d.comb += self.readPort.addr.eq(self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower))
            m.d.comb += self.readPort2.addr.eq(self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower) + 1)
            m.d.comb += self.sinSign.eq(-1)
            m.d.comb += self.cosSign.eq(-1)

        with m.Elif((self.dataIn[-1]==1) & (self.dataIn[-2]==1)):   # 75-100%
            m.d.comb += self.readPort.addr.eq(self.tableSize - self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower))
            m.d.comb += self.readPort2.addr.eq(self.tableSize - self.dataIn.bit_select(32 - 2 - self.tableSizePower, self.tableSizePower)-1)
            m.d.comb += self.sinSign.eq(-1)
            m.d.comb += self.cosSign.eq(1)

        m.d.sync += self.sinOut.eq(((self.readPort.data.bit_select(0, 32)*(~self.dataIn.bit_select(0, 32-1-self.tableSizePower-1)) + self.readPort2.data.bit_select(0, 32)*(self.dataIn.bit_select(0, 32-1-self.tableSizePower-1))).shift_right(32-1-self.tableSizePower-1)) * self.sinSign)

        m.d.sync += self.cosOut.eq(((self.readPort.data.bit_select(32, 32)*(~self.dataIn.bit_select(0, 32-1-self.tableSizePower-1)) + self.readPort2.data.bit_select(32, 32)*(self.dataIn.bit_select(0, 32-1-self.tableSizePower-1))).shift_right(32-1-self.tableSizePower-1)) * self.cosSign)

        return m

clock = int(100e6)
dut = sin_cos_lookup_32(10)

ENCODER_COUNT = 2**16

times = []
sin = []
cos = []
inputs = []
idealSin = []
idealCos = []
sinError = []
cosError = []


async def sincosBench(ctx):
    for i in range(0, 2**32, 2**20):

        idealSin.append(np.sin(np.pi*2 * i/(2**32-1)) * 2**31-1)
        idealCos.append(np.cos(np.pi*2 * i/(2**32-1)) * 2**31-1)
        
        inputVal = i
        ctx.set(dut.dataIn, inputVal)
        await ctx.tick()

        x=1
        while(not ctx.get(dut.outputReady) and x > 0):
            x -= 1
            await ctx.tick()
        
        times.append(i)
        sin.append(ctx.get(dut.sinOut))
        cos.append(ctx.get(dut.cosOut))
        sinError.append(sin[-1] - idealSin[-1])
        cosError.append(cos[-1] - idealCos[-1])
        inputs.append(inputVal)


            
if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock)
    sim.add_testbench(sincosBench)
    with sim.write_vcd("sin_cos.vcd"):
        sim.run()

    # plt.plot(times, sin)
    # plt.plot(times, cos)
    # plt.plot(times, inputs)
    # plt.plot(times, idealSin)
    # plt.plot(times, idealCos)
    plt.plot(times, sinError)
    #plt.plot(times, cosError)
    plt.show()

    # if (True):  # export
    #     top = biquad_32(int(100e6))
    #     with open("controller-firmware/src/amaranth sources/biquad_32.v", "w") as f:
    #         f.write(verilog.convert(top, name="biquad_32", ports=top.ports))