from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.wiring import Component, In, Out
from amaranth.lib.cdc import FFSynchronizer
from amaranth.lib.crc.catalog import CRC16_X25

import math

from registers2 import *


class Yaskawa_Encoders(Component):
    # Yaskawa encoder interface

    # Connected using rs485
    # manchester encoded with bit stuffing

    def __init__(self, number_of_encoders: int, encoder_settings: list):

        assert number_of_encoders > 0 and number_of_encoders <= 32

        self.number_of_encoders = number_of_encoders

        self.encoder_settings = encoder_settings

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
        self.encoder_group.add(Register("commutation_count", rw="r", type="unsigned", width=16, desc="Absolute commutation count"))   # scaled to 16 bits (just single turn in this case)
        self.encoder_group.add(Register("timestamp_slow", rw="r", type="unsigned", width=8, desc=""))
        self.encoder_group.add(Register("timestamp_fast", rw="r", type="unsigned", width=16, desc=""))


        self.encoder_group.add(Register("status", rw="r", desc="Encoder status", sub_registers=[
            Register("battery_fail", type="bool", desc="Battery fail"),
            Register("unindexed", type="bool", desc="Unindexed (currently unsupported)"),
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

        m.submodules.spi_send_debug = spi_debug = spi_send(data_width=112, data_count=self.number_of_encoders)

        m.d.sync_100 += [
            self.debug[0].eq(spi_debug.data_enable),
            self.debug[1].eq(spi_debug.clk),
            self.debug[2:8].eq(spi_debug.data),
        ]

        trigger = Signal()
        rx_start = Signal()

        new_data = Signal(self.number_of_encoders)

        last_dones = Signal(self.number_of_encoders)


        for i in range(self.number_of_encoders):
            receiver = Receive_Packet(self.encoder_settings[i])
            m.submodules[f"receiver_{i}"] = receiver
            self.encoders.append(receiver)

            m.d.comb += receiver.rx.eq(self.synced_rx[i])
            m.d.comb += receiver.start.eq(rx_start)

            with m.If(receiver.done & (~last_dones[i])):
                m.d.sync_100 += new_data[i].eq(1)

            m.d.sync_100 += last_dones[i].eq(receiver.done)

            m.d.sync_100 += spi_debug.__getattribute__(f"raw_data_{i}").eq(receiver.raw_data)


        with m.If(new_data.all()):
            m.d.sync_100 += spi_debug.start.eq(1)
            m.d.sync_100 += new_data.eq(0)
        with m.Else():
            m.d.sync_100 += spi_debug.start.eq(0)

            

        with m.FSM(domain="sync_100", init="idle") as fsm:
            with m.State("idle"):
                m.d.sync_100 += rx_start.eq(0)

                # for index, i in enumerate(self.encoders):
                #     with m.If(i.done):
                #         m.d.sync_100 += new_data[index].eq(1)


                with m.If(trigger):
                    m.d.sync_100 += request_packet.trigger.eq(1)
                    # m.d.sync_100 += new_data.eq(0)
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


        m.d.comb += self.tx.eq(request_packet.tx.replicate(self.number_of_encoders))
        m.d.comb += self.tx_enable.eq(request_packet.tx_enable.replicate(self.number_of_encoders))

        # system regs
        with m.If(self.bram_write_enable & (self.bram_address == self.rm.trigger.address_offset)):
            m.d.sync_100 += trigger.eq(1)
        with m.Else():
            m.d.sync_100 += trigger.eq(0)

        with m.If(self.bram_write_enable & (self.bram_address == self.rm.request_packet.address_offset)):
            m.d.sync_100 += request_packet.request_packet_data.eq(self.bram_write_data)


        # encoder regs

        encoder_address_lsb = int(math.log2(self.rm.encoder.alignment)) # TODO: add otion to get these directly from the register map
        encoder_address_msb = int(math.log2(self.rm.encoder.count)) + encoder_address_lsb + 1

        selected_encoder = Signal(range(self.number_of_encoders))
        m.d.comb += selected_encoder.eq(self.bram_address[encoder_address_lsb:encoder_address_msb])

        with m.Switch(selected_encoder):

            for index, e in enumerate(self.encoders):
                
                with m.Case(index):  # selected the encoder
                    with m.Switch(self.bram_address[0:encoder_address_lsb]):
                        with m.Case(self.rm.encoder.multiturn_count.address_offset):
                            m.d.sync_100 += self.bram_read_data.eq(e.multi_turn)
                        with m.Case(self.rm.encoder.singleturn_count.address_offset):
                            m.d.sync_100 += self.bram_read_data.eq(e.single_turn)
                        with m.Case(self.rm.encoder.commutation_count.address_offset):     # no actual commutation count, so just use single turn
                            m.d.sync_100 += self.bram_read_data.eq(e.single_turn>>16)
                        with m.Case(self.rm.encoder.timestamp_slow.address_offset):
                            m.d.sync_100 += self.bram_read_data.eq(e.timestamp_slow)
                        with m.Case(self.rm.encoder.timestamp_fast.address_offset):
                            m.d.sync_100 += self.bram_read_data.eq(e.timestamp_fast)
                        with m.Case(self.rm.encoder.status.address_offset):
                            m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.battery_fail.starting_bit].eq(e.battery_fail)
                            m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.no_response.starting_bit].eq(e.no_response)
                            m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.crc_fail.starting_bit].eq(~e.crc_valid)
                            m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.done.starting_bit].eq(e.done)
                            m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.unindexed.starting_bit].eq(e.unindexed)
                        with m.Default():
                            m.d.sync_100 += self.bram_read_data.eq(0)
                    
                
        return m
    

class Request_Packet(Component):
    def __init__(self):
        super().__init__({
            "request_packet_data": In(16),
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
        m.submodules += FFSynchronizer(i=self.trigger, o=trigger, o_domain="sync_200", stages=4)

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
    def __init__(self, settings: dict):
        self.settings = settings
        super().__init__({
            "rx": In(1),
            "start": In(1),
            "done": Out(1),
            "no_response": Out(1),
            "raw_data": Out(112),
            "crc_valid": Out(1),
            "single_turn": Out(32), # actual counts left shifted to fill 32 bits
            "multi_turn": Out(16),
            "battery_fail": Out(1),
            "timestamp_slow": Out(8),
            "timestamp_fast": Out(16),
            "unindexed": Out(1),
        })

    def elaborate(self, platform):
        m = Module()

        # CRC
        m.submodules.crc16_rx = txCRC = DomainRenamer("sync_200")(CRC16_X25(1).create())

        done = Signal()
        m.d.comb += self.done.eq(done)

        start = Signal()
        m.submodules += FFSynchronizer(i=self.start, o=start, o_domain="sync_200")

        raw_data = Signal(112)
        m.d.comb += self.raw_data.eq(raw_data)

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
        crc_reached = Signal()
        crc_match = Signal()

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

        with m.If(txCRC.start):
            m.d.sync_200 += txCRC.start.eq(0)

        with m.If(txCRC.valid):
            m.d.sync_200 += txCRC.valid.eq(0)

        with m.If(~rising_edge & (~falling_edge) & (timer != timeout_bit_time_count)):
            m.d.sync_200 += timer.eq(timer + 1)

        m.d.comb += txCRC.data.eq(~self.rx)

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
                        m.d.sync_200 += txCRC.valid.eq(~crc_reached)
                    with m.If(current_bit_cnt == len(raw_data)-17):
                        m.d.sync_200 += crc_reached.eq(1)

        with m.FSM(init="idle", domain="sync_200"):
            with m.State("idle"):
                with m.If(start):
                    m.d.sync_200 += current_bit_cnt.eq(0)
                    m.d.sync_200 += raw_data.eq(0)
                    m.d.sync_200 += done.eq(0)
                    m.d.sync_200 += txCRC.start.eq(1)
                    m.d.sync_200 += crc_reached.eq(0)
                    m.d.sync_200 += self.no_response.eq(1)
                    m.d.sync_100 += self.crc_valid.eq(0)
                    m.next = "start_sync"

            with m.State("start_sync"):
                with m.If(rising_edge):
                    m.d.sync_200 += timer.eq(0)
                    m.d.sync_200 += half_bit_reached.eq(0)
                    m.d.sync_200 += timeout_bit_reached.eq(0)
                    m.d.sync_200 += receive.eq(1)
                    m.d.sync_200 += self.no_response.eq(0)
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
                with m.If(txCRC.crc == raw_data[96:112]):
                    m.d.sync_100 += crc_match.eq(1)
                with m.If(last_8_bits == flag):
                    m.d.sync_200 += done.eq(1)
                    m.next = "idle"
                with m.If(timeout_bit_reached):
                    m.next = "idle"

        with m.If(crc_match):
            m.d.sync_100 += crc_match.eq(0)
            m.d.sync_100 += self.crc_valid.eq(1)
            m.d.sync_100 += [
                self.battery_fail.eq(raw_data[2]),
                self.timestamp_slow.eq(raw_data[16:24]),
                self.timestamp_fast.eq(raw_data[24:40]),
                self.unindexed.eq(raw_data[0])  # pretty sure this is correct, but don't have a way to reset it
            ]
            if self.settings.get("mode") == "16bit":
                m.d.sync_100 += self.single_turn.eq(raw_data[60:76]<<16)
                m.d.sync_100 += self.multi_turn.eq(raw_data[76:92])

            elif self.settings.get("mode") == "17bit":
                m.d.sync_100 += self.single_turn.eq(raw_data[60:77]<<15)
                m.d.sync_100 += self.multi_turn.eq(raw_data[77:93])

            else:
                raise ValueError("Invalid mode in settings, must be '16bit' or '17bit'")
            

            m.d.sync_200 += raw_data[96:112].eq(0)  # clear the crc so it doesn't get reused

        return m

class spi_send(Component):
    def __init__(self, data_width: int = 112, data_count: int = 6):
        assert data_width > 0 and data_width > 0
        self.data_width = data_width
        self.data_count = data_count

        s = {
            "start": In(1),
            "done": Out(1),
            "clk": Out(1),
            "data": Out(data_count),
            "data_enable": Out(1, init=1),  # active low
        }
        for i in range(data_count):
            s[f"raw_data_{i}"] = In(data_width)
        
        super().__init__(s)

    def elaborate(self, platform):
        m = Module()

        
        bit_count = Signal(range(self.data_width + 1))
        bit_timer = Signal(8)
        full_bit_time = 8
        bit_time_1_4 = full_bit_time // 4
        bit_time_1_2 = full_bit_time // 2
        
        full_data = Array(Signal(self.data_width) for _ in range(self.data_count))


        with m.FSM(init="idle", domain="sync_100") as fsm:
            with m.State("idle"):
                with m.If(self.start):
                    m.d.sync_100 += bit_count.eq(0)
                    m.d.sync_100 += bit_timer.eq(0)
                    m.d.sync_100 += self.done.eq(0)
                    m.d.sync_100 += self.data_enable.eq(0)  # active low
                    for i in range(self.data_count):    # latch in data
                        m.d.sync_100 += full_data[i].eq(self.__getattribute__(f"raw_data_{i}"))
                    m.next = "send"

            with m.State("send"):
                with m.If(bit_timer == bit_time_1_4):   # set new data
                    with m.If(bit_count == self.data_width):
                        m.d.sync_100 += self.done.eq(1)
                        m.d.sync_100 += self.data_enable.eq(1)  # active low
                        m.next = "idle"
                    with m.Else():
                        for i in range(self.data_count):
                            m.d.sync_100 += self.data[i].eq(full_data[i].bit_select(bit_count, 1))
                            m.d.sync_100 += bit_count.eq(bit_count + 1)

                with m.If(bit_timer == bit_time_1_2):   # rising clk
                    m.d.sync_100 += self.clk.eq(1)

                with m.If(bit_timer == full_bit_time):  # falling clk
                    m.d.sync_100 += self.clk.eq(0)
                    m.d.sync_100 += bit_timer.eq(0)     
                with m.Else():
                    m.d.sync_100 += bit_timer.eq(bit_timer + 1)

        return m
s = [
    {"mode": "17bit"},
    {"mode": "17bit"},
    {"mode": "17bit"},
    {"mode": "16bit"},
    {"mode": "16bit"},
    {"mode": "16bit"}
]
dut = Yaskawa_Encoders(6, s)

async def bench(ctx):
    ctx.set(dut.rx, 1)

    for c in range(1):

        ctx.set(dut.bram_address, dut.rm.request_packet.address_offset)
        ctx.set(dut.bram_write_data, 0b0111111001111110)
        ctx.set(dut.bram_write_enable, 1)
        await ctx.tick("sync_100")
        ctx.set(dut.bram_address, dut.rm.trigger.address_offset)
        ctx.set(dut.bram_write_enable, 1)
        await ctx.tick("sync_100")
        ctx.set(dut.bram_write_enable, 0)

        await ctx.tick("sync_100").repeat(2000)

        data = '010101010101010100111111010100000001001001110100000010011111010111001110000000000001110001000010001111101111101111101111100000100011101100101001111110'
        desired = '1010000000100100111010000001001111110111001110000000000001110001000010001111111111111111111100001000111011001010'
        clk = 0

        for d in data:

            for i in range(2):
                level = (int(d) or clk) and not (int(d) and clk)
                for j in range(6):
                    ctx.set(dut.rx[j], level)
                if clk:
                    clk = 0
                else:
                    clk = 1
                await ctx.tick("sync_200").repeat(25)

        await ctx.tick("sync_100").repeat(4000*2)

        # force the encoders to have some different values
        for index, e in enumerate(dut.encoders):
            ctx.set(e.multi_turn, index)
            ctx.set(e.single_turn, index * 100)


        for c in range(6):
            # read the data
            ctx.set(dut.bram_address, dut.rm.encoder.multiturn_count.address_offset + c*8)  # multiturn for each encoder
            await ctx.tick("sync_100").repeat(2)
            print(f"Multi turn {c}: ", ctx.get(dut.bram_read_data))

            ctx.set(dut.bram_address, dut.rm.encoder.singleturn_count.address_offset + c*8)  # singleturn for each encoder
            await ctx.tick("sync_100").repeat(2)
            print(f"Single turn {c}: ", ctx.get(dut.bram_read_data)>>16)    # right shift to get the actual 16 bit value


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
