from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog
from enum import IntEnum, auto


class serial_interface_card(Elaboratable):

    def __init__(self, clock) -> None:

        # external slot connections
        self.slotOut = Signal(22)
        self.slotOutEnable = Signal(22)
        self.slotIn = Signal(22)

        """
        0   -   1P
        1   -   1N
        2   -   2P
        3   -   2N
        4   -   3P
        5   -   3N
        6   -   4P
        7   -   4N
        8   -   5P
        9   -   5N
        10  -   6P
        11  -   6N
        12  -   7P
        13  -   7N
        14  -   8P
        15  -   8N
        16  -   9P
        17  -   9N
        18  -   10P
        19  -   10N
        20  -   11P
        21  -   11N
        """

        # config signals
        self.sdaIn = Signal()
        self.sdaOut = Signal()
        self.sdaOutEn = Signal()
        self.sclIn = Signal()
        self.sclOut = Signal()
        self.sclOutEn = Signal()

        # rs485/422 mode
        self.rs485rxtx = Array(Signal(name=f"rs485_RXTX_{_}") for _ in range(10))
        self.rs485txen = Array(Signal(name=f"rs485_TX_enable_{_}") for _ in range(10))

        # rs422 mode
        self.rs422rx = Array(Signal(name=f"rs422_RX_{_}") for _ in range(10))
        self.rs422tx = Array(Signal(name=f"rs422_TX_{_}") for _ in range(10))

        # quadrature encoder mode
        self.quadratureA = Array(Signal(name=f"quadrature_A_{_}") for _ in range(7))
        self.quadratureB = Array(Signal(name=f"quadrature_B_{_}") for _ in range(7))
        self.quadratureZ = Array(Signal(name=f"quadrature_Z_{_}") for _ in range(6))   # the final quadrature input does not have a Z input, hence there are only 6 Z signals but 7 A/B signals

    class pins(IntEnum):
        PIN_1P = 0
        PIN_1N = auto()
        PIN_2P = auto()
        PIN_2N = auto()
        PIN_3P = auto()
        PIN_3N = auto()
        PIN_4P = auto()
        PIN_4N = auto()
        PIN_5P = auto()
        PIN_5N = auto()
        PIN_6P = auto()
        PIN_6N = auto()
        PIN_7P = auto()
        PIN_7N = auto()
        PIN_8P = auto()
        PIN_8N = auto()
        PIN_9P = auto()
        PIN_9N = auto()
        PIN_10P = auto()
        PIN_10N = auto()
        PIN_11P = auto()
        PIN_11N = auto()


    def elaborate(self, platform):
        m = Module()

        pins = self.pins

        # I2C
        m.d.comb += self.sclIn.eq(self.slotIn[pins.PIN_11P])
        m.d.comb += self.sdaIn.eq(self.slotIn[pins.PIN_11N])
        m.d.comb += self.slotOut[pins.PIN_11P].eq(self.sclOut)
        m.d.comb += self.slotOutEnable[pins.PIN_11P].eq(self.sclOutEn)
        m.d.comb += self.slotOut[pins.PIN_11N].eq(self.sdaOut)
        m.d.comb += self.slotOutEnable[pins.PIN_11N].eq(self.sdaOutEn)
        
        # rs485
        m.d.comb += self.slotOutEnable[pins.PIN_10P].eq(self.rs485txen[0])
        with m.If(self.rs485txen[0]):
            m.d.comb += self.slotOut[pins.PIN_10N].eq(self.rs485rxtx[0])
        with m.Else():
            m.d.comb += self.rs485rxtx[0].eq(self.slotIn[pins.PIN_10N])

        m.d.comb += self.slotOutEnable[pins.PIN_5P].eq(self.rs485txen[1])
        with m.If(self.rs485txen[1]):
            m.d.comb += self.slotOut[pins.PIN_5N].eq(self.rs485rxtx[1])
        with m.Else():
            m.d.comb += self.rs485rxtx[1].eq(self.slotIn[pins.PIN_5N])
            
        m.d.comb += self.slotOutEnable[pins.PIN_9P].eq(self.rs485txen[2])
        with m.If(self.rs485txen[2]):
            m.d.comb += self.slotOut[pins.PIN_9N].eq(self.rs485rxtx[2])
        with m.Else():
            m.d.comb += self.rs485rxtx[2].eq(self.slotIn[pins.PIN_9N])
            
        m.d.comb += self.slotOutEnable[pins.PIN_4P].eq(self.rs485txen[3])
        with m.If(self.rs485txen[3]):
            m.d.comb += self.slotOut[pins.PIN_4N].eq(self.rs485rxtx[3])
        with m.Else():
            m.d.comb += self.rs485rxtx[3].eq(self.slotIn[pins.PIN_4N])
            
        m.d.comb += self.slotOutEnable[pins.PIN_8P].eq(self.rs485txen[4])
        with m.If(self.rs485txen[4]):
            m.d.comb += self.slotOut[pins.PIN_8N].eq(self.rs485rxtx[4])
        with m.Else():
            m.d.comb += self.rs485rxtx[4].eq(self.slotIn[pins.PIN_8N])
            
        m.d.comb += self.slotOutEnable[pins.PIN_3P].eq(self.rs485txen[5])
        with m.If(self.rs485txen[5]):
            m.d.comb += self.slotOut[pins.PIN_3N].eq(self.rs485rxtx[5])
        with m.Else():
            m.d.comb += self.rs485rxtx[5].eq(self.slotIn[pins.PIN_3N])
            
        m.d.comb += self.slotOutEnable[pins.PIN_7P].eq(self.rs485txen[6])
        with m.If(self.rs485txen[6]):
            m.d.comb += self.slotOut[pins.PIN_7N].eq(self.rs485rxtx[6])
        with m.Else():
            m.d.comb += self.rs485rxtx[6].eq(self.slotIn[pins.PIN_7N])
            
        m.d.comb += self.slotOutEnable[pins.PIN_2P].eq(self.rs485txen[7])
        with m.If(self.rs485txen[7]):
            m.d.comb += self.slotOut[pins.PIN_2N].eq(self.rs485rxtx[7])
        with m.Else():
            m.d.comb += self.rs485rxtx[7].eq(self.slotIn[pins.PIN_2N])
        
        m.d.comb += self.slotOutEnable[pins.PIN_6P].eq(self.rs485txen[8])
        with m.If(self.rs485txen[8]):
            m.d.comb += self.slotOut[pins.PIN_6N].eq(self.rs485rxtx[8])
        with m.Else():
            m.d.comb += self.rs485rxtx[8].eq(self.slotIn[pins.PIN_6N])
            
        m.d.comb += self.slotOutEnable[pins.PIN_1P].eq(self.rs485txen[9])
        with m.If(self.rs485txen[9]):
            m.d.comb += self.slotOut[pins.PIN_1N].eq(self.rs485rxtx[9])
        with m.Else():
            m.d.comb += self.rs485rxtx[9].eq(self.slotIn[pins.PIN_1N])

        # rs422
        m.d.comb += self.slotOut[pins.PIN_10N].eq(self.rs422tx[0])
        m.d.comb += self.rs422rx[0].eq(self.slotIn[pins.PIN_10P])

        m.d.comb += self.slotOut[pins.PIN_5N].eq(self.rs422tx[1])
        m.d.comb += self.rs422rx[1].eq(self.slotIn[pins.PIN_5P])

        m.d.comb += self.slotOut[pins.PIN_9N].eq(self.rs422tx[2])
        m.d.comb += self.rs422rx[2].eq(self.slotIn[pins.PIN_9P])

        m.d.comb += self.slotOut[pins.PIN_4N].eq(self.rs422tx[3])
        m.d.comb += self.rs422rx[3].eq(self.slotIn[pins.PIN_4P])

        m.d.comb += self.slotOut[pins.PIN_8N].eq(self.rs422tx[4])
        m.d.comb += self.rs422rx[4].eq(self.slotIn[pins.PIN_8P])

        m.d.comb += self.slotOut[pins.PIN_3N].eq(self.rs422tx[5])
        m.d.comb += self.rs422rx[5].eq(self.slotIn[pins.PIN_3P])

        m.d.comb += self.slotOut[pins.PIN_7N].eq(self.rs422tx[6])
        m.d.comb += self.rs422rx[6].eq(self.slotIn[pins.PIN_7P])

        m.d.comb += self.slotOut[pins.PIN_2N].eq(self.rs422tx[7])
        m.d.comb += self.rs422rx[7].eq(self.slotIn[pins.PIN_2P])

        m.d.comb += self.slotOut[pins.PIN_6N].eq(self.rs422tx[8])
        m.d.comb += self.rs422rx[8].eq(self.slotIn[pins.PIN_6P])

        m.d.comb += self.slotOut[pins.PIN_1N].eq(self.rs422tx[9])
        m.d.comb += self.rs422rx[9].eq(self.slotIn[pins.PIN_1P])

        # quadrature
        m.d.comb += self.quadratureA[0].eq(self.slotIn[pins.PIN_10N])
        m.d.comb += self.quadratureB[0].eq(self.slotIn[pins.PIN_10P])
        m.d.comb += self.quadratureZ[0].eq(self.slotIn[pins.PIN_5N])

        m.d.comb += self.quadratureA[1].eq(self.slotIn[pins.PIN_5P])
        m.d.comb += self.quadratureB[1].eq(self.slotIn[pins.PIN_9N])
        m.d.comb += self.quadratureZ[1].eq(self.slotIn[pins.PIN_9P])

        m.d.comb += self.quadratureA[2].eq(self.slotIn[pins.PIN_4N])
        m.d.comb += self.quadratureB[2].eq(self.slotIn[pins.PIN_4P])
        m.d.comb += self.quadratureZ[2].eq(self.slotIn[pins.PIN_8N])

        m.d.comb += self.quadratureA[3].eq(self.slotIn[pins.PIN_8P])
        m.d.comb += self.quadratureB[3].eq(self.slotIn[pins.PIN_3N])
        m.d.comb += self.quadratureZ[3].eq(self.slotIn[pins.PIN_3P])

        m.d.comb += self.quadratureA[4].eq(self.slotIn[pins.PIN_7N])
        m.d.comb += self.quadratureB[4].eq(self.slotIn[pins.PIN_7P])
        m.d.comb += self.quadratureZ[4].eq(self.slotIn[pins.PIN_2N])

        m.d.comb += self.quadratureA[5].eq(self.slotIn[pins.PIN_2P])
        m.d.comb += self.quadratureB[5].eq(self.slotIn[pins.PIN_6N])
        m.d.comb += self.quadratureZ[5].eq(self.slotIn[pins.PIN_6P])

        m.d.comb += self.quadratureA[6].eq(self.slotIn[pins.PIN_1N])
        m.d.comb += self.quadratureB[6].eq(self.slotIn[pins.PIN_1P])

        return m
    
            
if __name__ == "__main__":
    #TODO: make test bench
    pass