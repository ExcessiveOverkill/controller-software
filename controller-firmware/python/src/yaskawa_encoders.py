from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.wiring import Component, In, Out
from amaranth.lib.cdc import FFSynchronizer

from registers2 import *


class Yaskawa_Encoders(Component):
    # Yaskawa encoder interface

    # Connected using rs485
    # manchester encoded with bit stuffing

    def __init__(self, number_of_encoders: int):

        assert number_of_encoders > 0 and number_of_encoders <= 32

        self.number_of_encoders = number_of_encoders

        super().__init__({
            "tx" : Out(self.number_of_encoders),
            "tx_enable" : Out(self.number_of_encoders),
            "rx" : In(self.number_of_encoders),

            "bram_address": In(16),
            "bram_write_data": In(32),
            "bram_read_data": Out(32),
            "bram_write_enable": In(1),

            "debug": Out(8)
        })

        driver_settings = {}
        self.rm = RegisterMapGenerator("yaskawa_encoders", ["yaskawa_encoders"], driver_settings, "Yaskawa serial encoder interface")
        self.encoder_group = Group("encoder", self.number_of_encoders, 0x0, "Group of registers for each encoder")
        self.encoder_group.add(Register("multiturn_count", rw="r", type="unsigned", width=32, desc="Absolute multiturn count"))      # not scaled, typically only 16 bits are used
        self.encoder_group.add(Register("singleturn_count", rw="r", type="unsigned", width=32, desc="Absolute singleturn count"))    # scaled to 32 bits
        #self.encoder_group.add(Register("commutation_count", rw="r", type="unsigned", width=16, desc="Absolute commutation count"))   # scaled to 16 bits
        self.encoder_group.add(Register("unknown1", rw="r", type="unsigned", width=8, desc=""))
        self.encoder_group.add(Register("unknown2", rw="r", type="unsigned", width=16, desc=""))
        self.encoder_group.add(Register("unknown3", rw="r", type="unsigned", width=16, desc=""))


        self.encoder_group.add(Register("status", rw="r", desc="Encoder status", sub_registers=[
            Register("battery_fail", type="bool", desc="Battery fail"),
            Register("unindexed", type="bool", desc="Unindexed"),
            Register("no_response", type="bool", desc="No response"),
            Register("crc_fail", type="bool", desc="CRC fail"),
            Register("done", type="bool", desc="Done"),
        ]))

        self.rm.add(Register("trigger", rw="w", type="bool", desc="Trigger encoder capture"))
        self.rm.add(Register("request_packet", rw="w", type="unsigned", width=16, desc="Data to send to encoder from the controller"))


        self.rm.add(self.encoder_group)
        self.rm.generate()

        self.encoders = []


    def elaborate(self, platform):
        m = Module()

        self.synced_rx = Signal(self.number_of_encoders)

        m.submodules += FFSynchronizer(i=self.rx, o=self.synced_rx, o_domain="sync_100")

        


        m.submodules.request_packet = request_packet = Request_Packet()


        trigger = Signal()
        rx_start = Signal()

        new_data = Signal(self.number_of_encoders)
        last_new_data = Signal(self.number_of_encoders)
        m.d.sync_100 += last_new_data.eq(new_data)


        for i in range(self.number_of_encoders):
            receiver = Receive_Packet()
            m.submodules[f"receiver_{i}"] = receiver
            self.encoders.append(receiver)

            m.d.comb += receiver.rx.eq(self.synced_rx[i])
            m.d.comb += receiver.start.eq(rx_start)

            

        with m.FSM(domain="sync_100", init="idle") as fsm:
            with m.State("idle"):
                m.d.sync_100 += rx_start.eq(0)

                for index, i in enumerate(self.encoders):
                    with m.If(i.done):
                        m.d.sync_100 += new_data[index].eq(1)


                with m.If(trigger):
                    m.d.sync_100 += request_packet.trigger.eq(1)
                    m.d.sync_100 += new_data.eq(0)
                    m.next = "send_start"

            with m.State("send_start"):
                with m.If(~request_packet.done):
                    m.next = "send"

            with m.State("send"):
                m.d.sync_100 += request_packet.trigger.eq(0)
                with m.If(request_packet.done):
                    m.d.sync_100 += rx_start.eq(1)
                    m.next = "receive"
            
            with m.State("receive"):
                m.d.sync_100 += rx_start.eq(0)
                m.next = "idle"


        

        
        m.d.sync_100 += self.debug[0].eq(self.synced_rx[0])
        m.d.sync_100 += self.debug[1].eq(request_packet.trigger)
        m.d.sync_100 += self.debug[2].eq(request_packet.done)
        m.d.sync_100 += self.debug[3].eq(self.encoders[0].start)
        m.d.sync_100 += self.debug[4].eq(self.encoders[0].done)



        debug_timer = Signal(range(25))
        debug_cnt = Signal(range(112))


        with m.If((~last_new_data[0]) & new_data[0]):
            m.d.sync_100 += debug_cnt.eq(0)
            m.d.sync_100 += debug_timer.eq(0)
        
        with m.If(new_data[0] & (debug_cnt != 112)):
            with m.If(debug_timer == 25):
                m.d.sync_100 += self.debug[7].eq(self.encoders[0].raw_data.bit_select(debug_cnt, 1))
                m.d.sync_100 += debug_cnt.eq(debug_cnt + 1)
                m.d.sync_100 += debug_timer.eq(0)
            
            with m.Else():
                m.d.sync_100 += debug_timer.eq(debug_timer + 1)        

        with m.Else():
            m.d.sync_100 += self.debug[7].eq(0)


        m.d.comb += self.tx.eq(request_packet.tx.replicate(self.number_of_encoders))
        m.d.comb += self.tx_enable.eq(request_packet.tx_enable.replicate(self.number_of_encoders))

        with m.If(self.bram_write_enable & (self.bram_address == self.rm.trigger.address_offset)):
            m.d.sync_100 += trigger.eq(1)
        with m.Else():
            m.d.sync_100 += trigger.eq(0)

        with m.If(self.bram_write_enable & (self.bram_address == self.rm.request_packet.address_offset)):
            m.d.sync_100 += request_packet.request_packet_data.eq(self.bram_write_data)


        return m
    

class Request_Packet(Component):
    def __init__(self):
        super().__init__({
            "request_packet_data": In(16, init=0xFFFF),
            "trigger": In(1),
            "tx": Out(1),
            "tx_enable": Out(1),
            "done": Out(1),
        })

    def elaborate(self, platform):
        m = Module()

        request_packet_data = Signal(16)
        #m.d.comb += request_packet_data.eq(self.request_packet_data)
        m.submodules += FFSynchronizer(i=self.request_packet_data, o=request_packet_data, o_domain="sync_200")

        trigger = Signal()
        #m.d.comb += trigger.eq(self.trigger)
        m.submodules += FFSynchronizer(i=self.trigger, o=trigger, o_domain="sync_200")

        done = Signal()
        m.d.comb += self.done.eq(done)
        #m.submodules += FFSynchronizer(i=done, o=self.done, o_domain="sync_100")

        bit_time = 250e-9   # 250ns

        half_bit_time_count = int(200e6 * bit_time * .5) - 1

        preemble = Signal(16, init=0b0101010101010101)
        flag = Signal(8, init=0b01111110)
        
        timer = Signal(range(half_bit_time_count))

        clock = Signal()

        data = Signal(len(preemble) + len(flag) + len(request_packet_data) + len(flag))

        internal_data_start = len(preemble) + len(flag)
        internal_data_end = internal_data_start + len(request_packet_data)

        current_bit_cnt = Signal(range(len(data)))
        current_bit = Signal()
        last_5_bits = Signal(5)

        tx_inverted = Signal()
        m.d.comb += tx_inverted.eq(~self.tx)


        stuff_bit = Signal()
        increment_bit = Signal()

        with m.If((current_bit_cnt > internal_data_start) & (current_bit_cnt < internal_data_end) & (last_5_bits == 0b11111)):
            m.d.comb += stuff_bit.eq(1)
            m.d.sync_200 += current_bit.eq(0)
        with m.Else():
            m.d.comb += stuff_bit.eq(0)
            m.d.sync_200 += current_bit.eq(data.bit_select(current_bit_cnt, 1))

        with m.If(increment_bit):
            with m.If(~stuff_bit):
                m.d.sync_200 += current_bit.eq(data.bit_select(current_bit_cnt, 1))
                m.d.sync_200 += current_bit_cnt.eq(current_bit_cnt + 1)

            m.d.sync_200 += last_5_bits.eq(Cat(last_5_bits[1:], current_bit))
            m.d.sync_200 += increment_bit.eq(0)

        
        clk_falling = Signal()
        clk_rising = Signal()

        clk_reset = Signal()

        with m.If(~clk_reset):
            with m.If(timer == half_bit_time_count):
                m.d.sync_200 += clock.eq(~clock)
                m.d.sync_200 += clk_falling.eq(clock)
                m.d.sync_200 += clk_rising.eq(~clock)
                m.d.sync_200 += timer.eq(0)
            with m.Else():
                m.d.sync_200 += timer.eq(timer + 1)
                m.d.sync_200 += clk_falling.eq(0)
                m.d.sync_200 += clk_rising.eq(0)
        with m.Else():
            m.d.sync_200 += timer.eq(0)
            m.d.sync_200 += clk_falling.eq(0)
            m.d.sync_200 += clk_rising.eq(0)
            m.d.sync_200 += clock.eq(0)
            m.d.sync_200 += clk_reset.eq(0)


        m.d.comb += data.eq(Cat(preemble, flag, request_packet_data, flag))


        with m.FSM(init="idle", domain="sync_200"):
            with m.State("idle"):
                m.d.sync_200 += self.tx.eq(0)
                m.d.sync_200 += self.tx_enable.eq(0)
                m.d.sync_200 += done.eq(1)
                m.d.sync_200 += current_bit_cnt.eq(0)
                with m.If(trigger):
                    m.d.sync_200 += done.eq(0)
                    m.d.sync_200 += clk_reset.eq(1)
                    m.next = "send"

            with m.State("send"):
                with m.If(clk_rising):
                    m.d.sync_200 += increment_bit.eq(1)
                with m.If(clk_falling):
                    m.d.sync_200 += self.tx_enable.eq(1)
                with m.If(clk_falling | clk_rising):
                    m.d.sync_200 += self.tx.eq((current_bit | clock) & ~(current_bit & clock))

                with m.If(current_bit_cnt == len(data)):
                    m.next = "done"

            with m.State("done"):
                with m.If(clk_falling):
                    m.d.sync_200 += self.tx_enable.eq(0)
                    m.next = "idle"

        return m
    

class Receive_Packet(Component):
    def __init__(self):
        super().__init__({
            "rx": In(1),
            "start": In(1),
            "done": Out(1),
            "raw_data": Out(112),
            "crc_valid": Out(1)
        })

    def elaborate(self, platform):
        m = Module()

        done = Signal()
        m.d.comb += self.done.eq(done)
        #m.submodules += FFSynchronizer(i=done, o=self.done, o_domain="sync_100")

        start = Signal()
        #m.d.comb += start.eq(self.start)
        m.submodules += FFSynchronizer(i=self.start, o=start, o_domain="sync_200")

        raw_data = Signal(112)
        m.d.comb += self.raw_data.eq(raw_data)
        #m.submodules += FFSynchronizer(i=raw_data, o=self.raw_data, o_domain="sync_100")

        bit_time = 250e-9   # 250ns

        half_bit_time_count = int(200e6 * bit_time * .75) - 1   # edges before this are considered half bit time
        timeout_bit_time_count = int(200e6 * bit_time * 1.25) - 1   # edges after this are considered timeout

        flag = Signal(8, init=0b01111110)
        
        timer = Signal(range(timeout_bit_time_count))
        half_bit_reached = Signal()
        timeout_bit_reached = Signal()

        with m.If(timer == half_bit_time_count):
            m.d.sync_200 += half_bit_reached.eq(1)
        with m.If(timer == timeout_bit_time_count):
            m.d.sync_200 += timeout_bit_reached.eq(1)


        current_bit_cnt = Signal(range(len(raw_data)))
        last_8_bits = Signal(8)
        receive = Signal()
        save = Signal()

        falling_edge = Signal()
        rising_edge = Signal()
        prev_rx = Signal()

        m.d.sync_200 += prev_rx.eq(self.rx)

        with m.If(~prev_rx & self.rx):
            m.d.sync_200 += rising_edge.eq(1)
        with m.Else():
            m.d.sync_200 += rising_edge.eq(0)
        with m.If(prev_rx & ~self.rx):
            m.d.sync_200 += falling_edge.eq(1)
        with m.Else():
            m.d.sync_200 += falling_edge.eq(0)

        with m.If(~rising_edge & (~falling_edge) & (timer != timeout_bit_time_count)):
            m.d.sync_200 += timer.eq(timer + 1)

        with m.If(receive):
            with m.If((falling_edge | rising_edge) & half_bit_reached):
                m.d.sync_200 += timer.eq(0)
                m.d.sync_200 += half_bit_reached.eq(0)
                m.d.sync_200 += timeout_bit_reached.eq(0)
                m.d.sync_200 += last_8_bits.eq(Cat(~self.rx, last_8_bits[:-1]))
                with m.If(save):
                    m.d.sync_200 += raw_data.bit_select(current_bit_cnt, 1).eq(~self.rx)
                    with m.If(last_8_bits[0:5] != 0b11111):
                        m.d.sync_200 += current_bit_cnt.eq(current_bit_cnt + 1)

        with m.FSM(init="idle", domain="sync_200"):
            with m.State("idle"):
                with m.If(start):
                    m.d.sync_200 += current_bit_cnt.eq(0)
                    m.d.sync_200 += raw_data.eq(0)
                    m.d.sync_200 += done.eq(0)
                    m.next = "start_sync"

            with m.State("start_sync"):
                with m.If(rising_edge):
                    m.d.sync_200 += timer.eq(0)
                    m.d.sync_200 += half_bit_reached.eq(0)
                    m.d.sync_200 += timeout_bit_reached.eq(0)
                    m.d.sync_200 += receive.eq(1)
                    m.next = "start_flag"
            
            with m.State("start_flag"):
                with m.If(last_8_bits == flag):
                    m.d.sync_200 += save.eq(1)
                    m.next = "receive_data"
                with m.If(timeout_bit_reached):
                    m.next = "idle"

            with m.State("receive_data"):
                with m.If(current_bit_cnt == len(raw_data)):
                    m.d.sync_200 += save.eq(0)
                    m.next = "end_flag"
                with m.If(timeout_bit_reached):
                    m.next = "idle"

            with m.State("end_flag"):
                with m.If(last_8_bits == flag):
                    m.d.sync_200 += done.eq(1)
                    m.next = "idle"
                with m.If(timeout_bit_reached):
                    m.next = "idle"


        return m



dut = Yaskawa_Encoders(6)

async def bench(ctx):
    ctx.set(dut.rx, 1)

    for c in range(2):

        ctx.set(dut.bram_address, dut.rm.request_packet.address_offset)
        ctx.set(dut.bram_write_data, 0b0111111001111110)
        ctx.set(dut.bram_write_enable, 1)
        await ctx.tick("sync_100")
        ctx.set(dut.bram_address, dut.rm.trigger.address_offset)
        await ctx.tick("sync_100")
        ctx.set(dut.bram_write_enable, 0)

        await ctx.tick("sync_100").repeat(2000)

        data = '010101010101010100111111010100000001001001110100000010011111010111001110000000000001110001000010001111101111101111101111100000100011101100101001111110'
        desired = '1010000000100100111010000001001111110111001110000000000001110001000010001111111111111111111100001000111011001010'
        clk = 0

        for d in data:

            for i in range(2):
                level = (int(d) or clk) and not (int(d) and clk)
                ctx.set(dut.rx, level)
                if clk:
                    clk = 0
                else:
                    clk = 1
                await ctx.tick("sync_200").repeat(25)

        await ctx.tick("sync_100").repeat(4000)

        result = ctx.get(dut.encoders[0].raw_data)
        result = f"{result:0112b}"

        print(result[::-1])
        print(desired)



if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/100e6, domain="sync_100")
    sim.add_clock(1/200e6, domain="sync_200")
    sim.add_testbench(bench)
    with sim.write_vcd("yaskawa_encoders_test.vcd"):
        sim.run()
