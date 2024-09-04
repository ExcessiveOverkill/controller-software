from amaranth import *
from amaranth.sim import Simulator
import numpy as np
from amaranth.back import verilog
import math


class convergentRound(Elaboratable):
    """
    signed rounding with the lowest bias

    requires 1 clock cycle
    """

    def __init__(self, input_width, output_width):
        assert input_width > output_width, "Input width must be greater than output width"
        self.input = Signal(signed(input_width))
        self.LastInput = Signal(signed(input_width))
        self.output = Signal(signed(output_width))
        self.done = Signal(reset=1)
    
    def elaborate(self, platform):
        m = Module()

        # Calculate the bit-width difference
        shift = len(self.input) - len(self.output)

        # Extract the rounding bits
        round_bit = self.input[shift - 1]

        sticky_bit = self.input[:shift - 1].bool()

        # Determine if the input is negative
        is_negative = self.input[-1]

        self.rounding = Signal(signed(len(self.output)))

        with m.If(self.input[shift:-1].all() & ~is_negative):   # prevent overflow due to round-up when near max positive value
            m.d.comb += self.rounding.eq(self.input[shift:])

        with m.Elif(round_bit & sticky_bit):  # round up
            m.d.comb += self.rounding.eq(self.input[shift:] + 1)

        with m.Elif(~round_bit):  # round down
            m.d.comb += self.rounding.eq(self.input[shift:])

        with m.Elif(round_bit & ~sticky_bit):  # round to even
            m.d.comb += self.rounding.eq(self.input[shift:] + self.input[shift])

        m.d.sync += self.output.eq(self.rounding)
        m.d.sync += self.LastInput.eq(self.input)

        with m.If(self.input != self.LastInput):
            m.d.comb += self.done.eq(0)
        with m.Else():
            m.d.comb += self.done.eq(1)

        return m

    

dut = convergentRound(8, 4)   # round a 8bit number to 4 bit

async def convergentRoundBench(ctx):

    bias = 0
    
    for i in range(-2**7+1, 2**7-1):
        ctx.set(dut.input, i)
        await ctx.tick().repeat(3)

        out = ctx.get(dut.output)

        print(i, out)
        await ctx.tick()
        

            
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