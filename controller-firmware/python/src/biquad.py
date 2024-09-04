from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog
from src.convergent_round import convergentRound
import matplotlib.pyplot as plt


class biquad_32(Elaboratable):
    """
    high precision 32bit biquad IIR filter
    """

    def __init__(self, signed_, constrainOutput_) -> None:

        """
        signed: if true, use 32bit signed values for the input and output

        constrainOutput: force the output to be within the output range. Certain step inputs and coeff settings may cause the internal output value to exceed the output range, without counstrainOutput, the value will be left to wrap, but will exhibit the unchanged filter response
        """

        self.signed = signed_
        self.constrainOutput = constrainOutput_

        # ports
        self.update = Signal()
        if(self.signed):
            self.input = Signal(shape=signed(32))
            self.output = Signal(shape=signed(32))
        else:
            self.input = Signal(32)
            self.output = Signal(32)
        
        self.updateDone = Signal()

        self.zeroCoeff_0 = Signal(shape=signed(36))
        self.zeroCoeff_1 = Signal(shape=signed(36))
        self.zeroCoeff_2 = Signal(shape=signed(36))

        self.poleCoeff_1 = Signal(shape=signed(36))
        self.poleCoeff_2 = Signal(shape=signed(36))

        self.ports = [
            self.update,
            self.updateDone,
            self.input,
            self.output,
            self.zeroCoeff_0,
            self.zeroCoeff_1,
            self.zeroCoeff_2,
            self.poleCoeff_1,
            self.poleCoeff_2
        ]

        # internal signals
        self.dn_0 = Signal(shape=signed(48))
        self.dn_1 = Signal(shape=signed(48))

        self.zeroAccum = Signal(shape=signed(48*2))
        self.poleAccum = Signal(shape=signed(48*2))

        self.state = Signal(range(5))

        self.inputPrescaler = 2**32


    def elaborate(self, platform):
        m = Module()

        m.submodules.inputRound = self.inputRound = convergentRound(96, 96-32)
        m.submodules.outputRound = self.outputRound = convergentRound(96, 96-32)

        with m.If(self.update & self.state == 0):
            m.d.comb += self.inputRound.input.eq(self.inputPrescaler * self.input - self.dn_0 * self.poleCoeff_1 - self.dn_1 * self.poleCoeff_2)
            m.d.sync += self.state.eq(1)
            m.d.sync += self.updateDone.eq(0)
                
        with m.If(self.state == 1):
            m.d.sync += self.state.eq(2)
            m.d.comb += self.outputRound.input.eq(self.inputRound.output * self.zeroCoeff_0 + self.dn_0 * self.zeroCoeff_1 + self.dn_1 * self.zeroCoeff_2)
            m.d.sync += self.dn_0.eq(self.inputRound.output)
            m.d.sync += self.dn_1.eq(self.dn_0)

        with m.If(self.state == 2):
            
            if(self.constrainOutput):
                with m.If(self.outputRound.output >= 2**32):
                    m.d.sync += self.output.eq(2**32-1)
                with m.Elif(self.outputRound.output < 0):
                    m.d.sync += self.output.eq(0)
                with m.Else():
                    m.d.sync += self.output.eq(self.outputRound.output)
            else:
                m.d.sync += self.output.eq(self.outputRound.output)
            
            m.d.sync += self.updateDone.eq(1)
            m.d.sync += self.state.eq(Mux(self.update, 2, 0))

        return m
    

clock = int(100e6)
dut = biquad_32(False, True)

ENCODER_COUNT = 2**16

times = []
filteredPos = []
realEncoder = []
integerEncoder = []


async def biquadBench(ctx):
    ctx.set(dut.zeroCoeff_0, int(0.00003536167187236639 * 2**32))
    ctx.set(dut.zeroCoeff_1, int(0.00007072334374473279 * 2**32))
    ctx.set(dut.zeroCoeff_2, int(0.00003536167187236639 * 2**32))

    ctx.set(dut.poleCoeff_1, int(-1.9848607704104781 * 2**32))
    ctx.set(dut.poleCoeff_2, int(0.9850022170979679 * 2**32))

    encoderCountReal = 0.0
    encoderCountInteger = 0

    for i in range(4000):

        # update encoder
        encoderCountReal += 2**2
        encoderCountInteger = round(encoderCountReal, 0)
        if(encoderCountInteger > ENCODER_COUNT-1):
            encoderCountInteger -= ENCODER_COUNT
            encoderCountReal -= ENCODER_COUNT
        elif(encoderCountInteger < 0):
            encoderCountInteger += ENCODER_COUNT
            encoderCountReal += ENCODER_COUNT

        # if(i == 200):
        #     encoderCountReal += ENCODER_COUNT -50

        # if(i == 400):
        #     encoderCountReal += ENCODER_COUNT -50

        ctx.set(dut.input, int(int(encoderCountInteger) * (2**32 / ENCODER_COUNT)))
        ctx.set(dut.update, 1)
        await ctx.tick()
        ctx.set(dut.update, 0)

        while(not ctx.get(dut.updateDone)):
            await ctx.tick()
        
        times.append(i)
        filteredPos.append(ctx.get(dut.output))
        realEncoder.append(encoderCountReal * (2**32 / ENCODER_COUNT))
        integerEncoder.append(int(encoderCountInteger) * (2**32 / ENCODER_COUNT))


            
if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock)
    sim.add_testbench(biquadBench)
    with sim.write_vcd("biquad_32.vcd"):
        sim.run()

    plt.plot(times, filteredPos)
    plt.plot(times, realEncoder)
    plt.plot(times, integerEncoder)
    plt.show()

    # if (True):  # export
    #     top = biquad_32(int(100e6))
    #     with open("controller-firmware/src/amaranth sources/biquad_32.v", "w") as f:
    #         f.write(verilog.convert(top, name="biquad_32", ports=top.ports))