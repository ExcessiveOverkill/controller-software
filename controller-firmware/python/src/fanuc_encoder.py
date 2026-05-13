from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.wiring import Component, In, Out
from amaranth.lib.cdc import FFSynchronizer
from amaranth.lib.crc.catalog import CRC5_ITU

from registers2 import *

from sandbox.fanuc_encoder_sim import rs422_sim, rs485_sim

import math



class Fanuc_Encoders(Component):
    # Fanuc encoder interface

    def __init__(self, number_of_encoders: int):

        assert(number_of_encoders > 0 and number_of_encoders <= 32)

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

        # setup registers for DMA access
        driver_settings = {}
        self.rm = RegisterMapGenerator("fanuc_encoders", ["fanuc_encoders"], driver_settings, "Fanuc serial encoder interface")
        self.encoder_group = Group("encoder", self.number_of_encoders, 0x0, "Group of registers for each encoder")
        self.encoder_group.add(Register("multiturn_count", rw="r", type="unsigned", width=32, desc="Absolute multiturn count"))      # not scaled, typically only 16 bits are used
        self.encoder_group.add(Register("singleturn_count", rw="r", type="unsigned", width=32, desc="Absolute (after index) singleturn count"))    # scaled to 32 bits
        self.encoder_group.add(Register("commutation_count", rw="r", type="unsigned", width=16, desc="Absolute commutation count"))   # scaled to 16 bits
        self.encoder_group.add(Register("config", rw="rw", type="unsigned", width=32, desc="Configuration register", sub_registers=[
            Register("rs485_mode", type="bool", desc="Encoder uses 2-wire RS485 mode instead of 4-wire RS422"),
            Register("encoder_type", type="unsigned", width=4, desc="Encoder type"),
            """
            rs422 mode types:
            0: default

            rs485 mode types:
            0: ai64
            1: ai128

            """
        ]))
        
        self.encoder_group.add(Register("status", rw="r", desc="Encoder status", sub_registers=[
            Register("battery_fail", type="bool", desc="Battery fail"),
            Register("unindexed", type="bool", desc="Unindexed"),
            Register("no_response", type="bool", desc="No response"),
            Register("crc_fail", type="bool", desc="CRC fail"),
            Register("done", type="bool", desc="Done"),
        ]))

        self.rm.add(Register("trigger", rw="w", type="unsigned", width=32, desc="Trigger encoder capture, per bit for each encoder"))

        self.rm.add(self.encoder_group)
        self.rm.generate()

        self.encoders = []

    def elaborate(self, platform):
        m = Module()
        self.synced_rx = Signal(self.number_of_encoders)

        m.submodules += FFSynchronizer(i=self.rx, o=self.synced_rx, o_domain="sync_100")

        for i in range(self.number_of_encoders):

            encoder = Fanuc_Encoder()

            m.submodules[f"encoder_{i}"] = encoder

            self.encoders.append(encoder)


        # system regs
        with m.If(self.bram_write_enable & (self.bram_address == self.rm.trigger.address_offset)):
            for i in range(self.number_of_encoders):
                m.d.sync_100 += self.encoders[i].trigger.eq(self.bram_write_data.bit_select(i, 1))
        with m.Else():
            for i in range(self.number_of_encoders):
                m.d.sync_100 += self.encoders[i].trigger.eq(0)


        # encoder regs

        encoder_address_lsb = int(math.log2(self.rm.encoder.alignment)) # TODO: add otion to get these directly from the register map
        encoder_address_msb = int(math.log2(self.rm.encoder.count)) + encoder_address_lsb + 1

        for index, e in enumerate(self.encoders):

            # debug connections
            if index == 4:
                m.d.comb += self.debug[0].eq(e.tx)
                m.d.comb += self.debug[1].eq(e.tx_enable)
                m.d.comb += self.debug[2].eq(e.rx)
            if index == 5:
                m.d.comb += self.debug[3].eq(e.tx)
                m.d.comb += self.debug[4].eq(e.tx_enable)
                m.d.comb += self.debug[5].eq(e.rx)
            
            with m.If(self.bram_address[encoder_address_lsb:encoder_address_msb] == index<<encoder_address_lsb):  # selected the encoder
                # TODO: add a way to create these switches automatically from the register map
                with m.Switch(self.bram_address[0:encoder_address_lsb]):    # select the encoder register
                    with m.Case(self.rm.encoder.multiturn_count.address_offset):
                        m.d.sync_100 += self.bram_read_data.eq(e.multiturn_count)
                    with m.Case(self.rm.encoder.singleturn_count.address_offset):
                        m.d.sync_100 += self.bram_read_data.eq(e.singleturn_count)
                    with m.Case(self.rm.encoder.commutation_count.address_offset):
                        m.d.sync_100 += self.bram_read_data.eq(e.commutation_count)
                    with m.Case(self.rm.encoder.status.address_offset):
                        m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.battery_fail.starting_bit].eq(e.battery_fail)
                        m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.no_response.starting_bit].eq(e.no_response)
                        m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.crc_fail.starting_bit].eq(e.crc_fail)
                        m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.done.starting_bit].eq(e.done)
                        m.d.sync_100 += self.bram_read_data[self.rm.encoder.status.unindexed.starting_bit].eq(e.unindexed)
                    with m.Case(self.rm.encoder.config.address_offset):
                        m.d.sync_100 += self.bram_read_data[self.rm.encoder.config.rs485_mode.starting_bit].eq(e.rs485_mode)
                        m.d.sync_100 += self.bram_read_data[self.rm.encoder.config.encoder_type.starting_bit: self.rm.encoder.config.encoder_type.starting_bit + self.rm.encoder.config.encoder_type.width].eq(e.encoder_type)
                        with m.If(self.bram_write_enable):
                            m.d.sync_100 += e.rs485_mode.eq(self.bram_write_data[self.rm.encoder.config.rs485_mode.starting_bit])
                            m.d.sync_100 += e.encoder_type.eq(self.bram_write_data[self.rm.encoder.config.encoder_type.starting_bit: self.rm.encoder.config.encoder_type.starting_bit + self.rm.encoder.config.encoder_type.width])
                    with m.Default():
                        m.d.sync_100 += self.bram_read_data.eq(0)


        return m

class Request_Signal(Component):

    def __init__(self):
        super().__init__({
            "trigger": In(1),
            "tx": Out(1),
            "tx_enable": Out(1),
            "trigger_out": Out(1),  # starts receiver capture
            "mode": In(1),   # 0=rs422, 1=rs485
            "request_byte": In(9),
            "request_length": In(4),    # number of bits to send
        })

        #self.rs422_baud_rate = 1.024e6
        self.rs422_baud_rate = 125e3    # use one bit as the 8us pulse, may change once we figure out additional commands
        self.rs485_baud_rate = 2.7e6
        self.rs485_txen_delay = 480e-9  # 480ns delay between tx enable/disable and data

        self.clock = 100e6

        self.rs422_bit_ticks = int(self.clock / self.rs422_baud_rate)
        self.rs485_bit_ticks = int(self.clock / self.rs485_baud_rate)
        if(self.rs422_baud_rate != self.clock / self.rs422_bit_ticks):
            rs422_error = (abs(self.rs422_baud_rate - self.clock / self.rs422_bit_ticks) / self.rs422_baud_rate) * 100
            print(f"Warning: rs422 baud rate is not an integer divisor of the clock, timing will be slightly off ({self.rs422_baud_rate:.0f} vs {self.clock / self.rs422_bit_ticks:.0f}, error: {rs422_error:.1f}%)")
        if(self.rs485_baud_rate != self.clock / self.rs485_bit_ticks):
            rs485_error = (abs(self.rs485_baud_rate - self.clock / self.rs485_bit_ticks) / self.rs485_baud_rate) * 100
            print(f"Warning: rs485 baud rate is not an integer divisor of the clock, timing will be slightly off ({self.rs485_baud_rate:.0f} vs {self.clock / self.rs485_bit_ticks:.0f}, error: {rs485_error:.1f}%)")


    def elaborate(self, platform):
        m = Module()

        rs485_txen_delay_ticks = int(self.clock * self.rs485_txen_delay)

        cnt = Signal(range(max(self.rs422_bit_ticks, rs485_txen_delay_ticks)+1))
        cnt_reset = Signal()

        bit_ticks = Signal(cnt.shape())

        bit_index = Signal(self.request_length.shape())

        m.d.comb += bit_ticks.eq(Mux(self.mode == 0, self.rs422_bit_ticks-1, self.rs485_bit_ticks-1))

        with m.If(cnt_reset):
            m.d.sync_100 += cnt.eq(0)
        with m.Else():
            m.d.sync_100 += cnt.eq(cnt + 1)

        with m.If(self.trigger_out):
            m.d.sync_100 += self.trigger_out.eq(0)

        with m.FSM(init="IDLE", domain="sync_100"):
            with m.State("IDLE"):
                m.d.comb += self.tx_enable.eq(~self.mode)   # always enable for rs422
                m.d.comb += self.tx.eq(0)
                with m.If(self.trigger):
                    m.d.comb += cnt_reset.eq(1)
                    m.d.sync_100 += bit_index.eq(0)
                    with m.If(self.mode == 0):   # rs422
                        m.next = "TRANSMIT"
                    with m.Else():  # rs485
                        m.next = "RS485_START"  

            with m.State("RS485_START"):    # delay after enabling tx before sending data
                m.d.comb += self.tx_enable.eq(1)
                m.d.comb += self.tx.eq(1)   # start level high
                with m.If(cnt == rs485_txen_delay_ticks):
                    m.d.comb += cnt_reset.eq(1)
                    m.next = "TRANSMIT"

            with m.State("TRANSMIT"):   # send bits
                m.d.comb += self.tx_enable.eq(1)
                m.d.comb += self.tx.eq(self.request_byte.bit_select(bit_index, 1))
                with m.If(cnt == bit_ticks):
                    m.d.comb += cnt_reset.eq(1)
                    with m.If(bit_index == self.request_length - 1):
                        with m.If(self.mode == 0):   # rs422
                            m.d.sync_100 += self.trigger_out.eq(1)
                            m.next = "IDLE"
                        with m.Else():  # rs485
                            m.next = "RS485_STOP"
                    with m.Else():
                        m.d.sync_100 += bit_index.eq(bit_index + 1)

            with m.State("RS485_STOP"): # delay before disabling tx
                m.d.comb += self.tx_enable.eq(1)
                m.d.comb += self.tx.eq(1)   # stop level high
                with m.If(cnt == rs485_txen_delay_ticks):
                    m.d.sync_100 += self.trigger_out.eq(1)
                    m.next = "IDLE"

        return m

class Fanuc_rs422_CRC(Component):
    def __init__(self):
        super().__init__({
            'input': In(1),
            'strobe': In(1),
            'crc_ok': Out(1),
            'reset': In(1),
        })

    def elaborate(self, platform):
        m = Module()

        shreg = Signal(5)
        xor = Signal(5)

        m.d.comb += self.crc_ok.eq(shreg == 0)

        with m.If(shreg[-1]):
            m.d.comb += xor.eq(0b01011)

        with m.If(self.strobe):
            m.d.sync_100 += shreg.eq(Cat(self.input, shreg) ^ xor)

        with m.If(self.reset):
            m.d.sync_100 += shreg.eq(0)

        return m

class Receiver(Component):
    def __init__(self):
        super().__init__({
            'rx': In(1),
            "trigger": In(1),

            "rs485_mode": In(1),
            "encoder_type": In(4),

            "multiturn_count": Out(16),
            "singleturn_count": Out(32),
            "commutation_count": Out(16),
            "battery_fail": Out(1),
            "unindexed": Out(1),

            "no_response": Out(1),
            "crc_fail": Out(1),
            "done": Out(1),
        })

        self.rs422_baud_rate = 1.024e6
        self.rs485_baud_rate = 2.7e6

        self.clock = 100e6

        self.rs422_bit_ticks = int(self.clock / self.rs422_baud_rate)
        self.rs485_bit_ticks = int(self.clock / self.rs485_baud_rate)
        if(self.rs422_baud_rate != self.clock / self.rs422_bit_ticks):
            rs422_error = (abs(self.rs422_baud_rate - self.clock / self.rs422_bit_ticks) / self.rs422_baud_rate) * 100
            print(f"Warning: rs422 baud rate is not an integer divisor of the clock, timing will be slightly off ({self.rs422_baud_rate:.0f} vs {self.clock / self.rs422_bit_ticks:.0f}, error: {rs422_error:.1f}%)")
        if(self.rs485_baud_rate != self.clock / self.rs485_bit_ticks):
            rs485_error = (abs(self.rs485_baud_rate - self.clock / self.rs485_bit_ticks) / self.rs485_baud_rate) * 100
            print(f"Warning: rs485 baud rate is not an integer divisor of the clock, timing will be slightly off ({self.rs485_baud_rate:.0f} vs {self.clock / self.rs485_bit_ticks:.0f}, error: {rs485_error:.1f}%)")


        self.capture = Signal()
        self.cnt = Signal(range(2 * max(self.rs422_bit_ticks, self.rs485_bit_ticks)))
        self.idx = Signal(range(96))
        self.state = Signal(3)

        
        self.prev_rx = Signal()

    def elaborate(self, platform):
        m = Module()

        m.submodules.crc = crc = Fanuc_rs422_CRC()

        bit_ticks_3_2 = Signal(self.cnt.shape())
        bit_ticks_1_1 = Signal(self.cnt.shape())
        bit_ticks_1_2 = Signal(self.cnt.shape())

        m.d.sync_100 += self.prev_rx.eq(self.rx)

        buf = Array(Signal(name=f"buf_{_}") for _ in range(76))

        with m.If(self.trigger):
            m.d.sync_100 += self.no_response.eq(1)

        with m.If(self.rs485_mode):
            m.d.comb += bit_ticks_3_2.eq(self.rs485_bit_ticks + self.rs485_bit_ticks // 2 - 1)
            m.d.comb += bit_ticks_1_1.eq(self.rs485_bit_ticks - 1)
            m.d.comb += bit_ticks_1_2.eq(self.rs485_bit_ticks // 2 - 1)
        with m.Else():
            m.d.comb += bit_ticks_3_2.eq(self.rs422_bit_ticks + self.rs422_bit_ticks // 2 - 1)
            m.d.comb += bit_ticks_1_1.eq(self.rs422_bit_ticks - 1)
            m.d.comb += bit_ticks_1_2.eq(self.rs422_bit_ticks // 2 - 1)
        
        with m.FSM(init="WAIT_START", domain="sync_100"):
            with m.State('WAIT_START'):
                with m.If(self.prev_rx & (~self.rx)): # falling edge
                    m.d.sync_100 += self.done.eq(0)
                    m.d.sync_100 += self.no_response.eq(0)   # we got at least one edge
                    m.d.sync_100 += self.cnt.eq(bit_ticks_3_2)
                    m.d.sync_100 += self.idx.eq(0)
                    m.next = 'CAPTURE'

            with m.State('CAPTURE'):
                with m.If(self.trigger):    # reset on trigger
                    m.next = 'WAIT_START'

                with m.If(self.cnt == 0):
                    m.d.comb += self.capture.eq(1)
                    m.d.sync_100 += self.cnt.eq(bit_ticks_1_1)
                    m.d.sync_100 += self.idx.eq(self.idx + 1)

                with m.Elif(self.rx != self.prev_rx):
                    m.d.sync_100 += self.cnt.eq(bit_ticks_1_2)

                with m.Else():
                    m.d.sync_100 += self.cnt.eq(self.cnt - 1)

                with m.If(self.idx == 76):
                    m.next = 'DONE'

            with m.State('DONE'):
                m.d.sync_100 += self.done.eq(1)
                with m.If(self.no_response):    # no response
                    pass

                with m.Elif(~crc.crc_ok):   # crc fail
                    m.d.sync_100 += self.crc_fail.eq(1)

                with m.Else():  # valid response
                    m.d.sync_100 += self.crc_fail.eq(0)
                    
                    '''
                    bits 0..4 	constant : = 0b00101
                    bit  5     	1=battery fail
                    bits 6,7	unknown = 0b10,a860-360 0b00,a860-370 
                    bit  8		1=un-indexed
                    bits 9..17	unknown, perhaps for higher res encoders
                    bits 18..33	16 bit absolute encoder data (0..65535 for one turn)
                    bits 34..35     unknown = 0b01
                    bits 36..51	16 bit absolute turns count
                    bits 52,53	unknown = 0b01
                    bits 54..63	10 bit absolute commutation encoder (four 0->1023 cycles per turn) (is it always 4 or is it matched to the motor poles?)
                    '''
                    m.d.sync_100 += [
                        self.multiturn_count.eq(Cat(buf[36:52])),
                        self.singleturn_count.eq(Cat(buf[18:34])),
                        self.commutation_count.eq(Cat(buf[54:64])),
                        self.battery_fail.eq(Cat(buf[5])),
                        self.unindexed.eq(Cat(buf[8])),
                    ]

                with m.If(~self.trigger):   # wait until trigger resets to prevent freerunning
                    m.next = "WAIT_START"
        
        with m.If(self.capture):
            m.d.sync_100 += buf[self.idx].eq(self.rx)

        m.d.comb += [
            crc.input.eq(self.rx),
            crc.strobe.eq(self.capture),
            crc.reset.eq(self.done),
        ]

        return m

class Fanuc_Encoder(Component): # encapsulates both the request signal and receiver for one encoder and handles mode/type configuration
    def __init__(self):
        super().__init__({
            "rx": In(1),
            "tx": Out(1),
            "tx_enable": Out(1),

            "rs485_mode": In(1),
            "encoder_type": In(4),
            "cmd": In(9),   # for future use, currently unused

            "trigger": In(1),

            "multiturn_count": Out(16),
            "singleturn_count": Out(32),
            "commutation_count": Out(16),
            "battery_fail": Out(1),
            "unindexed": Out(1),

            "no_response": Out(1),
            "crc_fail": Out(1),
            "done": Out(1),
        })

    def elaborate(self, platform):
        m = Module()

        m.submodules.request_signal = request_signal = Request_Signal()
        m.submodules.receiver = receiver = Receiver()

        m.d.comb += [
            request_signal.trigger.eq(self.trigger),
            request_signal.mode.eq(self.rs485_mode),

            self.tx.eq(request_signal.tx),
            self.tx_enable.eq(request_signal.tx_enable),

            receiver.rx.eq(self.rx),
            receiver.trigger.eq(request_signal.trigger_out),
            receiver.rs485_mode.eq(self.rs485_mode),
            receiver.encoder_type.eq(self.encoder_type)
        ]

        with m.Switch(self.rs485_mode):
            with m.Case(0):   # rs422 mode
                with m.Switch(self.encoder_type):
                    with m.Default():   # all the same for now
                        m.d.comb += [
                            request_signal.request_byte.eq(0b1),
                            request_signal.request_length.eq(1)
                        ]

            with m.Case(1):   # rs485 mode
                with m.Switch(self.encoder_type):
                    with m.Default():   # all the same for now
                        m.d.comb += [
                            request_signal.request_byte.eq(0b001110000),
                            request_signal.request_length.eq(9)
                        ]

        m.d.comb += [
            self.multiturn_count.eq(receiver.multiturn_count),
            self.singleturn_count.eq(receiver.singleturn_count),
            self.commutation_count.eq(receiver.commutation_count),
            self.battery_fail.eq(receiver.battery_fail),
            self.unindexed.eq(receiver.unindexed),
            self.no_response.eq(receiver.no_response),
            self.crc_fail.eq(receiver.crc_fail),
            self.done.eq(receiver.done),
        ]

        return m

request_signal_dut = Request_Signal()
async def request_signal_bench(ctx):

    # rs422 mode
    print("Testing rs422 mode")

    test_rs422_encoder = rs422_sim("controller-firmware/python/src/sandbox/fanuc_encoder_rs422.csv", 100e6)

    ctx.set(request_signal_dut.mode, 0)   # rs422
    ctx.set(request_signal_dut.request_byte, 0b1)   # just one bit
    ctx.set(request_signal_dut.request_length, 1)
    await ctx.tick("sync_100")
    ctx.set(request_signal_dut.trigger, 1)
    await ctx.tick("sync_100")
    ctx.set(request_signal_dut.trigger, 0)

    for i in range(1000):
        if(ctx.get(request_signal_dut.tx_enable) != 1):
            print("Error: tx_enable should always be 1 in rs422 mode")
        
        test_rs422_encoder.set_request_level(ctx.get(request_signal_dut.tx))
        test_rs422_encoder.tick()
        await ctx.tick("sync_100")


    # rs485 mode
    print("Testing rs485 mode")

    test_rs485_encoder = rs485_sim("controller-firmware/python/src/sandbox/fanuc_rs485_detected_edges.csv", 100e6)

    ctx.set(request_signal_dut.mode, 1)   # rs485
    ctx.set(request_signal_dut.request_byte, 0b001110000)   # 9 bits
    ctx.set(request_signal_dut.request_length, 9)
    await ctx.tick("sync_100")
    ctx.set(request_signal_dut.trigger, 1)
    await ctx.tick("sync_100")
    ctx.set(request_signal_dut.trigger, 0)
    
    for i in range(1000):
        
        test_rs485_encoder.set_inputs(ctx.get(request_signal_dut.tx), ctx.get(request_signal_dut.tx_enable))
        test_rs485_encoder.tick()
        await ctx.tick("sync_100")

single_encoder_dut = Fanuc_Encoder()
async def single_encoder_bench(ctx):

    # rs422 mode
    print("Testing rs422 mode")

    test_rs422_encoder = rs422_sim("controller-firmware/python/src/sandbox/fanuc_encoder_rs422.csv", 100e6)

    ctx.set(single_encoder_dut.rs485_mode, 0)   # rs422
    await ctx.tick("sync_100")
    ctx.set(single_encoder_dut.trigger, 1)
    await ctx.tick("sync_100")
    ctx.set(single_encoder_dut.trigger, 0)

    for i in range(10000):
        if(ctx.get(single_encoder_dut.tx_enable) != 1):
            print("Error: tx_enable should always be 1 in rs422 mode")
        
        test_rs422_encoder.set_request_level(ctx.get(single_encoder_dut.tx))
        ctx.set(single_encoder_dut.rx, test_rs422_encoder.get_tx_level())
        test_rs422_encoder.tick()
        await ctx.tick("sync_100")


    # rs485 mode
    print("Testing rs485 mode")

    test_rs485_encoder = rs485_sim("controller-firmware/python/src/sandbox/fanuc_rs485_detected_edges.csv", 100e6)

    ctx.set(single_encoder_dut.rs485_mode, 1)   # rs485
    await ctx.tick("sync_100").repeat(100)
    ctx.set(single_encoder_dut.trigger, 1)
    await ctx.tick("sync_100")
    ctx.set(single_encoder_dut.trigger, 0)
    
    for i in range(7000):
        level = test_rs485_encoder.get_tx_level()
        if level == 1:
            ctx.set(single_encoder_dut.rx, 1)
        elif level == 0:
            ctx.set(single_encoder_dut.rx, 0)
        
        ctx.set(single_encoder_dut.rx, test_rs485_encoder.get_tx_level())
        test_rs485_encoder.set_inputs(ctx.get(single_encoder_dut.tx), ctx.get(single_encoder_dut.tx_enable))
        test_rs485_encoder.tick()
        await ctx.tick("sync_100")


if __name__ == "__main__":

    # sim = Simulator(dut)
    sim = Simulator(single_encoder_dut)
    sim.add_clock(1/100e6, domain="sync_100")
    sim.add_testbench(single_encoder_bench)
    with sim.write_vcd("fanuc_single_encoder_test.vcd"):
        sim.run()
