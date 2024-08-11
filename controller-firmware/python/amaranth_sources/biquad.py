from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from amaranth.lib.crc.catalog import CRC32_MPEG_2
from enum import IntEnum, auto
import numpy as np
from amaranth.back import verilog
import math


class biquad(Elaboratable):
    """
    high precision unsigned biquad IIR filter with data wrap support(input steps over 50% input range are not treated as steps)
    """

    """
    Registers (32 bit):
    """

        
    def __init__(self, clock:int) -> None:

        self.clock = clock

        # ports
        self.update = Signal()
        self.dataIn = Signal(32)
        self.dataOut = Signal(32)

        self.zeroCoeff_0 = Signal(shape=signed(36))
        self.zeroCoeff_1 = Signal(shape=signed(36))
        self.zeroCoeff_2 = Signal(shape=signed(36))

        self.poleCoeff_1 = Signal(shape=signed(36))
        self.poleCoeff_2 = Signal(shape=signed(36))


        # internal signals
        self.dn_0 = Signal(shape=signed(48))
        self.dn_1 = Signal(shape=signed(48))

        self.zeroAccum = Signal(shape=signed(48*2))
        self.poleAccum = Signal(shape=signed(48*(2)))

        self.state = Signal(range(5))

        self.inputPrescaler = 2**16


    def elaborate(self, platform):
        m = Module()

        with m.If(self.update):
            m.d.sync += self.state.eq(1)
            m.d.sync += self.poleAccum.eq(self.inputPrescaler * self.dataIn - self.dn_0 * self.poleCoeff_1 - self.dn_1 * self.poleCoeff_2)

        with m.If(self.state == 1):
            m.d.sync += self.poleAccum


        
        return m
    

clock = int(100e6)
dut = biquad(clock)

async def biquadBench(ctx):
    pass

            
if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock)
    sim.add_testbench(biquadBench)
    with sim.write_vcd("biquad.vcd"):
        sim.run()

    if (True):  # export
        top = biquad(int(100e6))
        with open("controller-firmware/src/amaranth sources/biquad.v", "w") as f:
            f.write(verilog.convert(top, name="biquad", ports=top.ports))