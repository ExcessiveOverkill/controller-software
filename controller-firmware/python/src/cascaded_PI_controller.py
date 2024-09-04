from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog
from amaranth.hdl import Array
from src.convergent_round import convergentRound
from src.biquad import biquad_32


class cascaded_PI_controller(Elaboratable):


        
    def __init__(self, instances) -> None:

        self.instances = instances
        

        # Ports
        self.address = Signal(16)
        self.writeData = Signal(32)
        self.readData = Signal(32)
        self.writeEnable = Signal()

        # internal signals
        self.selectedInstance = Signal(range(self.instances))

        self.startUpdate = Signal()
        self.updateDone = Signal()

        # per-instance signals
        self.positionCmd = Array(Signal(shape=signed(64), name=f"posCMD_{_}") for _ in range(self.instances))
        self.positionCmdOld = Array(Signal(shape=signed(64), name=f"posCMDold_{_}") for _ in range(self.instances))
        self.positionFbk = Array(Signal(shape=signed(64), name=f"posFBK_{_}") for _ in range(self.instances))
        self.positionError = Array(Signal(shape=signed(64), name=f"posError_{_}") for _ in range(self.instances))
        self.positionIntegralLimit = Array(Signal(shape=signed(32), name=f"posIlim_{_}") for _ in range(self.instances))
        self.positionIntegral = Array(Signal(shape=signed(32), name=f"posIntegral_{_}") for _ in range(self.instances))
        self.positionIntegralGain = Array(Signal(shape=signed(32), name=f"posIgain_{_}") for _ in range(self.instances))
        self.positionProportionalGain = Array(Signal(shape=signed(32), name=f"posPgain_{_}") for _ in range(self.instances))

        self.velocityCmd = Array(Signal(shape=signed(32), name=f"velCMD_{_}") for _ in range(self.instances))
        self.velocityFFCmd = Array(Signal(shape=signed(32), name=f"velFFCMD_{_}") for _ in range(self.instances))
        self.velocityFbk = Array(Signal(shape=signed(32), name=f"velFBK_{_}") for _ in range(self.instances))
        self.estimateVelocityFbk = Array(Signal(1, name=f"velEstimateFbkEnable_{_}") for _ in range(self.instances))
        self.velocityError = Array(Signal(shape=signed(32), name=f"velError_{_}") for _ in range(self.instances))
        self.velocityIntegralLimit = Array(Signal(shape=signed(32), name=f"velIlim_{_}") for _ in range(self.instances))
        self.velocityIntegral = Array(Signal(shape=signed(32), name=f"velIntegral_{_}") for _ in range(self.instances))
        self.velocityIntegralGain = Array(Signal(shape=signed(32), name=f"velIgain_{_}") for _ in range(self.instances))
        self.velocityProportionalGain = Array(Signal(shape=signed(32), name=f"velPgain_{_}") for _ in range(self.instances))
        self.velocityFilterZeroCoeff_0 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.velocityFilterZeroCoeff_1 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.velocityFilterZeroCoeff_2 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.velocityFilterPoleCoeff_1 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.velocityFilterPoleCoeff_2 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.velocityFilterDn_0 = Array(Signal(shape=signed(48)) for _ in range(self.instances))
        self.velocityFilterDn_1 = Array(Signal(shape=signed(48)) for _ in range(self.instances))
        
        self.torqueFFCmd = Array(Signal(shape=signed(32)) for _ in range(self.instances))
        self.torqueCmd = Array(Signal(shape=signed(32)) for _ in range(self.instances))
        self.torqueFilterZeroCoeff_0 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.torqueFilterZeroCoeff_1 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.torqueFilterZeroCoeff_2 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.torqueFilterPoleCoeff_1 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.torqueFilterPoleCoeff_2 = Array(Signal(shape=signed(36)) for _ in range(self.instances))
        self.torqueFilterDn_0 = Array(Signal(shape=signed(48)) for _ in range(self.instances))
        self.torqueFilterDn_1 = Array(Signal(shape=signed(48)) for _ in range(self.instances))

    def elaborate(self, platform):
        m = Module()

        m.submodules.rounder = self.rounder = convergentRound(48+32, 48)
        m.submodules.filter = self.filter = biquad_32(signed_=True, constrainOutput_=True)
        
        # handle reading/writing data

        # update loops

        self.velAccum = Signal(shape=signed(64))

        self.multiplier = Signal(shape=signed(48+32))
        self.multA = Signal(shape=signed(32))
        self.multB = Signal(shape=signed(48))

        self.addAccum = Signal(shape=signed(34))

        m.d.comb += self.multiplier.eq(self.multA * self.multB)


        with m.FSM(init="done"):
            with m.State("done"):
                m.d.sync += self.updateDone.eq(1)
                with m.If(self.startUpdate):
                    m.d.sync += self.updateDone.eq(0)
                    m.next = "prepare_pos_loop"

            #### POSITION LOOP ####

            with m.State("prepare_pos_loop"):
                m.d.sync += self.positionError[self.selectedInstance].eq(self.positionFbk[self.selectedInstance] - self.positionCmd[self.selectedInstance])
                
                with m.If(self.estimateVelocityFbk[self.selectedInstance]):
                   m.d.sync += self.velocityFbk[self.selectedInstance].eq(self.positionCmd[self.selectedInstance] - self.positionCmdOld[self.selectedInstance])

                m.d.sync += self.positionCmdOld[self.selectedInstance].eq(self.positionCmd[self.selectedInstance])
                m.d.sync += self.addAccum.eq(self.velocityFFCmd[self.selectedInstance])
                m.next = "pos_p_gain_mult"

            with m.State("pos_p_gain_mult"):
                m.d.sync += self.multA.eq(self.positionError[self.selectedInstance])
                m.d.sync += self.multB.eq(self.positionProportionalGain[self.selectedInstance])
                m.next = "pos_p_gain_round"

            with m.State("pos_p_gain_round"):
                m.d.comb += self.rounder.input.eq(self.multiplier.shift_left(1))

                m.next = "pos_p_gain_overflow_check"

            with m.State("pos_p_gain_overflow_check"):
                with m.If(self.rounder.output > 2**31-1):   # positive overflow
                    m.d.sync += self.addAccum.eq(self.addAccum + 2**31-1)
                with m.Elif(self.rounder.output < -2**31):   # negative overflow
                    m.d.sync += self.addAccum.eq(self.addAccum - 2**31)
                with m.Else():
                    m.d.sync += self.addAccum.eq(self.addAccum + self.rounder.output)

                m.next = "pos_i_gain_mult"

            with m.State("pos_i_gain_mult"):
                m.d.sync += self.multA.eq(self.positionError[self.selectedInstance])
                m.d.sync += self.multB.eq(self.positionIntegralGain[self.selectedInstance])
                m.next = "pos_i_gain_round"

            with m.State("pos_i_gain_round"):
                m.d.comb += self.rounder.input.eq(self.multiplier.shift_left(1))

                m.next = "pos_i_accum"

            with m.State("pos_i_accum"):
                m.d.sync += self.positionIntegral[self.selectedInstance].eq(self.positionIntegral[self.selectedInstance] + self.rounder.output)

                m.next = "pos_i_limit_check"

            with m.State("pos_i_limit_check"):
                with m.If(self.positionIntegral[self.selectedInstance] > self.positionIntegralLimit[self.selectedInstance]):   # positive limit reached
                    m.d.sync += self.positionIntegral[self.selectedInstance].eq(2**31-1)
                with m.Elif(self.positionIntegral[self.selectedInstance] < -self.positionIntegralLimit[self.selectedInstance]):   # negative limit reached
                    m.d.sync += self.positionIntegral[self.selectedInstance].eq(-2**31)
                
                m.next = "vel_cmd_accum"

            with m.State("vel_cmd_accum"):
                m.d.sync += self.addAccum.eq(self.addAccum + self.positionIntegral[self.selectedInstance])

                m.next = "vel_cmd_accum_overflow_check"

            with m.State("vel_cmd_accum_overflow_check"):
                with m.If(self.addAccum > 2**31-1):   # positive overflow
                    m.d.sync += self.velocityCmd[self.selectedInstance].eq(2**31-1)
                with m.Elif(self.rounder.output < -2**31):   # negative overflow
                    m.d.sync += self.velocityCmd[self.selectedInstance].eq(-2**31)
                with m.Else():
                    m.d.sync += self.velocityCmd[self.selectedInstance].eq(self.addAccum)

                m.next = "prepare_vel_loop"


            #### VELOCITY LOOP ####


            with m.State("prepare_vel_loop"):
                m.d.sync += self.filter.input.eq(self.velocityFbk[self.selectedInstance] - self.velocityCmd[self.selectedInstance])
                m.d.sync += self.filter.dn_0.eq(self.velocityFilterDn_0[self.selectedInstance])
                m.d.sync += self.filter.dn_1.eq(self.velocityFilterDn_1[self.selectedInstance])
                m.d.sync += self.filter.poleCoeff_1.eq(self.velocityFilterPoleCoeff_1[self.selectedInstance])
                m.d.sync += self.filter.poleCoeff_2.eq(self.velocityFilterPoleCoeff_2[self.selectedInstance])
                m.d.sync += self.filter.zeroCoeff_0.eq(self.velocityFilterZeroCoeff_0[self.selectedInstance])
                m.d.sync += self.filter.zeroCoeff_1.eq(self.velocityFilterZeroCoeff_1[self.selectedInstance])
                m.d.sync += self.filter.zeroCoeff_2.eq(self.velocityFilterZeroCoeff_2[self.selectedInstance])
                m.d.sync += self.filter.update.eq(1)

                m.d.sync == self.addAccum.eq(self.torqueFFCmd[self.selectedInstance])

                m.next = "vel_error_filter"

            with m.State("vel_error_filter"):
                m.d.sync += self.filter.update.eq(0)
                with m.If(self.filter.updateDone):
                    m.d.sync += self.velocityFilterDn_0[self.selectedInstance].eq(self.filter.dn_0)
                    m.d.sync += self.velocityFilterDn_1[self.selectedInstance].eq(self.filter.dn_1)
                    m.d.sync += self.velocityError[self.selectedInstance].eq(self.filter.output)
                    m.next = "vel_p_gain_mult"

            with m.State("vel_p_gain_mult"):
                m.d.sync += self.multA.eq(self.velocityError[self.selectedInstance])
                m.d.sync += self.multB.eq(self.velocityProportionalGain[self.selectedInstance])
                m.next = "vel_p_gain_round"

            with m.State("vel_p_gain_round"):
                m.d.comb += self.rounder.input.eq(self.multiplier.shift_left(1))

                m.next = "vel_p_gain_overflow_check"

            with m.State("vel_p_gain_overflow_check"):
                with m.If(self.rounder.output > 2**31-1):   # positive overflow
                    m.d.sync += self.addAccum.eq(self.addAccum + 2**31-1)
                with m.Elif(self.rounder.output < -2**31):   # negative overflow
                    m.d.sync += self.addAccum.eq(self.addAccum - 2**31)
                with m.Else():
                    m.d.sync += self.addAccum.eq(self.addAccum + self.rounder.output)

                m.next = "vel_i_gain_mult"

            with m.State("vel_i_gain_mult"):
                m.d.sync += self.multA.eq(self.velocityError[self.selectedInstance])
                m.d.sync += self.multB.eq(self.velocityIntegralGain[self.selectedInstance])
                m.next = "vel_i_gain_round"

            with m.State("vel_i_gain_round"):
                m.d.comb += self.rounder.input.eq(self.multiplier.shift_left(1))

                m.next = "vel_i_accum"

            with m.State("vel_i_accum"):
                m.d.sync += self.velocityIntegral[self.selectedInstance].eq(self.velocityIntegral[self.selectedInstance] + self.rounder.output)

                m.next = "vel_i_limit_check"

            with m.State("vel_i_limit_check"):
                with m.If(self.velocityIntegral[self.selectedInstance] > self.velocityIntegralLimit[self.selectedInstance]):   # positive limit reached
                    m.d.sync += self.velocityIntegral[self.selectedInstance].eq(2**31-1)
                with m.Elif(self.velocityIntegral[self.selectedInstance] < -self.velocityIntegralLimit[self.selectedInstance]):   # negative limit reached
                    m.d.sync += self.velocityIntegral[self.selectedInstance].eq(-2**31)
                
                m.next = "torque_cmd_accum"

            with m.State("torque_cmd_accum"):
                m.d.sync += self.addAccum.eq(self.addAccum + self.velocityIntegral[self.selectedInstance])

                m.next = "torque_cmd_accum_overflow_check_and_filter"

            with m.State("torque_cmd_accum_overflow_check_and_filter"):
                with m.If(self.addAccum > 2**31-1):   # positive overflow
                    m.d.sync += self.filter.input.eq(2**31-1)
                with m.Elif(self.rounder.output < -2**31):   # negative overflow
                    m.d.sync += self.filter.input.eq(-2**31)
                with m.Else():
                    m.d.sync += self.filter.input.eq(self.addAccum)

                m.d.sync += self.filter.dn_0.eq(self.torqueFilterDn_0[self.selectedInstance])
                m.d.sync += self.filter.dn_1.eq(self.torqueFilterDn_1[self.selectedInstance])
                m.d.sync += self.filter.poleCoeff_1.eq(self.torqueFilterPoleCoeff_1[self.selectedInstance])
                m.d.sync += self.filter.poleCoeff_2.eq(self.torqueFilterPoleCoeff_2[self.selectedInstance])
                m.d.sync += self.filter.zeroCoeff_0.eq(self.torqueFilterZeroCoeff_0[self.selectedInstance])
                m.d.sync += self.filter.zeroCoeff_1.eq(self.torqueFilterZeroCoeff_1[self.selectedInstance])
                m.d.sync += self.filter.zeroCoeff_2.eq(self.torqueFilterZeroCoeff_2[self.selectedInstance])
                m.d.sync += self.filter.update.eq(1)

                m.next = "torque_filter"

            with m.State("torque_filter"):
                m.d.sync += self.filter.update.eq(0)
                with m.If(self.filter.updateDone):
                    m.d.sync += self.torqueFilterDn_0[self.selectedInstance].eq(self.filter.dn_0)
                    m.d.sync += self.torqueFilterDn_1[self.selectedInstance].eq(self.filter.dn_1)
                    m.d.sync += self.torqueCmd[self.selectedInstance].eq(self.filter.output)
                    m.next = "done"


        return m
    

dut = cascaded_PI_controller(1)

async def PIBench(ctx):

    ctx.set(dut.positionProportionalGain[0], 2**31-1)
    ctx.set(dut.positionIntegralGain[0], 2**31-1)
    ctx.set(dut.positionIntegralLimit[0], 2**31-1)
    for i in range(1000):
        ctx.set(dut.positionCmd[0],i)
        ctx.set(dut.startUpdate, 1)
        await ctx.tick()

        while(not ctx.get(dut.updateDone)):
            await ctx.tick()
            ctx.set(dut.startUpdate, 0)
    
    #TODO: write full test bench

            
if __name__ == "__main__":

    sim = Simulator(dut)
    sim.add_clock(1/int(100e6))
    sim.add_testbench(PIBench)
    with sim.write_vcd("cascaded_PI_controller.vcd"):
        sim.run()

    # if (True):  # export
    #     top = cascaded_PI_controller(1)
    #     with open("controller-firmware/src/amaranth sources/cascaded_PI_controller.v", "w") as f:
    #         f.write(verilog.convert(top, name="cascaded_PI_controller", ports=[top.ports]))