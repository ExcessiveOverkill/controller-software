from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog
from amaranth.lib import wiring
from amaranth.lib.wiring import In, Out
from shift_dma import shift_dma_node
from registers import *


class gpio_node(wiring.Component):

    def __init__(self) -> None:
        super().__init__({
            "gpio_out": Out(32),
            "gpio_out_enable": Out(32),
            "gpio_in": In(32),
        })

        self.desc = RTL_Block("gpio_node")
        self.desc.addCompatibleDriver("gpio_node")
        self.address = 255

        self.desc.addRegister(Register("gpio_out", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "GPIO out register", width=32))
        self.desc.addRegister(Register("gpio_out_enable", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "GPIO out enable register", width=32))
        self.desc.addRegister(Register("gpio_in", RegisterDataType.UNSIGNED, ReadWritePermissions.READ, "GPIO in register", width=32))

        self.desc.generateAddressMap()

    def elaborate(self, platform):
        m = Module()

        m.submodules.shift_dma = self.shift_dma = shift_dma_node(self.address)

        with m.If(self.shift_dma.bram_address == self.desc.getRegistor("gpio_out").address):
            m.d.sync += self.shift_dma.bram_read_data.eq(self.gpio_out)
            with m.If(self.shift_dma.bram_write_enable == 1):
                m.d.sync += self.gpio_out.eq(self.shift_dma.bram_write_data)

        with m.If(self.shift_dma.bram_address == self.desc.getRegistor("gpio_out_enable").address):
            m.d.sync += self.shift_dma.bram_read_data.eq(self.gpio_out_enable)
            with m.If(self.shift_dma.bram_write_enable == 1):
                m.d.sync += self.gpio_out_enable.eq(self.shift_dma.bram_write_data)

        with m.If(self.shift_dma.bram_address == self.desc.getRegistor("gpio_in").address):
            m.d.sync += self.shift_dma.bram_read_data.eq(self.gpio_in)
        
        return m
    