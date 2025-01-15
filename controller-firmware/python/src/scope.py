from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from enum import IntEnum, auto
from amaranth.back import verilog

from registers import RTL_Block, Register, RegisterDataType, ReadWritePermissions


class scope(Elaboratable):
    """
    Scope for debugging signals
    """

    """
    Registers (32 bit):

    
    """

    def __init__(self, clock:int, mem_depth:int) -> None:
        """
        clock: input clock frequency (Hz)

        mem_depth: depth of capture memory in 32 bit words
        """
        
        self.clock = clock
        self.memDepth = mem_depth

        self.txStartBits = 1
        self.rxStartBits = 1
        self.txStopBits = 1
        self.rxStopBits = 1

        # Ports
        self.address = Signal(16)
        self.writeData = Signal(32)
        self.readData = Signal(32)
        self.writeEnable = Signal()

        self.triggers = Signal(16)

        # IO pins
        self.input = Signal(32)
        self.triggerIn = Signal()
        self.triggerOut = Signal()


        self.regs = RTL_Block("scope")

        self.regs.addRegister(Register("control", RegisterDataType.PACKED, ReadWritePermissions.READ_WRITE, "control register"))

        self.regs.packInto("control", Register("start", RegisterDataType.BOOL, ReadWritePermissions.READ_WRITE, "run scope until triggered or stopped"))
        self.regs.packInto("control", Register("stop", RegisterDataType.BOOL, ReadWritePermissions.READ_WRITE, "stop scope"))
        self.regs.packInto("control", Register("single_mode", RegisterDataType.BOOL, ReadWritePermissions.READ_WRITE, "capture until triggered then stop (start bit needs cycled to start again)"))
        self.regs.packInto("control", Register("trigger_enable", RegisterDataType.BOOL, ReadWritePermissions.READ_WRITE, "enable trigger"))
        #self.regs.packInto("control", Register("enable_2bit_packing", RegisterDataType.BOOL, ReadWritePermissions.READ_WRITE, "pack 2 bits into 32 bit words for higher memory efficiency"))

        self.regs.addRegister(Register("status", RegisterDataType.PACKED, ReadWritePermissions.READ, "status register"))
        self.regs.packInto("status", Register("running", RegisterDataType.BOOL, ReadWritePermissions.READ, "scope is running"))
        self.regs.packInto("status", Register("triggered", RegisterDataType.BOOL, ReadWritePermissions.READ, "scope was triggered"))


        self.regs.addRegister(Register("sample_divider", RegisterDataType.UNSIGNED, ReadWritePermissions.READ_WRITE, "divider for generating sample triggers from input clock", width=16))

        self.regs.addRegister(Register("trigger_level", RegisterDataType.UNSIGNED, ReadWritePermissions.READ_WRITE, "trigger level", width=32))

        self.regs.addRegister(Register("trigger_time_offset", RegisterDataType.SIGNED, ReadWritePermissions.READ_WRITE, "where to start and stop the capture when triggered. 0 is 50/50 before/after, .5*mem_depth is 100/0, -.5*mem_depth is 0/100", width=32))

        self.regs.addRegister(Register("trigger_index", RegisterDataType.UNSIGNED, ReadWritePermissions.READ, "memory index of where the trigger occured", width=32))

        self.regs.addRegisterBank(Register("capture_memory", RegisterDataType.UNSIGNED, ReadWritePermissions.READ, "capture memory", width=32), count=self.memDepth)

        self.regs.generateAddressMap()

        self.regs.exportDataJSON("scope.json")


    def elaborate(self, platform):
        m = Module()

        m.submodules.memory = self.memory = Memory(shape=unsigned(32), depth=(self.memDepth), init=[])   # store only capture data in bram


        self.externalReadPort = self.memory.read_port()
        #self.externalWritePort = self.memory.write_port()
        #self.internalReadPort = self.memory.read_port()
        self.internalWritePort = self.memory.write_port()

        # connect external memory interfaces
        m.d.comb += self.externalReadPort.addr.eq(self.address)
        #m.d.comb += self.externalWritePort.addr.eq(self.address)
        #m.d.comb += self.externalWritePort.data.eq(self.writeData)
        #m.d.comb += self.externalWritePort.en.eq(self.writeEnable)
        m.d.comb += self.readData.eq(self.externalReadPort.data)

        self.internalReadWriteAddress = Signal(16)
        #m.d.comb += self.internalReadPort.addr.eq(self.internalReadWriteAddress)
        m.d.comb += self.internalWritePort.addr.eq(self.internalReadWriteAddress)


        self.start = Signal()
        self.stop = Signal()
        self.single_mode = Signal()
        self.trigger_enable = Signal()

        
        with m.FSM(init="stop"):

            with m.State("stop"):
                with m.If(self.start):
                    m.next = "run"

            with m.State("run"):
                with m.If(self.regs.control.stop):
                    m.next = "IDLE"
                with m.If(self.regs.control.single_mode):
                    m.next = "IDLE"
                with m.If(self.regs.control.trigger_enable):
                    m.next = "TRIGGERED"
            with m.State("TRIGGERED"):
                with m.If(self.regs.control.stop):
                    m.next = "IDLE"

        return m

clock = int(100e6) # 100 Mhz
dut = scope(clock, 1024)

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