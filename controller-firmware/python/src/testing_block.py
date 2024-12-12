from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from amaranth.back import verilog
from amaranth.lib.wiring import In, Out
from amaranth.lib import wiring


# minimal rtl block for testing, just some bram


class test_block(wiring.Component):

    def __init__(self, clock) -> None:
        super().__init__({
            "write_data": In(32),
            "write_enable": In(1),
            "address": In(16),
            "read_data": Out(32)
        })
        self.clock = clock
    
    def elaborate(self, platform):
        m = Module()

        m.submodules.memory = self.memory = Memory(shape=unsigned(32), depth=256, init=[])   # small memory for testing

        self.externalReadPort = self.memory.read_port()
        self.externalWritePort = self.memory.write_port()

        # connect external memory interfaces
        m.d.comb += self.externalReadPort.addr.eq(self.address)
        m.d.comb += self.externalWritePort.addr.eq(self.address)
        m.d.comb += self.externalWritePort.data.eq(self.write_data)
        m.d.comb += self.externalWritePort.en.eq(self.write_enable)
        m.d.comb += self.read_data.eq(self.externalReadPort.data)

        return m
