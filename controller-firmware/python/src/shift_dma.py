from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog
from amaranth.lib import wiring
from amaranth.lib.wiring import In, Out
from amaranth.lib.memory import Memory
from testing_block import test_block
from enum import IntEnum
import csv


class shift_dma_node(wiring.Component):
    """
    Node to connect a RTL module to the data loop

    Requires connections to a BRAM port (or matching interface) that the RTL module must support
    """

    # # support up to 256 nodes
    # read_node_address_input: In(8, init=0)   # type: ignore
    # read_node_address_output: Out(8, init=0) # type: ignore
    # write_node_address_input: In(8, init=0)   # type: ignore
    # write_node_address_output: Out(8, init=0) # type: ignore

    # # 65536 BRAM address space
    # read_bram_address_input: In(16, init=0)   # type: ignore
    # read_bram_address_output: Out(16, init=0)   # type: ignore

    # write_bram_address_input: In(16, init=0)   # type: ignore
    # write_bram_address_output: Out(16, init=0)   # type: ignore

    # # 32 bit data
    # data_input: In(32, init=0)   # type: ignore
    # data_output: Out(32, init=0)   # type: ignore

    # # read complete flag
    # read_complete_input: In(1, init=0)   # type: ignore
    # read_complete_output: Out(1, init=0)   # type: ignore

    # # write complete flag
    # write_complete_input: In(1, init=0)   # type: ignore
    # write_complete_output: Out(1, init=0)   # type: ignore


    # # bram ports
    # bram_address: Out(16, init=0)   # type: ignore
    # bram_write_data: Out(32, init=0)   # type: ignore
    # bram_read_data: In(32, init=0)   # type: ignore
    # bram_write_enable: Out(1, init=0)   # type: ignore

    def __init__(self, address):
        super().__init__({
            "read_node_address_input": In(8),
            "read_node_address_output": Out(8),
            "write_node_address_input": In(8),
            "write_node_address_output": Out(8),
            "read_bram_address_input": In(16),
            "read_bram_address_output": Out(16),
            "write_bram_address_input": In(16),
            "write_bram_address_output": Out(16),
            "data_input": In(32),
            "data_output": Out(32),
            "read_complete_input": In(1),
            "read_complete_output": Out(1),
            "write_complete_input": In(1),
            "write_complete_output": Out(1),
            
            "bram_address": Out(16),
            "bram_write_data": Out(32),
            "bram_read_data": In(32),
            "bram_write_enable": Out(1)
        })

        self.address = address

       
    def elaborate(self, platform):
        m = Module()
        #m.domains.sync_200 = self.sync_200 = ClockDomain("sync_200", async_reset=True)

        # 2 stage shift register to allow time for bram read/write

        self.buf_write_bram_address = Signal(16)
        self.buf_read_bram_address = Signal(16)
        self.buf_write_node_address = Signal(8)
        self.buf_read_node_address = Signal(8)
        self.buf_data = Signal(32)
        self.buf_read_complete = Signal(1)
        self.buf_write_complete = Signal(1)

        self.read_next = Signal(1)

        # these signals are never modified by nodes so they pass right through, shift to buffer, then to output
        m.d.sync_100 += self.buf_write_bram_address.eq(self.write_bram_address_input)
        m.d.sync_100 += self.buf_read_bram_address.eq(self.read_bram_address_input)
        m.d.sync_100 += self.buf_write_node_address.eq(self.write_node_address_input)
        m.d.sync_100 += self.buf_read_node_address.eq(self.read_node_address_input)

        m.d.sync_100 += self.write_bram_address_output.eq(self.buf_write_bram_address)
        m.d.sync_100 += self.read_bram_address_output.eq(self.buf_read_bram_address)
        m.d.sync_100 += self.write_node_address_output.eq(self.buf_write_node_address)
        m.d.sync_100 += self.read_node_address_output.eq(self.buf_read_node_address)

        m.d.sync_100 += self.buf_data.eq(self.data_input)
        m.d.sync_100 += self.buf_read_complete.eq(self.read_complete_input)
        #m.d.sync_100 += self.buf_write_complete.eq(self.write_complete_input)
        m.d.sync_100 += self.write_complete_output.eq(self.buf_write_complete)


        # if node matches first stage read address and it has not read yet, set specified bram address
        with m.If((self.read_node_address_input == self.address) & (self.read_complete_input == 0)):
            m.d.comb += self.bram_address.eq(self.read_bram_address_input)
            m.d.sync_100 += self.read_next.eq(1)
        with m.Elif(self.read_next):
            m.d.sync_100 += self.read_next.eq(0)

        with m.If(self.read_next):
            m.d.sync_100 += self.data_output.eq(self.bram_read_data)
            m.d.sync_100 += self.read_complete_output.eq(1)
            
        with m.Else():
            m.d.sync_100 += self.data_output.eq(self.buf_data)
            m.d.sync_100 += self.read_complete_output.eq(self.buf_read_complete)


        # if node matches first stage write address, has read, and has not written yet, set specified bram address, data, and write enable
        with m.If((self.write_node_address_input == self.address) & (self.read_complete_input) & (self.write_complete_input == 0)):
            m.d.comb += self.bram_address.eq(self.write_bram_address_input)
            m.d.comb += self.bram_write_data.eq(self.data_input)
            m.d.comb += self.bram_write_enable.eq(1)
            m.d.sync_100 += self.buf_write_complete.eq(1)
        with m.Else():
            m.d.comb += self.bram_write_enable.eq(0)
            m.d.sync_100 += self.buf_write_complete.eq(self.write_complete_input)


        return m

class shift_dma_controller(wiring.Component):
    """
    Controller for the shift DMA nodes

    This module is responsible for injecting data into the shift DMA nodes and reading the data back out
    """

    # TODO: fix system lockups when a valid source node is given but the destination node is invalid in COPY instructions

    def __init__(self, instruction_memory_depth=4096):
        self.instruction_memory_depth = instruction_memory_depth
        super().__init__({
            "read_node_address_input": In(8),
            "read_node_address_output": Out(8),
            "write_node_address_input": In(8),
            "write_node_address_output": Out(8),
            "read_bram_address_input": In(16),
            "read_bram_address_output": Out(16),
            "write_bram_address_input": In(16),
            "write_bram_address_output": Out(16),
            "data_input": In(32),
            "data_output": Out(32),
            "read_complete_input": In(1),
            "read_complete_output": Out(1),
            "write_complete_input": In(1),
            "write_complete_output": Out(1),

            "start": In(1),
            "busy": Out(1),

            "instruction_memory_address": Out(16),
            "instruction_memory_read_data": In(64),

            "data_memory_address": Out(16),
            "data_memory_read_data": In(32),
            "data_memory_write_data": Out(32),
            "data_memory_write_enable": Out(1)
        })

    class Instruction(IntEnum):
        END = 0     # end of program
        NOP = 1     # no operation
        COPY = 2    # copy data from source to destination
       
    def elaborate(self, platform):
        m = Module()

        #m.domains.sync_200 = self.sync_200 = ClockDomain("sync_200", async_reset=True)

        self.source_node = Signal(8)
        self.destination_node = Signal(8)
        self.source_address = Signal(16)
        self.destination_address = Signal(16)
        self.instruction = Signal(4)

        self.opening_available = Signal(1)   # if there is an opening available to add a new instruction to the loop
        

        m.submodules.dma_node = self.dma_node = shift_dma_node(0)   # create a dma node which will be used to make the data memory accessible to the dma nodes
        #self.dma_node.sync_200 = self.sync_200
        # create instruction memory
        # bits:
        # 0-7: source node
        # 8-15: destination node
        # 16-31: source address
        # 32-47: destination address
        # 48-51: instruction
        # 52-63: not used
        #m.submodules.instruction_memory = self.instruction_memory = Memory(shape=unsigned(64), depth=(4096), init=[])   # about enough memory to use up an entire update period at 50% utilization (hopefully more than we'll ever need)
        #self.instruction_memory_read_port = self.instruction_memory.read_port()  # read is used only internally
        #self.instruction_memory_write_port = self.instruction_memory.write_port()   # write is used by the axi controller to configure the dma
        #self.current_instruction = self.instruction_memory_read_port.addr

        self.current_instruction = self.instruction_memory_address

        # create data memory
        # divided into 2 blocks to allow for simultaneous read and write from the axi bus
        # 64 bit to match the axi bus width, this requires some extra logic to handle the 32 bit data from the dma nodes
        # m.submodules.data_memory_read = self.data_memory = Memory(shape=unsigned(64), depth=(1024), init=[])
        # m.submodules.data_memory_write = self.data_memory = Memory(shape=unsigned(64), depth=(1024), init=[])

        # m.submodules.data_memory = self.data_memory = Memory(shape=unsigned(32), depth=(4096), init=[])   # about enough memory to use up an enture update period at 50% utilization (hopefully more than we'll ever need)
        # self.data_memory_read_port = self.data_memory.read_port()
        # self.data_memory_write_port = self.data_memory.write_port()
        # self.data_memory_read_port2 = self.data_memory.read_port()
        # self.data_memory_write_port2 = self.data_memory.write_port()
        

        # connect memory interfaces
        m.d.comb += self.source_node.eq(self.instruction_memory_read_data[0:8])
        m.d.comb += self.destination_node.eq(self.instruction_memory_read_data[8:16])
        m.d.comb += self.source_address.eq(self.instruction_memory_read_data[16:32])
        m.d.comb += self.destination_address.eq(self.instruction_memory_read_data[32:48])
        m.d.comb += self.instruction.eq(self.instruction_memory_read_data[48:52])

        m.d.comb += self.data_memory_address.eq(self.dma_node.bram_address)
        m.d.comb += self.data_memory_address.eq(self.dma_node.bram_address)
        m.d.comb += self.data_memory_write_data.eq(self.dma_node.bram_write_data)
        m.d.comb += self.data_memory_write_enable.eq(self.dma_node.bram_write_enable)
        m.d.comb += self.dma_node.bram_read_data.eq(self.data_memory_read_data)

        # link internal node to output signals
        m.d.comb += self.read_node_address_output.eq(self.dma_node.read_node_address_output)
        m.d.comb += self.write_node_address_output.eq(self.dma_node.write_node_address_output)
        m.d.comb += self.read_bram_address_output.eq(self.dma_node.read_bram_address_output)
        m.d.comb += self.write_bram_address_output.eq(self.dma_node.write_bram_address_output)
        m.d.comb += self.data_output.eq(self.dma_node.data_output)
        m.d.comb += self.read_complete_output.eq(self.dma_node.read_complete_output)
        m.d.comb += self.write_complete_output.eq(self.dma_node.write_complete_output)

        with m.If(self.write_complete_input | (~self.read_complete_input)):  # these cases mean that the current instruction has completed or is invalid, so we can safely replace it with a new one
            m.d.comb += self.opening_available.eq(1)
        with m.Else():
            m.d.comb += self.opening_available.eq(0)
            m.d.sync_100 += self.dma_node.read_node_address_input.eq(self.read_node_address_input)
            m.d.sync_100 += self.dma_node.write_node_address_input.eq(self.write_node_address_input)
            m.d.sync_100 += self.dma_node.read_bram_address_input.eq(self.read_bram_address_input)
            m.d.sync_100 += self.dma_node.write_bram_address_input.eq(self.write_bram_address_input)
            m.d.sync_100 += self.dma_node.data_input.eq(self.data_input)
            m.d.sync_100 += self.dma_node.read_complete_input.eq(self.read_complete_input)
            m.d.sync_100 += self.dma_node.write_complete_input.eq(self.write_complete_input)
        

        with m.If(((self.instruction != self.Instruction.END) & (self.current_instruction != self.instruction_memory_depth-1)) & (self.start | self.busy)):
            m.d.sync_100 += self.busy.eq(1)

            with m.If((self.instruction == self.Instruction.COPY)):
                with m.If(self.opening_available):
                    # feed data into the internal node
                    m.d.sync_100 += self.dma_node.read_node_address_input.eq(self.source_node)
                    m.d.sync_100 += self.dma_node.write_node_address_input.eq(self.destination_node)
                    m.d.sync_100 += self.dma_node.read_bram_address_input.eq(self.source_address)
                    m.d.sync_100 += self.dma_node.write_bram_address_input.eq(self.destination_address)
                    m.d.sync_100 += self.dma_node.data_input.eq(0)
                    m.d.sync_100 += self.dma_node.read_complete_input.eq(0)
                    m.d.sync_100 += self.dma_node.write_complete_input.eq(0)
                    
                    # increment the current instruction pointer
                    m.d.sync_100 += self.current_instruction.eq(self.current_instruction + 1)


            with m.Elif(self.instruction == self.Instruction.NOP): 
                # increment the current instruction pointer
                m.d.sync_100 += self.current_instruction.eq(self.current_instruction + 1)
                with m.If(self.opening_available):
                    # reset the data to all zero with complete flags set, this will end up doing nothing
                    m.d.sync_100 += self.dma_node.read_node_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.write_node_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.read_bram_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.write_bram_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.data_input.eq(0)
                    m.d.sync_100 += self.dma_node.read_complete_input.eq(1)
                    m.d.sync_100 += self.dma_node.write_complete_input.eq(1)
            

            with m.Else(): # this should never occur as it means an unknown instruction, but we will just treat it as a NOP to prevent the system from hanging
                # increment the current instruction pointer
                m.d.sync_100 += self.current_instruction.eq(self.current_instruction + 1)
                with m.If(self.opening_available):
                    # reset the data to all zero with complete flags set, this will end up doing nothing
                    m.d.sync_100 += self.dma_node.read_node_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.write_node_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.read_bram_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.write_bram_address_input.eq(0)
                    m.d.sync_100 += self.dma_node.data_input.eq(0)
                    m.d.sync_100 += self.dma_node.read_complete_input.eq(1)
                    m.d.sync_100 += self.dma_node.write_complete_input.eq(1)

        with m.Else():
            m.d.sync_100 += self.busy.eq(0)
            with m.If(self.start):
                m.d.sync_100 += self.current_instruction.eq(0)

            with m.If(self.opening_available):
                # reset the data to all zero with complete flags set, this will end up doing nothing
                m.d.sync_100 += self.dma_node.read_node_address_input.eq(0)
                m.d.sync_100 += self.dma_node.write_node_address_input.eq(0)
                m.d.sync_100 += self.dma_node.read_bram_address_input.eq(0)
                m.d.sync_100 += self.dma_node.write_bram_address_input.eq(0)
                m.d.sync_100 += self.dma_node.data_input.eq(0)
                m.d.sync_100 += self.dma_node.read_complete_input.eq(1)
                m.d.sync_100 += self.dma_node.write_complete_input.eq(1)

        return m
    
class test_bench(wiring.Component):
    """
    Test bench for the shift DMA controller
    """
    # start: In(1, init=0)   # type: ignore
    # busy: Out(1, init=0)   # type: ignore

    def __init__(self, clock, node_count):
        super().__init__({
            "start": In(1),
            "busy": Out(1)
        })
        self.clock = clock
        self.node_count = node_count
        self.node_mem = {}
        self.nodes = {}


    def elaborate(self, platform):
        m = Module()

        #m.domains.sync_200 = ClockDomain("sync_200", async_reset=True)

        m.submodules.controller = self.controller = shift_dma_controller()
        m.d.comb += self.controller.start.eq(self.start)
        m.d.comb += self.busy.eq(self.controller.busy)

        for node_index in range(self.node_count):
            m.submodules[f"node_{node_index+1}"] = node = shift_dma_node(node_index+1)
            self.nodes[f"node_{node_index+1}"] = node
            m.submodules[f"node_test_block_{node_index+1}"] = test_block_ = test_block(self.clock)
            self.node_mem[f"node_test_block_{node_index+1}"] = test_block_
            m.d.comb += node.bram_read_data.eq(test_block_.read_data)
            m.d.comb += test_block_.write_data.eq(node.bram_write_data)
            m.d.comb += test_block_.write_enable.eq(node.bram_write_enable)
            m.d.comb += test_block_.address.eq(node.bram_address)
            if node_index == 0:
                m.d.comb += node.read_node_address_input.eq(self.controller.read_node_address_output)
                m.d.comb += node.write_node_address_input.eq(self.controller.write_node_address_output)
                m.d.comb += node.read_bram_address_input.eq(self.controller.read_bram_address_output)
                m.d.comb += node.write_bram_address_input.eq(self.controller.write_bram_address_output)
                m.d.comb += node.data_input.eq(self.controller.data_output)
                m.d.comb += node.read_complete_input.eq(self.controller.read_complete_output)
                m.d.comb += node.write_complete_input.eq(self.controller.write_complete_output)
            else:
                m.d.comb += node.read_node_address_input.eq(m.submodules[f"node_{node_index}"].read_node_address_output)
                m.d.comb += node.write_node_address_input.eq(m.submodules[f"node_{node_index}"].write_node_address_output)
                m.d.comb += node.read_bram_address_input.eq(m.submodules[f"node_{node_index}"].read_bram_address_output)
                m.d.comb += node.write_bram_address_input.eq(m.submodules[f"node_{node_index}"].write_bram_address_output)
                m.d.comb += node.data_input.eq(m.submodules[f"node_{node_index}"].data_output)
                m.d.comb += node.read_complete_input.eq(m.submodules[f"node_{node_index}"].read_complete_output)
                m.d.comb += node.write_complete_input.eq(m.submodules[f"node_{node_index}"].write_complete_output)
        node = m.submodules[f"node_{self.node_count}"]
        m.d.comb += self.controller.read_node_address_input.eq(node.read_node_address_output)
        m.d.comb += self.controller.write_node_address_input.eq(node.write_node_address_output)
        m.d.comb += self.controller.read_bram_address_input.eq(node.read_bram_address_output)
        m.d.comb += self.controller.write_bram_address_input.eq(node.write_bram_address_output)
        m.d.comb += self.controller.data_input.eq(node.data_output)
        m.d.comb += self.controller.read_complete_input.eq(node.read_complete_output)
        m.d.comb += self.controller.write_complete_input.eq(node.write_complete_output)

        return m
    
node_count = 4
clock = int(200e6) # 200 Mhz
dut = test_bench(clock, node_count)
dut = shift_dma_controller()
sim = Simulator(dut)

async def test_bench(ctx):
    # copy values from internal mem to all external nodes
    # for index in range(node_count):
    #     source_node = 0
    #     destination_node = index
    #     source_address = index
    #     destination_address = 0
    #     instruction = dut.controller.Instruction.COPY
    #     data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    #     ctx.set(dut.controller.instruction_memory.data[index], data)
    #     ctx.set(dut.controller.data_memory.data[index], index+1)
    #     print(f"set instruction {index} to {data}")

    # # copy values from all external nodes to internal mem
    # for index in range(node_count):
    #     source_node = node_count + 1
    #     destination_node = 0
    #     source_address = 0
    #     destination_address = index + node_count + 1
    #     instruction = dut.controller.Instruction.COPY
    #     data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    #     ctx.set(dut.controller.instruction_memory.data[index+node_count], data)
    #     print(f"set instruction {index+node_count} to {data}")

    instruction_step = 0

    # nops to get the system to a known state (probably not actually required)
    for i in range(1):
        source_node = 0
        destination_node = 0
        source_address = 0
        destination_address = 0
        instruction = dut.controller.Instruction.NOP
        data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
        ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
        instruction_step += 1


    ctx.set(dut.controller.data_memory.data[1], 1)
    # ctx.set(dut.controller.data_memory.data[2], 2)
    # ctx.set(dut.controller.data_memory.data[3], 3)
    # ctx.set(dut.controller.data_memory.data[4], 4)
    # ctx.set(dut.controller.data_memory.data[5], 5)
    # ctx.set(dut.controller.data_memory.data[6], 6)
    #ctx.set(dut.node_mem["node_test_block_2"].memory.data[1], 8)

    # forward copy
    # copy value from controller mem 1 to node 2-1
    source_node = 0
    destination_node = 2
    source_address = 1
    destination_address = 1
    instruction = dut.controller.Instruction.COPY
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    instruction_step += 1

    # for i in range(node_count+4):
    #     source_node = 0
    #     destination_node = 0
    #     source_address = 0
    #     destination_address = 0
    #     instruction = dut.controller.Instruction.NOP
    #     data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    #     ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    #     instruction_step += 1

    # forward copy
    # copy value from node 2-1 to node 4-1
    source_node = 2
    destination_node = 4
    source_address = 1
    destination_address = 1
    instruction = dut.controller.Instruction.COPY
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    instruction_step += 1

    # for i in range(node_count+4):
    #     source_node = 0
    #     destination_node = 0
    #     source_address = 0
    #     destination_address = 0
    #     instruction = dut.controller.Instruction.NOP
    #     data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    #     ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    #     instruction_step += 1
    
    # self copy
    # copy value from node 4-1 to node 4-2
    source_node = 4
    destination_node = 4
    source_address = 1
    destination_address = 2
    instruction = dut.controller.Instruction.COPY
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    instruction_step += 1

    for i in range(node_count*2+3): # nops required to allow the previous copy to complete since it loops around
        source_node = 0
        destination_node = 0
        source_address = 0
        destination_address = 0
        instruction = dut.controller.Instruction.NOP
        data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
        ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
        instruction_step += 1
    
    # # reverse copy
    # # copy value from node 4-1 to node 3-2
    source_node = 4
    destination_node = 3
    source_address = 2
    destination_address = 2
    instruction = dut.controller.Instruction.COPY
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    instruction_step += 1

    for i in range(node_count*2+3):
        source_node = 0
        destination_node = 0
        source_address = 0
        destination_address = 0
        instruction = dut.controller.Instruction.NOP
        data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
        ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
        instruction_step += 1

    # # copy to controller mem
    # # copy value from node 3-2 to controller mem 2
    source_node = 3
    destination_node = 0
    source_address = 2
    destination_address = 2
    instruction = dut.controller.Instruction.COPY
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    instruction_step += 1

    source_node = 3
    destination_node = 0
    source_address = 2
    destination_address = 2
    instruction = dut.controller.Instruction.END
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    ctx.set(dut.controller.instruction_memory.data[instruction_step], data)
    instruction_step += 1

    #await ctx.tick().repeat(2)
    ctx.set(dut.start, 1)
    await ctx.tick()
    ctx.set(dut.start, 0)

    results = []

    for n in range(100):
        await ctx.tick()
        data = []
        node = []
        node.append(ctx.get(dut.controller.read_node_address_output))
        node.append(ctx.get(dut.controller.write_node_address_output))
        node.append(ctx.get(dut.controller.read_bram_address_output))
        node.append(ctx.get(dut.controller.write_bram_address_output))
        node.append(ctx.get(dut.controller.data_output))
        node.append(ctx.get(dut.controller.read_complete_output))
        node.append(ctx.get(dut.controller.write_complete_output))
        data.append(node)
        print(node)


        for i in range(node_count):
            node = []
            node.append(ctx.get(dut.nodes[f"node_{i+1}"].read_node_address_output))
            node.append(ctx.get(dut.nodes[f"node_{i+1}"].write_node_address_output))
            node.append(ctx.get(dut.nodes[f"node_{i+1}"].read_bram_address_output))
            node.append(ctx.get(dut.nodes[f"node_{i+1}"].write_bram_address_output))
            node.append(ctx.get(dut.nodes[f"node_{i+1}"].data_output))
            node.append(ctx.get(dut.nodes[f"node_{i+1}"].read_complete_output))
            node.append(ctx.get(dut.nodes[f"node_{i+1}"].write_complete_output))
            data.append(node)

        line = []
        for i in data:
            for j in i:
                line.append(j)
        results.append(line)
    print(results[0])
    with open('results.csv', 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        for cycle, line in enumerate(results):
            csvwriter.writerow([cycle] + line)

    if(ctx.get(dut.busy)):
        print(f"FAILED: controller did not finish in time")

    if(not (ctx.get(dut.controller.data_memory.data[1]) == ctx.get(dut.node_mem["node_test_block_2"].memory.data[1]) == 1)):
        print(f"FAILED: forward copy failed (0-1 to 2-1)")
    else:
        print(f"PASS: forward copy passed (0-1 to 2-1)")

    if(not (ctx.get(dut.node_mem["node_test_block_2"].memory.data[1]) == ctx.get(dut.node_mem["node_test_block_4"].memory.data[1]) == 1)):
        print(f"FAILED: forward copy failed (2-1 to 4-1)")
    else:
        print(f"PASS: forward copy passed (2-1 to 4-1)")

    if(not (ctx.get(dut.node_mem["node_test_block_4"].memory.data[1]) == ctx.get(dut.node_mem["node_test_block_4"].memory.data[2]) == 1)):
        print(f"FAILED: self copy failed (4-1 to 4-2)")
    else:
        print(f"PASS: self copy passed (4-1 to 4-2)")

    if(not (ctx.get(dut.node_mem["node_test_block_4"].memory.data[2]) == ctx.get(dut.node_mem["node_test_block_3"].memory.data[2]) == 1)):
        print(f"FAILED: reverse copy failed (4-2 to 3-2)")
    else:
        print(f"PASS: reverse copy passed (4-2 to 3-2)")

    if(not (ctx.get(dut.node_mem["node_test_block_3"].memory.data[2]) == ctx.get(dut.controller.data_memory.data[2]) == 1)):
        print(f"FAILED: copy to controller mem failed (3-2 to 0-2)")
    else:
        print(f"PASS: copy to controller mem passed (3-2 to 0-2)")





if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/clock)
    sim.add_testbench(test_bench)
    with sim.write_vcd("shift_dma_test.vcd"):
        sim.run()

    if (False):  # export
        top = shift_dma_node(100e6, 0)
        with open("S:/Vivado/autogen_sources/shift_dma_node.v", "w") as f:
            f.write(verilog.convert(top, name="shift_dma_node", ports=top.ports))