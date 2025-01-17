from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.memory import Memory
from amaranth.back import verilog
from amaranth.lib.wiring import In, Out
from amaranth.lib import wiring


# small i2c interface


class i2c(wiring.Component):

    def __init__(self) -> None:
        super().__init__({
            "scl": Out(1),
            "scl_enable": Out(1),
            "sda_in": In(1),
            "sda_out": Out(1, init=0),  # always driven low
            "sda_out_enable": Out(1),    # toggled to actually drive the line

            "busy": Out(1), # transaction in progress
            "start": In(1), # rising edge triggers transaction
            "error": Out(1), # error during transaction

            "read": In(1), # 0 for write, 1 for read

            "device_address": In(7),
            "register_address": In(8),
            "byte_count": In(3),    # up to 4 bytes in a transaction (this only includes data)
            "data_in": In(8*4),
            "data_out": Out(8*4)


            # "write_data": In(32),
            # "write_enable": In(1),
            # "address": In(16),
            # "read_data": Out(32)
        })
    
    def elaborate(self, platform):
        m = Module()

        #self.scl_enable = Signal()
        self.scl_toggle = Signal()
        self.counter = Signal(range(int(25e6/400e3/2)))
        self.half_counter = Signal(range(int(25e6/400e3/4)))
        
        self.scl_high_sample = Signal() # triggers to change sda that are centered between scl edges
        self.scl_low_sample = Signal()

        self.bit_counter = Signal(3)
        self.byte_counter = Signal(3)

        # generate clock signal (400khz)
        with m.If(self.counter == 0):
            m.d.sync_25 += self.scl_toggle.eq(~self.scl_toggle)
            m.d.sync_25 += self.counter.eq(int(25e6/400e3/2)-1)
            m.d.sync_25 += self.half_counter.eq(int(25e6/400e3/4)-1)

        with m.Else():
            m.d.sync_25 += self.counter.eq(self.counter - 1)
            with m.If(self.half_counter == 0):
                m.d.sync_25 += self.scl_high_sample.eq(self.scl)
                m.d.sync_25 += self.scl_low_sample.eq(~self.scl)
                m.d.sync_25 += self.half_counter.eq(int(25e6/400e3/4))
            with m.Else():
                m.d.sync_25 += self.half_counter.eq(self.half_counter - 1)
                m.d.sync_25 += self.scl_high_sample.eq(0)
                m.d.sync_25 += self.scl_low_sample.eq(0)

        m.d.sync_25 += self.scl.eq(self.scl_enable & self.scl_toggle)
        #m.d.comb += self.scl_enable.eq(self.scl_enable)
        

        self.d_out = Signal(init=1)
        m.d.comb += self.sda_out_enable.eq(~self.d_out)  # we use the out enable as the ouput since i2c is open drain driven (output is never DRIVEN high)
        
        self.send_data = Signal(8*6)
        self.read_data = Signal(8*4)

        self.bytes_sent = Signal(3)

        self.read_next = Signal()

        with m.FSM(init="idle", domain="sync_25", name="i2c_fsm"):
            with m.State("idle"):
                m.d.sync_25 += [
                    self.busy.eq(0),
                    self.d_out.eq(1),
                    self.scl_enable.eq(0),
                    self.read_next.eq(0),
                ]
                with m.If(self.start):    
                    m.next = "start"

            with m.State("start"):
                m.d.sync_25 += [
                    self.busy.eq(1),
                    self.error.eq(0),    # errors are cleared on start
                    self.byte_counter.eq(5),
                    self.bytes_sent.eq(0),
                    self.bit_counter.eq(7),
                    self.read_data.eq(0),
                    self.send_data[40:48].eq(self.device_address << 1 | self.read_next),
                    self.send_data[32:40].eq(self.register_address),
                    self.send_data[24:32].eq(self.data_in[0:8]),
                    self.send_data[16:24].eq(self.data_in[8:16]),
                    self.send_data[8:16].eq(self.data_in[16:24]),
                    self.send_data[0:8].eq(self.data_in[24:32]),
                ]

                with m.If(self.counter == 0):
                    m.d.sync_25 += self.scl_enable.eq(1)
                
                with m.If(self.scl_high_sample):
                    m.d.sync_25 += self.d_out.eq(0)
                    m.next = "send"

            with m.State("send"):
                with m.If(self.scl_low_sample):
                    m.d.sync_25 += self.d_out.eq((self.send_data.bit_select(self.bit_counter | (self.byte_counter << 3), 1)))
                    
                    with m.If(self.bit_counter == 0):
                        m.d.sync_25 += self.byte_counter.eq(self.byte_counter - 1)
                        with m.If(self.byte_counter < 4):
                            m.d.sync_25 += self.bytes_sent.eq(self.bytes_sent + 1)
                        m.d.sync_25 += self.bit_counter.eq(7)
                        m.next = "receive_ack_prepare"

                    with m.Else():
                        m.d.sync_25 += self.bit_counter.eq(self.bit_counter - 1)

            with m.State("receive_start"):
                with m.If(self.scl_high_sample):
                    m.d.sync_25 += self.d_out.eq(0)
                    m.next = "receive"

            with m.State("receive"):
                with m.If(self.scl_high_sample):
                    #m.d.sync_25 += self.d_out.eq((self.send_data.bit_select(self.bit_counter | (self.byte_counter << 3), 1)))
                    m.d.sync_25 += self.read_data.bit_select(self.bit_counter | (self.byte_counter << 3), 1).eq(self.sda_in | 1)    # DEBUG
                    
                    with m.If(self.bit_counter == 0):
                        m.d.sync_25 += self.byte_counter.eq(self.byte_counter - 1)
                        m.d.sync_25 += self.bytes_sent.eq(self.bytes_sent + 1)
                        m.d.sync_25 += self.bit_counter.eq(7)
                        m.next = "send_ack_prepare"

                    with m.Else():
                        m.d.sync_25 += self.bit_counter.eq(self.bit_counter - 1)

            with m.State("send_ack_prepare"):
                with m.If(self.scl_low_sample):
                    m.d.sync_25 += self.d_out.eq(self.bytes_sent >= self.byte_count)
                    m.next = "send_ack"

            with m.State("send_ack"):
                with m.If(self.scl_low_sample):
                    with m.If(self.d_out):
                        m.next = "done"
                    with m.Else():
                        m.next = "receive"
            
            with m.State("receive_ack_prepare"):
                with m.If(self.scl_low_sample):
                    m.d.sync_25 += self.d_out.eq(1)
                    m.next = "receive_ack"

            with m.State("receive_ack"):
                with m.If(self.scl_high_sample):
                    with m.If(self.sda_in):
                        m.next = "error"
                    with m.Else():
                        with m.If(self.bytes_sent >= self.byte_count):
                            m.next = "done"
                        with m.Elif(self.read & (self.byte_counter == 3)):    # read next
                            m.d.sync_25 += self.read_next.eq(1)
                            m.d.sync_25 += self.byte_counter.eq(5)
                            m.next = "start"
                        with m.Elif(self.read_next):
                            m.d.sync_25 += self.byte_counter.eq(3)
                            m.next = "receive"
                        with m.Else():  # normal write
                            m.next = "send"

            with m.State("done"):
                with m.If(self.scl_low_sample):
                    m.d.sync_25 += self.d_out.eq(0)
                with m.If(self.scl_high_sample):
                    m.d.sync_25 += self.d_out.eq(1)
                    m.next = "idle"

                m.d.sync_25 += [
                    self.data_out[0:8].eq(self.read_data[24:32]),
                    self.data_out[8:16].eq(self.read_data[16:24]),
                    self.data_out[16:24].eq(self.read_data[8:16]),
                    self.data_out[24:32].eq(self.read_data[0:8]),
                ]

            with m.State("error"):
                m.d.sync_25 += self.error.eq(1)
                m.next = "idle"

        return m

dut = i2c()
async def i2c_bench(ctx):
    # write
    ctx.set(dut.device_address, 0x2)
    ctx.set(dut.register_address, 0x3)
    ctx.set(dut.data_in, 0x4)
    ctx.set(dut.byte_count, 1)

    ctx.set(dut.sda_in, 0)

    ctx.set(dut.start, 1)
    await ctx.tick("sync_25")
    ctx.set(dut.start, 0)

    for i in range(2000):
        await ctx.tick("sync_25")

    # read
    ctx.set(dut.device_address, 0x2)
    ctx.set(dut.register_address, 0x3)
    ctx.set(dut.read, 1)
    ctx.set(dut.data_in, 0x4)
    ctx.set(dut.byte_count, 1)

    ctx.set(dut.sda_in, 0)

    ctx.set(dut.start, 1)
    await ctx.tick("sync_25")
    ctx.set(dut.start, 0)

    for i in range(3000):
        await ctx.tick("sync_25")
    


if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/25e6, domain="sync_25")
    sim.add_testbench(i2c_bench)
    with sim.write_vcd("i2c_test.vcd"):
        sim.run()
