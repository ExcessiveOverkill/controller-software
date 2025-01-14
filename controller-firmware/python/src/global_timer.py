from amaranth import *
from amaranth.lib.wiring import Component, In, Out
from amaranth.sim import Simulator
from registers2 import *



class Global_Timers(Component):
    # Global timers for the controller


    def __init__(self):

        self.number_of_timers = 8

        super().__init__({
            "timer_pulse" : Out(8),

            "trigger": In(1),

            "bram_address": Out(16),
            "bram_write_data": Out(32),
            "bram_read_data": In(32),
            "bram_write_enable": Out(1)
        })

        # setup registers for DMA access
        driver_settings = {
            "clock_frequency": 25e6
        }
        self.rm = RegisterMapGenerator("global_timers", ["global_timers"], driver_settings, "Global timers for triggering events in the fpga")

        self.rm.add(Register("counter", rw="w", type="unsigned", width=32, desc="Value that the timer counts down from before triggering", bank_size=self.number_of_timers))
        self.rm.generate()

    def elaborate(self, platform):
        m = Module()

        self.current_counts = Array(Signal(name=f"current_count_{_}", shape=16) for _ in range(self.number_of_timers))
        self.reset_counts = Array(Signal(name=f"reset_count_{_}", shape=16, reset=10) for _ in range(self.number_of_timers))

        # with m.Switch(self.bram_address):
        #     for i in range(self.number_of_timers):
        #         with m.Case(i):
        #             with m.If(self.bram_write_enable):
        #                 m.d.sync_100 += self.reset_counts[i].eq(self.bram_write_data)

        with m.If(self.trigger):    # trigger input resets first timer
                m.d.sync_25 += self.current_counts[0].eq(self.reset_counts[0])

        for i in range(self.number_of_timers):
            
            with m.Elif(self.current_counts[i] != 0):
                m.d.sync_25 += self.current_counts[i].eq(self.current_counts[i] - 1)

            with m.If(self.current_counts[i] == 1):     # trigger output when timer reaches 1, will create a pulse 1 clock cycle long
                m.d.comb += self.timer_pulse[i].eq(1)
                if i < self.number_of_timers - 1:
                    m.d.sync_25 += self.current_counts[i+1].eq(self.reset_counts[i+1])    # start the next timer

        return m



dut = Global_Timers()
async def bench(ctx):
    # ctx.set(dut.bram_address, 0)
    # ctx.set(dut.bram_write_data, 20)
    # ctx.set(dut.bram_write_enable, 1)
    # await ctx.tick("sync_100")
    # ctx.set(dut.bram_write_enable, 0)


    await ctx.tick("sync_25").repeat(10)
    ctx.set(dut.trigger, 1)
    await ctx.tick("sync_25")
    ctx.set(dut.trigger, 0)

    await ctx.tick("sync_25").repeat(100)
    


if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/25e6, domain="sync_25")
    #sim.add_clock(1/100e6, domain="sync_100")
    sim.add_testbench(bench)
    with sim.write_vcd("global_timers_test.vcd"):
        sim.run()
