from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.wiring import Component, In, Out
from amaranth.lib.cdc import FFSynchronizer

from registers2 import *

from sandbox.fanuc_encoder_sim import rs422_sim



class Fanuc_Encoders(Component):
    # Fanuc encoders interface
    # currently only supports rs422 encoders


    def __init__(self, number_of_encoders: int):

        assert(number_of_encoders > 0 and number_of_encoders <= 32)

        self.number_of_encoders = number_of_encoders

        super().__init__({
            "tx" : Out(self.number_of_encoders),
            "tx_enable" : Out(self.number_of_encoders),
            "rx" : In(self.number_of_encoders),

            "trigger": In(1),

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
        
        self.encoder_group.add(Register("status", rw="r", desc="Encoder status", sub_registers=[
            Register("battery_fail", type="bool", desc="Battery fail"),
            Register("unindexed", type="bool", desc="Unindexed"),
            Register("no_response", type="bool", desc="No response"),
            Register("crc_fail", type="bool", desc="CRC fail"),
            Register("done", type="bool", desc="Done"),
        ]))

        self.rm.add(self.encoder_group)
        self.rm.generate()

        self.encoders = []

    def elaborate(self, platform):
        m = Module()
        self.synced_rx = Signal(self.number_of_encoders)

        m.submodules.request_pulse = request_pulse = Request_Pulse()

        m.submodules += FFSynchronizer(i=self.trigger, o=request_pulse.trigger, o_domain="sync_100")
        m.submodules += FFSynchronizer(i=self.rx, o=self.synced_rx, o_domain="sync_100")
        
        m.d.comb += self.tx_enable.eq(0xffffffff)   # enable all transmitters
        m.d.comb += self.tx.eq(request_pulse.req.replicate(self.number_of_encoders))   # request pulse to all transmitters

        receiver = Fanuc_rs422_Receiver()
        m.submodules += receiver

        m.d.comb += [
            receiver.rx.eq(self.synced_rx[0]),
            receiver.trigger.eq(request_pulse.trigger),
            self.debug.eq(self.bram_address),
        ]

        with m.Switch(self.bram_address[0:4]):
            with m.Case(0):
                m.d.sync_100 += self.bram_read_data.eq(receiver.multiturn_count)
            with m.Case(1):
                m.d.sync_100 += self.bram_read_data.eq(receiver.singleturn_count << 16)
            with m.Case(2):
                m.d.sync_100 += self.bram_read_data.eq(receiver.commutation_count << 6)
            with m.Case(3):
                m.d.sync_100 += self.bram_read_data.eq(receiver.battery_fail | (receiver.unindexed << 1) | (receiver.no_response << 2) | (receiver.crc_fail << 3) | (receiver.done << 4))
            with m.Default():
                m.d.sync_100 += self.bram_read_data.eq(0)


        # with m.Switch(self.bram_address[4:8]):  # up to 32 encoders, with up to 16 data addresses each
        #     for i in range(self.number_of_encoders):
        #         receiver = Fanuc_rs422_Receiver()
        #         m.submodules[f"encoder_{i}"] = receiver
        #         self.encoders.append(receiver)

        #         m.d.comb += [
        #             receiver.rx.eq(self.synced_rx[i]),
        #             receiver.trigger.eq(request_pulse.trigger),
        #         ]

        #         if i == 0:
        #             m.d.comb += [
        #                 self.debug.eq(receiver.commutation_count),
        #             ]

        #         with m.Case(i):
        #             with m.Switch(self.bram_address[0:4]):
        #                 with m.Case(0):
        #                     m.d.sync_100 += self.bram_read_data.eq(receiver.multiturn_count)
        #                 with m.Case(1):
        #                     m.d.sync_100 += self.bram_read_data.eq(receiver.singleturn_count)
        #                 with m.Case(2):
        #                     m.d.sync_100 += self.bram_read_data.eq(receiver.commutation_count)
        #                 with m.Case(3):
        #                     m.d.sync_100 += self.bram_read_data.eq(receiver.battery_fail | (receiver.unindexed << 1) | (receiver.no_response << 2) | (receiver.crc_fail << 3) | (receiver.done << 4))
        #                 with m.Default():
        #                     m.d.sync_100 += self.bram_read_data.eq(0)

        return m

class Request_Pulse(Component):

    def __init__(self):
        self.pulse_width = int(8e-6 * 100e6)  # 8us pulse

        super().__init__({
            "trigger": In(1),
            "req": Out(1)
        })
    
    def elaborate(self, platform):
        m = Module()

        cnt = Signal(range(self.pulse_width))

        m.d.sync_100 += cnt.eq(cnt + 1)

        with m.FSM(init="idle", domain="sync_100"):
            with m.State("idle"):
                with m.If(self.trigger):
                    m.d.sync_100 += cnt.eq(0)
                    m.d.sync_100 += self.req.eq(1)
                    m.next = "pulse"

            with m.State("pulse"):
                with m.If(cnt == self.pulse_width - 1):
                    m.d.sync_100 += self.req.eq(0)
                    with m.If(~self.trigger):   # wait until trigger resets to prevent multiple pulses
                        m.next = "idle"
                
                with m.Else():
                    m.d.sync_100 += cnt.eq(cnt + 1)

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

class Fanuc_rs422_Receiver(Component):

    def __init__(self):
        self.bit_time = int(100e6 / 1024000) # 1.024M Baud

        self.capture = Signal()
        self.cnt = Signal(range(2 * self.bit_time))
        self.idx = Signal(range(96))
        self.state = Signal(3)

        
        self.input_prev = Signal()

        super().__init__({
            'rx': In(1),

            "trigger": In(1),

            "multiturn_count": Out(16),
            "singleturn_count": Out(16),
            "commutation_count": Out(10),
            "battery_fail": Out(1),
            "unindexed": Out(1),

            "no_response": Out(1),
            "crc_fail": Out(1),
            "done": Out(1),
        })

        self.input = self.rx

    def elaborate(self, platform):
        m = Module()

        m.submodules.crc = crc = Fanuc_rs422_CRC()

        

        m.d.sync_100 += self.input_prev.eq(self.input)

        buf = Array(Signal(name=f"buf_{_}") for _ in range(76))

        with m.If(self.trigger):
            m.d.sync_100 += self.no_response.eq(1)

        with m.FSM(init="WAIT_START", domain="sync_100"):
            with m.State('WAIT_START'):
                m.d.comb += self.state.eq(1)
                with m.If(self.input_prev & (~self.input)): # falling edge
                    m.d.sync_100 += self.done.eq(0)
                    m.d.sync_100 += self.no_response.eq(0)   # we got at least one edge
                    m.d.sync_100 += self.cnt.eq(self.bit_time + self.bit_time // 2 - 1)
                    m.d.sync_100 += self.idx.eq(0)
                    m.next = 'CAPTURE'

            with m.State('CAPTURE'):
                m.d.comb += self.state.eq(2)

                with m.If(self.trigger):    # reset on trigger
                    m.next = 'WAIT_START'

                with m.If(self.cnt == 0):
                    m.d.comb += self.capture.eq(1)
                    m.d.sync_100 += self.cnt.eq(self.bit_time - 1)
                    m.d.sync_100 += self.idx.eq(self.idx + 1)

                with m.Elif(self.input != self.input_prev):
                    m.d.sync_100 += self.cnt.eq(self.bit_time // 2 - 1)

                with m.Else():
                    m.d.sync_100 += self.cnt.eq(self.cnt - 1)

                with m.If(self.idx == 76):
                    m.next = 'DONE'

            with m.State('DONE'):
                m.d.comb += self.state.eq(3)
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
            m.d.sync_100 += buf[self.idx].eq(self.input)

        m.d.comb += [
            crc.input.eq(self.input),
            crc.strobe.eq(self.capture),
            crc.reset.eq(self.done),
        ]

        # with m.If(done):
        #     m.d.sync_100 += [
        #         self.raw.eq(Cat(buf)),
        #     ]

        return m
    



dut = Fanuc_Encoders(6)
test_encoder = rs422_sim("controller-firmware/python/src/sandbox/fanuc_encoder_rs422.csv", 100e6)

async def bench(ctx):

    for i in range(2):

        # trigger the request pulse
        await ctx.tick("sync_100").repeat(10)
        ctx.set(dut.trigger, 1)
        await ctx.tick("sync_100")
        ctx.set(dut.trigger, 0)

        #test_encoder.inject_error()

        for j in range(int(100e-6 / (1/100e6))):
            test_encoder.set_request_level(ctx.get(dut.tx[0]))
            ctx.set(dut.rx[0], test_encoder.get_tx_level())
            test_encoder.tick()
            await ctx.tick("sync_100")
    


if __name__ == "__main__":

    sim = Simulator(dut)
    #sim.add_clock(1/25e6, domain="sync_25")
    sim.add_clock(1/100e6, domain="sync_100")
    sim.add_testbench(bench)
    with sim.write_vcd("fanuc_encoders_test.vcd"):
        sim.run()
