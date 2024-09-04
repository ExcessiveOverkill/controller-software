from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from amaranth.lib.crc.catalog import CRC32_MPEG_2
from enum import IntEnum, auto
import numpy as np
from amaranth.back import verilog
import math


class dma(Elaboratable):
    """
    DMA controller for moving data between all peripherals efficiently
    """

    """
    Registers (32 bit):
    """

        
    def __init__(self, clock:int, max_instructions:int, number_of_workers:int, number_of_peripherals:int, debug:bool = False) -> None:

        self.clock = clock
        self.maxInstructions = max_instructions
        self.numOfWorkers = number_of_workers
        self.numOfPeripherals = number_of_peripherals
        self.debug = debug

        assert self.numOfWorkers == 1, "More than one worker is not yet supported"



        self.ports = []

        # peripheral signals
        self.peripheralPorts = {}
        for i in range(self.numOfPeripherals):
            self.peripheralPorts[i] = {
                "address": Signal(16, name=f"peripheral_address_{i}"),
                "writeData": Signal(32, name=f"peripheral_writeData_{i}"),
                "readData": Signal(32, name=f"peripheral_readData_{i}"),
                "writeEnable": Signal(name=f"peripheral_writeEnable_{i}")
            }
            self.ports.append(self.peripheralPorts[i]["address"])
            self.ports.append(self.peripheralPorts[i]["writeData"])
            self.ports.append(self.peripheralPorts[i]["readData"])
            self.ports.append(self.peripheralPorts[i]["writeEnable"])

        self.configAddress = Signal(16)
        self.configWriteData = Signal(32)
        self.configReadData = Signal(32)
        self.configWriteEnable = Signal()

        self.ports.append(self.configAddress)
        self.ports.append(self.configWriteData)
        self.ports.append(self.configReadData)
        self.ports.append(self.configWriteEnable)

    class instructions(IntEnum):
        NONE = 0x0
        TRANSFER_DATA = 0x1
        WAIT_FOR_ACTIVE_BITS = 0x2  # any bits set in the TARGET register must also be set in data at the SOURCE address to move to the next instruction
        END_OF_PROGRAM = 0xF

    class configRegisters(IntEnum):
        TIMER = 0x0
        CURRENT_INSTRUCTION = 0x1
        STATUS = 0x2
        WORKER_STATUS = 0x3
        WORKER_SELECT = 0x4
        MEMORY_TYPE_SELECT = 0x5

    def elaborate(self, platform):
        m = Module()

        self.selectedWorker = Signal(range(self.numOfWorkers))
        self.selectedMemType = Signal(range(3))

        self.currentInstructionStep = Signal(range(self.maxInstructions))

        self.timerSetpoint = Signal(24)
        self.timer = Signal(24)

        self.triggerProgram = Signal()

        self.memories = {}

        with m.If(self.triggerProgram):
            m.d.sync += self.currentInstructionStep.eq(0)
            m.d.sync += self.triggerProgram.eq(0)

        self.workerDone = Signal(self.numOfWorkers)
        self.allowSkip = Signal(self.numOfWorkers)

        self.programDone = Signal()
        self.ports.append(self.programDone)
        self.programNotFinishedFault = Signal()
        self.ports.append(self.programNotFinishedFault)


        # handle configuration

        with m.If(self.configAddress == self.configRegisters.TIMER):
            m.d.sync += self.configReadData.eq(self.timerSetpoint)
            with m.If(self.configWriteEnable):
                m.d.sync += self.timerSetpoint.eq(self.configWriteData)

        with m.If(self.configAddress == self.configRegisters.CURRENT_INSTRUCTION):
            m.d.sync += self.configReadData.eq(self.currentInstructionStep)
            with m.If(self.configWriteEnable):
                m.d.sync += self.currentInstructionStep.eq(self.configWriteData)

        with m.If(self.configAddress == self.configRegisters.STATUS):
            m.d.sync += self.configReadData.eq(self.programDone)

        with m.If(self.configAddress == self.configRegisters.WORKER_STATUS):
            m.d.sync += self.configReadData.eq(self.workerDone)

        with m.If(self.configAddress == self.configRegisters.WORKER_SELECT):
            m.d.sync += self.configReadData.eq(self.selectedWorker)
            with m.If(self.configWriteEnable):
                m.d.sync += self.selectedWorker.eq(self.configWriteData)

        with m.If(self.configAddress == self.configRegisters.MEMORY_TYPE_SELECT):
            m.d.sync += self.configReadData.eq(self.selectedMemType)
            with m.If(self.configWriteEnable):
                m.d.sync += self.selectedMemType.eq(self.configWriteData)


        # program trigger timer
        with m.If((self.timerSetpoint != 0) & (self.timer == 0)):
            m.d.sync += self.timer.eq(self.timerSetpoint)
            m.d.sync += self.triggerProgram.eq(1)

        with m.If(self.timer != 0):
            m.d.sync += self.timer.eq(self.timer - 1)


        # main BRAM ports, the CPU can read/write directly to these
        self.address = Signal(16)
        self.writeData = Signal(32)
        self.readData = Signal(32)
        self.writeEnable = Signal()

        self.ports.append(self.address)
        self.ports.append(self.writeData)
        self.ports.append(self.readData)
        self.ports.append(self.writeEnable)

        # add memories for all peripherals if in debug mode
        if self.debug:
            self.debugMemories = {}
            for i in range(self.numOfPeripherals):
                self.debugMemories[i] = {}
                m.submodules[f"memory_debug_peripheral_{i}"] = self.debugMemories[i]["memory"] = Memory(shape=unsigned(32), depth=32, init=[])
                self.debugMemories[i]["read_port"] = self.debugMemories[i]["memory"].read_port()
                self.debugMemories[i]["write_port"] = self.debugMemories[i]["memory"].write_port()


        # add memories for each worker
        for i in range(self.numOfWorkers):
            self.memories[i] = {}
            m.submodules[f"memory_{i}_instructions"] = self.memories[i]["instructions"] = Memory(shape=unsigned(16), depth=self.maxInstructions, init=[])
            m.submodules[f"memory_{i}_sources"] = self.memories[i]["sources"] = Memory(shape=unsigned(32), depth=self.maxInstructions, init=[])
            m.submodules[f"memory_{i}_targets"] = self.memories[i]["targets"] = Memory(shape=unsigned(32), depth=self.maxInstructions, init=[])

            readReady = Signal(1, name=f"worker_{i}_read_ready")

            for type, memObj in enumerate(self.memories[i].values()):
                externalReadPort = memObj.read_port()
                externalWritePort = memObj.write_port()

                with m.If((self.selectedWorker == i) & (self.selectedMemType == type)):
                    m.d.comb += externalReadPort.addr.eq(self.address)
                    m.d.comb += externalWritePort.addr.eq(self.address)
                    m.d.comb += externalWritePort.data.eq(self.writeData)
                    m.d.comb += externalWritePort.en.eq(self.writeEnable)
                    m.d.comb += self.readData.eq(externalReadPort.data)

            instructionReadPort = self.memories[i]["instructions"].read_port()
            sourcesReadPort = self.memories[i]["sources"].read_port()
            targetsReadPort = self.memories[i]["targets"].read_port()

            m.d.comb += instructionReadPort.addr.eq(self.currentInstructionStep)
            m.d.comb += sourcesReadPort.addr.eq(self.currentInstructionStep)
            m.d.comb += targetsReadPort.addr.eq(self.currentInstructionStep)

            workerDataNode = Signal(32, name=f"data_node_{i}")

            with m.If((instructionReadPort.data == self.instructions.TRANSFER_DATA)):
                m.d.sync += self.allowSkip[i].eq(1)

            with m.If((instructionReadPort.data == self.instructions.TRANSFER_DATA) & readReady):
                m.d.sync += self.workerDone[i].eq(1)

            with m.If((instructionReadPort.data == self.instructions.WAIT_FOR_ACTIVE_BITS) & readReady):
                with m.If((~workerDataNode & targetsReadPort.data) == 0):
                    m.d.sync += self.workerDone[i].eq(1)
                with m.Else():
                    m.d.sync += self.workerDone[i].eq(0)

            with m.If(instructionReadPort.data == self.instructions.NONE):
                m.d.sync += self.workerDone[i].eq(1)
                m.d.sync += self.allowSkip[i].eq(1)

            with m.If(instructionReadPort.data == self.instructions.END_OF_PROGRAM):
                m.d.comb += self.programDone.eq(1)
                m.d.sync += self.workerDone[i].eq(1)
                #m.d.sync += self.allowSkip[i].eq(1)

            for index, peripheral in self.peripheralPorts.items():

                with m.If(self.triggerProgram):
                    m.d.comb += peripheral["writeEnable"].eq(0)

                with m.If(((instructionReadPort.data == self.instructions.TRANSFER_DATA) | (instructionReadPort.data == self.instructions.WAIT_FOR_ACTIVE_BITS)) & (index == sourcesReadPort.data.shift_right(16))):
                    m.d.comb += peripheral["address"].eq(sourcesReadPort.data.bit_select(0, 16))
                    m.d.sync += readReady.eq(1)

                with m.If(((instructionReadPort.data == self.instructions.TRANSFER_DATA) | (instructionReadPort.data == self.instructions.WAIT_FOR_ACTIVE_BITS)) & (index == sourcesReadPort.data.shift_right(16)) & readReady):
                    m.d.comb += workerDataNode.eq(peripheral["readData"])

                with m.If((instructionReadPort.data == self.instructions.TRANSFER_DATA) & (index == targetsReadPort.data.shift_right(16)) & readReady):
                    m.d.comb += peripheral["address"].eq(targetsReadPort.data.bit_select(0, 16))
                    m.d.comb += peripheral["writeData"].eq(workerDataNode)
                    m.d.comb += peripheral["writeEnable"].eq(1)

                with m.If(((instructionReadPort.data != self.instructions.TRANSFER_DATA) | (index != targetsReadPort.data.shift_right(16)))):
                    m.d.comb += peripheral["writeEnable"].eq(0)

                with m.If(self.workerDone[i]):
                    m.d.comb += peripheral["writeEnable"].eq(0)

                # link peripheral mem ports to signals for debugging
                if self.debug:
                    m.d.comb += self.debugMemories[index]["read_port"].addr.eq(peripheral["address"])
                    m.d.comb += self.debugMemories[index]["write_port"].addr.eq(peripheral["address"])
                    m.d.comb += peripheral["readData"].eq(self.debugMemories[index]["read_port"].data)
                    m.d.comb += self.debugMemories[index]["write_port"].data.eq(peripheral["writeData"])
                    m.d.comb += self.debugMemories[index]["write_port"].en.eq(peripheral["writeEnable"])

        with m.If(self.workerDone.all() & (self.programDone == 0)):
            with m.If(self.allowSkip.all()):
                m.d.sync += self.currentInstructionStep.eq(self.currentInstructionStep + 1)
                m.d.sync += self.workerDone.eq(0)
                m.d.sync += self.allowSkip.eq(0)
            with m.Else():
                m.d.sync += self.allowSkip.eq(0xFF)

        with m.If(self.triggerProgram):
            m.d.sync += self.currentInstructionStep.eq(0)
            m.d.sync += self.allowSkip.eq(0)
            m.d.sync += self.workerDone.eq(0)

        with m.If(self.triggerProgram & (self.programDone == 0)):
            m.d.sync += self.programNotFinishedFault.eq(1)

        with m.If(self.triggerProgram & self.programNotFinishedFault):
            m.d.sync += self.programNotFinishedFault.eq(0)


        return m
    

clock = int(100e6)
dut = dma(clock, 64, 1, 20, True)

# TODO: fix/add multiple worker support

async def dmaBench(ctx):

    # move data through all peripherals and make sure it moves correctly (single worker)
    instruction = 0
    for p in range(dut.numOfPeripherals):
        instruction += 1
        ctx.set(dut.debugMemories[0]["memory"].data[p], p+1)

        ctx.set(dut.memories[0]["instructions"].data[instruction], dut.instructions.TRANSFER_DATA)
        ctx.set(dut.memories[0]["sources"].data[instruction], p+1 | 0<<16)
        ctx.set(dut.memories[0]["targets"].data[instruction], 0x1 | (p+1)<<16)

    ctx.set(dut.memories[0]["instructions"].data[instruction], dut.instructions.END_OF_PROGRAM)

    while(ctx.get(dut.programDone) == 0):   # wait for program to finish
        await ctx.tick()

    # verify data was transfered correctly
    for p in range(dut.numOfPeripherals-1):
        assert p+2 == ctx.get(dut.debugMemories[p+1]["memory"].data[1]), "\n\nSingle worker scatter transfer test failed\n\n"

    print("Single worker scatter transfer test passed")


    # # move data through all peripherals and make sure it moves correctly (all workers)
    # instruction = 0
    # for worker in range(dut.numOfWorkers):
    #     for p in range(dut.numOfPeripherals):
    #         instruction += 1
    #         ctx.set(dut.debugMemories[worker]["memory"].data[p], (p+1)*(worker+1))

    #         ctx.set(dut.memories[worker]["instructions"].data[instruction], dut.instructions.TRANSFER_DATA)
    #         ctx.set(dut.memories[worker]["sources"].data[instruction], p+1 | worker<<16)
    #         ctx.set(dut.memories[worker]["targets"].data[instruction], 0x1 | (p+1+dut.numOfWorkers)<<16)

    #     ctx.set(dut.memories[worker]["instructions"].data[instruction], dut.instructions.END_OF_PROGRAM)

    # ctx.set(dut.triggerProgram, 1)
    # await ctx.tick()
    # ctx.set(dut.triggerProgram, 0)
    # await ctx.tick()

    # while(ctx.get(dut.programDone) == 0):   # wait for program to finish
    #     await ctx.tick()

    # # verify data was transfered correctly
    # for p in range(dut.numOfPeripherals-1):
    #     assert (p+2 == ctx.get(dut.debugMemories[p+1]["memory"].data[1]))

    # print("Multiple worker scatter transfer test passed")



    # move data through all peripherals and make sure it moves correctly with no-ops(single worker)
    instruction = 0
    for p in range(dut.numOfPeripherals):
        
        ctx.set(dut.debugMemories[0]["memory"].data[p], p+1)

        instruction += 1
        ctx.set(dut.memories[0]["instructions"].data[instruction], dut.instructions.NONE)

        instruction += 1
        ctx.set(dut.memories[0]["instructions"].data[instruction], dut.instructions.TRANSFER_DATA)
        ctx.set(dut.memories[0]["sources"].data[instruction], p+1 | 0<<16)
        ctx.set(dut.memories[0]["targets"].data[instruction], 0x1 | (p+1)<<16)

    ctx.set(dut.memories[0]["instructions"].data[instruction], dut.instructions.END_OF_PROGRAM)

    ctx.set(dut.triggerProgram, 1)
    await ctx.tick()
    ctx.set(dut.triggerProgram, 0)
    await ctx.tick()

    while(ctx.get(dut.programDone) == 0):   # wait for program to finish
        await ctx.tick()

    # verify data was transfered correctly
    for p in range(dut.numOfPeripherals-1):
        assert p+2 == ctx.get(dut.debugMemories[p+1]["memory"].data[1]), "\n\nSingle worker scatter transfer + no-ops test failed\n\n"

    print("Single worker scatter transfer with no-ops test passed")


    # wait for active bits test

    ctx.set(dut.debugMemories[0]["memory"].data[0], 0x0)

    ctx.set(dut.memories[0]["instructions"].data[0], dut.instructions.WAIT_FOR_ACTIVE_BITS)
    ctx.set(dut.memories[0]["sources"].data[0], 0x0 | 0<<16)
    ctx.set(dut.memories[0]["targets"].data[0], 0b1100)

    ctx.set(dut.memories[0]["instructions"].data[1], dut.instructions.END_OF_PROGRAM)

    ctx.set(dut.triggerProgram, 1)
    await ctx.tick()
    ctx.set(dut.triggerProgram, 0)
    await ctx.tick()

    await ctx.tick().repeat(10)
    assert ctx.get(dut.programDone) == 0, "Wait for active bits test failed, passed while bits not active"
    
    ctx.set(dut.debugMemories[0]["memory"].data[0], 0b1000)

    await ctx.tick().repeat(10)
    assert ctx.get(dut.programDone) == 0, "Wait for active bits test failed, passed with only some bits active"

    ctx.set(dut.debugMemories[0]["memory"].data[0], 0b1100)

    await ctx.tick().repeat(10)
    assert ctx.get(dut.programDone) == 1, "Wait for active bits test failed, did not pass with all bits active"

    print("Single worker wait for active bits test passed")


    # test timer
    ctx.set(dut.debugMemories[0]["memory"].data[0], 0x1)
    ctx.set(dut.debugMemories[1]["memory"].data[0], 0x0)

    ctx.set(dut.memories[0]["instructions"].data[0], dut.instructions.TRANSFER_DATA)
    ctx.set(dut.memories[0]["sources"].data[0], 0x0 | 0<<16)
    ctx.set(dut.memories[0]["targets"].data[0], 0x0 | 1<<16)

    ctx.set(dut.memories[0]["instructions"].data[1], dut.instructions.END_OF_PROGRAM)

    ctx.set(dut.timerSetpoint, 20)

    for i in range(5):
        await ctx.tick().repeat(30)
        assert ctx.get(dut.debugMemories[0]["memory"].data[0]) == ctx.get(dut.debugMemories[1]["memory"].data[0]), "timer test failed"
        ctx.set(dut.debugMemories[0]["memory"].data[0], i+2)

    print("timer test passed")

            
if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock)
    sim.add_testbench(dmaBench)
    with sim.write_vcd("dma.vcd"):
        sim.run()

    if (True):  # export
        top = dma(int(100e6), 100, 1, 5, False)
        with open("controller-firmware/src/amaranth sources/dma.v", "w") as f:
            f.write(verilog.convert(top, name="dma", ports=top.ports))