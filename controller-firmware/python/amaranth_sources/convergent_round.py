from amaranth import *
from amaranth.sim import Simulator
import numpy as np
from amaranth.back import verilog
import math


class convergentRound(Elaboratable):
    """
    unsigned rounding with the lowest bias

    requires 1 clock cycle
    """

        
    def __init__(self, startingBits, endingBits) -> None:

        self.startingBits = startingBits
        self.endingBits = endingBits

        self.truncatedBits = self.startingBits-self.endingBits

        # ports
        self.dataIn = Signal(shape=signed(self.startingBits))
        self.dataOut = Signal(shape=signed(self.endingBits))

        self.ports = [self.dataIn, self.dataOut]

        self.debug = Signal(2)


    def elaborate(self, platform):
        m = Module()

        with m.If(self.dataIn.bit_select(0, self.truncatedBits) > 0b1 << self.truncatedBits-1): # round up
            m.d.sync += self.dataOut.eq(self.dataIn.bit_select(self.truncatedBits, self.endingBits) + 1)
            m.d.sync += self.debug.eq(1)

        with m.Elif(self.dataIn.bit_select(0, self.truncatedBits) < 0b1 << self.truncatedBits-1): # round down
            m.d.sync += self.dataOut.eq(self.dataIn.bit_select(self.truncatedBits, self.endingBits))
            m.d.sync += self.debug.eq(2)

        with m.Elif(self.dataIn.bit_select(0, self.truncatedBits) == 0b1 << self.truncatedBits-1): # round to even
            m.d.sync += self.dataOut.eq(self.dataIn.bit_select(self.truncatedBits, self.endingBits) + self.dataIn.bit_select(self.truncatedBits+1, 1))
            m.d.sync += self.debug.eq(3)
        
        return m
    

dut = convergentRound(12, 8)   # round a 32bit number to 16 bit

async def convergentRoundBench(ctx):
    for i in range(-2**11+1, 2**11-1):
        ctx.set(dut.dataIn, i)
        await ctx.tick()

        out = ctx.get(dut.dataOut)

        bias = (out * 8) - i

        print(i, out*8)
        

            
if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/int(100e6))
    sim.add_testbench(convergentRoundBench)
    with sim.write_vcd("convergentRound.vcd"):
        sim.run()

    # if (True):  # export
    #     top = convergentRound(32, 16)
    #     with open("controller-firmware/src/amaranth sources/convergentRound.v", "w") as f:
    #         f.write(verilog.convert(top, name="convergentRound", ports=top.ports))