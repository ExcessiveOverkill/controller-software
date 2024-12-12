from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog
from enum import IntEnum, auto
from amaranth.lib import wiring
from amaranth.lib.wiring import In, Out
from shift_dma import shift_dma_node
from registers2 import *
from i2c import i2c


class serial_interface_card(wiring.Component):

    def __init__(self) -> None:
        super().__init__({
            "slotOut": Out(22),
            "slotOutEnable": Out(22),
            "slotIn": In(22),

            # "read_node_address_input": In(8),
            # "read_node_address_output": Out(8),
            # "write_node_address_input": In(8),
            # "write_node_address_output": Out(8),
            # "read_bram_address_input": In(16),
            # "read_bram_address_output": Out(16),
            # "write_bram_address_input": In(16),
            # "write_bram_address_output": Out(16),
            # "data_input": In(32),
            # "data_output": Out(32),
            # "read_complete_input": In(1),
            # "read_complete_output": Out(1),
            # "write_complete_input": In(1),
            # "write_complete_output": Out(1),

            "rs485_rx": Out(10),
            "rs485_tx": In(10),
            "rs485_tx_enable": In(10),

            "rs422_rx": Out(10),
            "rs422_tx": In(10),

            "quadrature_A": Out(7),
            "quadrature_B": Out(7),
            "quadrature_Z": Out(6),
            
            "bram_address": Out(16),
            "bram_write_data": Out(32),
            "bram_read_data": In(32),
            "bram_write_enable": Out(1)
        })

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
        # self.desc = RTL_Block("serial_interface_card")
        # self.desc.addCompatibleDriver("serial_interface_card")
        # self.desc.addRegister(Register("port_mode_enable", RegisterDataType.PACKED, ReadWritePermissions.WRITE, "Enable port modes"))
        # self.desc.packInto("port_mode_enable", Register("rs485_mode_enable", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "Enable rs485 mode", width=10))
        # self.desc.packInto("port_mode_enable", Register("rs422_mode_enable", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "Enable rs422 mode", width=10))
        # self.desc.packInto("port_mode_enable", Register("quadrature_mode_enable", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "Enable quadrature mode", width=7))

        # self.desc.addRegister(Register("i2c_config", RegisterDataType.PACKED, ReadWritePermissions.WRITE, "I2c configuration"))
        # self.desc.packInto("i2c_config", Register("read", RegisterDataType.BOOL, ReadWritePermissions.WRITE, "read mode"))
        # self.desc.packInto("i2c_config", Register("device_address", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "Device address", width=7))
        # self.desc.packInto("i2c_config", Register("device_register", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "Device register", width=8))
        # self.desc.packInto("i2c_config", Register("byte_count", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "Byte count", width=3))
        # self.desc.packInto("i2c_config", Register("start", RegisterDataType.BOOL, ReadWritePermissions.WRITE, "Start"))

        # self.desc.addRegister(Register("i2c_data_rx", RegisterDataType.UNSIGNED, ReadWritePermissions.READ, "I2c data rx", width=32))
        # self.desc.addRegister(Register("i2c_data_tx", RegisterDataType.UNSIGNED, ReadWritePermissions.WRITE, "I2c data tx", width=32))

        # self.desc.addRegister(Register("i2c_status", RegisterDataType.PACKED, ReadWritePermissions.READ, "I2c status"))
        # self.desc.packInto("i2c_status", Register("busy", RegisterDataType.BOOL, ReadWritePermissions.READ, "Busy"))
        # self.desc.packInto("i2c_status", Register("error", RegisterDataType.BOOL, ReadWritePermissions.READ, "Error"))

        # self.desc.generateAddressMap()

        driver_settings = {
        }

        self.rm = RegisterMapGenerator("serial_interface_card", ["serial_interface_card"], driver_settings, "Differential serial interface card")
        self.rm.add(Register("port_mode_enable", rw="w", desc="Select which port modes to enable", sub_registers=[
            Register("rs485_mode_enable", width=10),
            Register("rs422_mode_enable", width=10),
            Register("quadrature_mode_enable", width=7)
        ]))
        self.rm.add(Register("i2c_config", rw="w", desc="I2C configuration", sub_registers=[
            Register("read", type="bool", desc="Read mode"),
            Register("device_address", width=7, desc="Device address"),
            Register("register_address", width=8, desc="Device register"),
            Register("byte_count", width=3, desc="Byte count"),
            Register("start", type="bool", desc="Start")
        ]))
        self.rm.add(Register("i2c_data_rx", rw="r", desc="I2C data RX"))
        self.rm.add(Register("i2c_data_tx", rw="w", desc="I2C data TX"))
        self.rm.add(Register("i2c_status", rw="r", desc="I2C status", sub_registers=[
            Register("busy", type="bool", desc="Busy"),
            Register("error", type="bool", desc="Error")
        ]))
        
        self.rm.generate()

        self.rs485_mode_enable = Signal(10)
        self.rs422_mode_enable = Signal(10)
        self.quadrature_mode_enable = Signal(7)

        # # rs485/422 mode
        # self.rs485rxtx = Array(Signal(name=f"rs485_RXTX_{_}") for _ in range(10))
        # self.rs485txen = Array(Signal(name=f"rs485_TX_enable_{_}") for _ in range(10))

        # # rs422 mode
        # self.rs422_rx = Array(Signal(name=f"rs422_RX_{_}") for _ in range(10))
        # self.rs422_tx = Array(Signal(name=f"rs422_TX_{_}") for _ in range(10))

        # # quadrature encoder mode
        # self.quadratureA = Array(Signal(name=f"quadrature_A_{_}") for _ in range(7))
        # self.quadratureB = Array(Signal(name=f"quadrature_B_{_}") for _ in range(7))
        # self.quadratureZ = Array(Signal(name=f"quadrature_Z_{_}") for _ in range(6))   # the final quadrature input does not have a Z input, hence there are only 6 Z signals but 7 A/B signals


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

        #m.submodules.shift_dma = self.shift_dma = shift_dma_node(self.address)
        m.submodules.i2c = self.i2c = i2c()

        pins = self.pins

        # I2C
        m.d.comb += self.i2c.sda_in.eq(self.slotIn[pins.PIN_11N])
        m.d.comb += self.slotOut[pins.PIN_11P].eq(self.i2c.scl)
        m.d.comb += self.slotOutEnable[pins.PIN_11P].eq(self.i2c.scl_enable)
        m.d.comb += self.slotOut[pins.PIN_11N].eq(self.i2c.sda_out)
        m.d.comb += self.slotOutEnable[pins.PIN_11N].eq(self.i2c.sda_out_enable)


        # memory interface
        with m.Switch(self.bram_address):   # TODO: make this switch able to be generated from the address map automatically
            with m.Case(self.rm.port_mode_enable.address_offset):
                m.d.sync_100 += self.bram_read_data.eq(0)
                with m.If(self.bram_write_enable):
                    m.d.sync_100 += self.rs485_mode_enable.eq(self.bram_write_data[:10])
                    m.d.sync_100 += self.rs422_mode_enable.eq(self.bram_write_data[10:20])
                    m.d.sync_100 += self.quadrature_mode_enable.eq(self.bram_write_data[20:27])
            with m.Case(self.rm.i2c_config.address_offset):
                m.d.sync_100 += self.bram_read_data.eq(0)
                with m.If(self.bram_write_enable):
                    m.d.sync_100 += self.i2c.read.eq(self.bram_write_data[0])
                    m.d.sync_100 += self.i2c.device_address.eq(self.bram_write_data[1:8])
                    m.d.sync_100 += self.i2c.register_address.eq(self.bram_write_data[8:16])
                    m.d.sync_100 += self.i2c.byte_count.eq(self.bram_write_data[16:19])
                    m.d.sync_100 += self.i2c.start.eq(self.bram_write_data[19])

            with m.Case(self.rm.i2c_data_tx.address_offset):
                m.d.sync_100 += self.bram_read_data.eq(0)
                with m.If(self.bram_write_enable):
                    m.d.sync_100 += self.i2c.data_in.eq(self.bram_write_data)

            with m.Case(self.rm.i2c_data_rx.address_offset):
                m.d.sync_100 += self.bram_read_data.eq(self.i2c.data_out)

            with m.Case(self.rm.i2c_status.address_offset):
                m.d.sync_100 += self.bram_read_data.eq(Cat(self.i2c.busy, self.i2c.error))

        with m.If(self.i2c.busy):   # reset start once a transaction is started
            m.d.sync_100 += self.i2c.start.eq(0)



        # with m.If(self.rs485_mode_enable[0]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_10P].eq(self.rs485txen[0])
        #     with m.If(self.rs485txen[0]):
        #         m.d.comb += self.slotOut[pins.PIN_10N].eq(self.rs485rxtx[0])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[0].eq(self.slotIn[pins.PIN_10N])

        # with m.If(self.rs485_mode_enable[1]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_5P].eq(self.rs485txen[1])
        #     with m.If(self.rs485txen[1]):
        #         m.d.comb += self.slotOut[pins.PIN_5N].eq(self.rs485rxtx[1])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[1].eq(self.slotIn[pins.PIN_5N])
        
        # with m.If(self.rs485_mode_enable[2]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_9P].eq(self.rs485txen[2])
        #     with m.If(self.rs485txen[2]):
        #         m.d.comb += self.slotOut[pins.PIN_9N].eq(self.rs485rxtx[2])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[2].eq(self.slotIn[pins.PIN_9N])
        
        # with m.If(self.rs485_mode_enable[3]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_4P].eq(self.rs485txen[3])
        #     with m.If(self.rs485txen[3]):
        #         m.d.comb += self.slotOut[pins.PIN_4N].eq(self.rs485rxtx[3])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[3].eq(self.slotIn[pins.PIN_4N])
        
        # with m.If(self.rs485_mode_enable[4]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_8P].eq(self.rs485txen[4])
        #     with m.If(self.rs485txen[4]):
        #         m.d.comb += self.slotOut[pins.PIN_8N].eq(self.rs485rxtx[4])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[4].eq(self.slotIn[pins.PIN_8N])
        
        # with m.If(self.rs485_mode_enable[5]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_3P].eq(self.rs485txen[5])
        #     with m.If(self.rs485txen[5]):
        #         m.d.comb += self.slotOut[pins.PIN_3N].eq(self.rs485rxtx[5])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[5].eq(self.slotIn[pins.PIN_3N])
        
        # with m.If(self.rs485_mode_enable[6]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_7P].eq(self.rs485txen[6])
        #     with m.If(self.rs485txen[6]):
        #         m.d.comb += self.slotOut[pins.PIN_7N].eq(self.rs485rxtx[6])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[6].eq(self.slotIn[pins.PIN_7N])
        
        # with m.If(self.rs485_mode_enable[7]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_2P].eq(self.rs485txen[7])
        #     with m.If(self.rs485txen[7]):
        #         m.d.comb += self.slotOut[pins.PIN_2N].eq(self.rs485rxtx[7])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[7].eq(self.slotIn[pins.PIN_2N])
        
        # with m.If(self.rs485_mode_enable[8]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_6P].eq(self.rs485txen[8])
        #     with m.If(self.rs485txen[8]):
        #         m.d.comb += self.slotOut[pins.PIN_6N].eq(self.rs485rxtx[8])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[8].eq(self.slotIn[pins.PIN_6N])
        
        # with m.If(self.rs485_mode_enable[9]):
        #     m.d.comb += self.slotOutEnable[pins.PIN_1P].eq(self.rs485txen[9])
        #     with m.If(self.rs485txen[9]):
        #         m.d.comb += self.slotOut[pins.PIN_1N].eq(self.rs485rxtx[9])
        #     with m.Else():
        #         m.d.comb += self.rs485rxtx[9].eq(self.slotIn[pins.PIN_1N])



        with m.If(self.rs422_mode_enable[0]):
            m.d.comb += self.slotOut[pins.PIN_10N].eq(self.rs422_tx[0])
            m.d.comb += self.rs422_rx[0].eq(self.slotIn[pins.PIN_10P])
            m.d.comb += self.slotOutEnable[pins.PIN_10N].eq(1)

        with m.If(self.rs422_mode_enable[1]):
            m.d.comb += self.slotOut[pins.PIN_5N].eq(self.rs422_tx[1])
            m.d.comb += self.rs422_rx[1].eq(self.slotIn[pins.PIN_5P])
            m.d.comb += self.slotOutEnable[pins.PIN_5N].eq(1)

        with m.If(self.rs422_mode_enable[2]):
            m.d.comb += self.slotOut[pins.PIN_9N].eq(self.rs422_tx[2])
            m.d.comb += self.rs422_rx[2].eq(self.slotIn[pins.PIN_9P])
            m.d.comb += self.slotOutEnable[pins.PIN_9N].eq(1)

        with m.If(self.rs422_mode_enable[3]):
            m.d.comb += self.slotOut[pins.PIN_4N].eq(self.rs422_tx[3])
            m.d.comb += self.rs422_rx[3].eq(self.slotIn[pins.PIN_4P])
            m.d.comb += self.slotOutEnable[pins.PIN_4N].eq(1)

        with m.If(self.rs422_mode_enable[4]):
            m.d.comb += self.slotOut[pins.PIN_8N].eq(self.rs422_tx[4])
            m.d.comb += self.rs422_rx[4].eq(self.slotIn[pins.PIN_8P])
            m.d.comb += self.slotOutEnable[pins.PIN_8N].eq(1)

        with m.If(self.rs422_mode_enable[5]):
            m.d.comb += self.slotOut[pins.PIN_3N].eq(self.rs422_tx[5])
            m.d.comb += self.rs422_rx[5].eq(self.slotIn[pins.PIN_3P])
            m.d.comb += self.slotOutEnable[pins.PIN_3N].eq(1)

        with m.If(self.rs422_mode_enable[6]):
            m.d.comb += self.slotOut[pins.PIN_7N].eq(self.rs422_tx[6])
            m.d.comb += self.rs422_rx[6].eq(self.slotIn[pins.PIN_7P])
            m.d.comb += self.slotOutEnable[pins.PIN_7N].eq(1)

        with m.If(self.rs422_mode_enable[7]):
            m.d.comb += self.slotOut[pins.PIN_2N].eq(self.rs422_tx[7])
            m.d.comb += self.rs422_rx[7].eq(self.slotIn[pins.PIN_2P])
            m.d.comb += self.slotOutEnable[pins.PIN_2N].eq(1)

        with m.If(self.rs422_mode_enable[8]):
            m.d.comb += self.slotOut[pins.PIN_6N].eq(self.rs422_tx[8])
            m.d.comb += self.rs422_rx[8].eq(self.slotIn[pins.PIN_6P])
            m.d.comb += self.slotOutEnable[pins.PIN_6N].eq(1)

        with m.If(self.rs422_mode_enable[9]):
            m.d.comb += self.slotOut[pins.PIN_1N].eq(self.rs422_tx[9])
            m.d.comb += self.rs422_rx[9].eq(self.slotIn[pins.PIN_1P])
            m.d.comb += self.slotOutEnable[pins.PIN_1N].eq(1)


        
        # with m.If(self.quadrature_mode_enable[0]):
        #     m.d.comb += self.quadratureA[0].eq(self.slotIn[pins.PIN_10N])
        #     m.d.comb += self.quadratureB[0].eq(self.slotIn[pins.PIN_10P])
        #     m.d.comb += self.quadratureZ[0].eq(self.slotIn[pins.PIN_5N])

        # with m.If(self.quadrature_mode_enable[1]):        
        #     m.d.comb += self.quadratureA[1].eq(self.slotIn[pins.PIN_5P])
        #     m.d.comb += self.quadratureB[1].eq(self.slotIn[pins.PIN_9N])
        #     m.d.comb += self.quadratureZ[1].eq(self.slotIn[pins.PIN_9P])

        # with m.If(self.quadrature_mode_enable[2]):
        #     m.d.comb += self.quadratureA[2].eq(self.slotIn[pins.PIN_4N])
        #     m.d.comb += self.quadratureB[2].eq(self.slotIn[pins.PIN_4P])
        #     m.d.comb += self.quadratureZ[2].eq(self.slotIn[pins.PIN_8N])

        # with m.If(self.quadrature_mode_enable[3]):
        #     m.d.comb += self.quadratureA[3].eq(self.slotIn[pins.PIN_8P])
        #     m.d.comb += self.quadratureB[3].eq(self.slotIn[pins.PIN_3N])
        #     m.d.comb += self.quadratureZ[3].eq(self.slotIn[pins.PIN_3P])

        # with m.If(self.quadrature_mode_enable[4]):
        #     m.d.comb += self.quadratureA[4].eq(self.slotIn[pins.PIN_7N])
        #     m.d.comb += self.quadratureB[4].eq(self.slotIn[pins.PIN_7P])
        #     m.d.comb += self.quadratureZ[4].eq(self.slotIn[pins.PIN_2N])

        # with m.If(self.quadrature_mode_enable[5]):
        #     m.d.comb += self.quadratureA[5].eq(self.slotIn[pins.PIN_2P])
        #     m.d.comb += self.quadratureB[5].eq(self.slotIn[pins.PIN_6N])
        #     m.d.comb += self.quadratureZ[5].eq(self.slotIn[pins.PIN_6P])

        # with m.If(self.quadrature_mode_enable[6]):
        #     m.d.comb += self.quadratureA[6].eq(self.slotIn[pins.PIN_1N])
        #     m.d.comb += self.quadratureB[6].eq(self.slotIn[pins.PIN_1P])

        return m
    
            
if __name__ == "__main__":
    #TODO: make test bench
    pass