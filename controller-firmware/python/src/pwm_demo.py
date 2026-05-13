from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.wiring import Component, In, Out

from registers2 import *


class PWM_DEMO(Component):
    # Simple PWM demo for yt video

    def __init__(self):

        super().__init__({

            # define the Signals going in and out of the module

            "pwm_out" : Out(1),

            # ignore these for now
            "bram_address": In(16),
            "bram_write_data": In(32),
            "bram_read_data": Out(32),
            "bram_write_enable": In(1),
        })

        driver_settings = {}
        self.rm = RegisterMapGenerator("pwm_demo", ["pwm_demo"], driver_settings, "Simple PWM demo for yt video")

        self.rm.generate()

    def elaborate(self, platform):
        m = Module()

        clock_freq = 25_000_000  # 25 MHz
        pwm_freq = 10_000         # 10 kHz
        pwm_clk_cycles = clock_freq // pwm_freq  # Number of clock cycles per PWM cycle, note that this may not result in exact frequency specified

        print(f"PWM clock cycles: {pwm_clk_cycles}, actual PWM frequency: {clock_freq / pwm_clk_cycles} Hz")


        # Create the signals needed
        counter = Signal(range(pwm_clk_cycles)) # Counter that can count up to pwm_clk_cycles

        default_duty_cycle = pwm_clk_cycles // 2  # Default to 50% duty cycle
        # default_duty_cycle = 0
        
        duty_cycle = Signal(range(pwm_clk_cycles + 1), init=default_duty_cycle)

        # PWM logic

        # Increment the counter every clock cycle, reset to 0 when it reaches pwm_clk_cycles
        with m.If(counter == pwm_clk_cycles - 1):
            m.d.sync_25 += counter.eq(0)
        with m.Else():
            m.d.sync_25 += counter.eq(counter + 1)

        # Turn on ouput signal if the counter is less than the duty cycle, else turn it off
        with m.If(counter < duty_cycle):
            m.d.comb += self.pwm_out.eq(1)
        with m.Else():
            m.d.comb += self.pwm_out.eq(0)
        return m
    

dut = PWM_DEMO()

async def bench(ctx):
    # Testbench for PWM demo

    # Run for a while
    await ctx.tick("sync_25").repeat(10000)



if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/25e6, domain="sync_25")
    sim.add_testbench(bench)
    with sim.write_vcd("pwm_demo_test.vcd"):
        sim.run()
