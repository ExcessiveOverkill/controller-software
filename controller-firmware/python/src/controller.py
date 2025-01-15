from amaranth import *
from amaranth.lib import wiring
from amaranth.lib.wiring import In, Out
from shift_dma import shift_dma_controller, shift_dma_node
from amaranth.lib.memory import Memory
from registers2 import *
from interface_cards.serial_interface import serial_interface_card
from fanuc_encoder import Fanuc_Encoders
from global_timer import Global_Timers
from em_serial_controller import EM_Serial_Controller



class Controller(wiring.Component):
    def __init__(self, nodes:dict, sim=False):
        self.sim = sim
        self.nodes = nodes

        interface_name = "AXI_controller_master"

        # Define the interface parameters
        interface_params = (
            f"XIL_INTERFACENAME {interface_name}, "
            "CLK_DOMAIN controller_firmware_processing_system7_0_0_FCLK_CLK0, "
            "FREQ_HZ 100000000, "
            "PHASE 0.0, "
            "PROTOCOL AXI3, "
            f"DATA_WIDTH {64}, "
            f"ID_WIDTH {0}, "
            f"ADDR_WIDTH {32}, "
            "HAS_BURST 1, "
            "HAS_CACHE 0, "
            "HAS_LOCK 0, "
            "HAS_PROT 0, "
            "HAS_QOS 0, "
            "HAS_REGION 0, "
            "HAS_WSTRB 1, "
            "HAS_BRESP 0, "
            "HAS_RRESP 0, "
            "SUPPORTS_NARROW_BURST 0, "
            "MAX_BURST_LENGTH 16, "
            "NUM_READ_OUTSTANDING 1, "
            "NUM_WRITE_OUTSTANDING 1, "
            "READ_WRITE_MODE READ_WRITE"
        )

        super().__init__({
            # Clock and Reset
            "clk_200M": In(1),
            "clk_100M": In(1),
            "clk_50M": In(1),
            "clk_25M": In(1),
            "reset": In(1),


            # Clock and Reset
            "ACLK": In(1),
            "ARESETN": In(1),

            # Write address channel
            #"AWID": Out(6),
            "AWADDR": Out(32),
            "AWLEN": Out(4),
            "AWSIZE": Out(3, init=0b011),   # 64 bit
            "AWBURST": Out(2, init=0b01),   # incrementing burst
            #"AWLOCK": Out(1),    # unused
            #"AWCACHE": Out(4),   # unused
            "AWPROT": Out(3),    # unused
            #"AWREGION": Out(4),  # unused
            #"AWQOS": Out(4),     # unused
            "AWUSER": Out(0),
            "AWVALID": Out(1),
            "AWREADY": In(1),

            # Write data channel
            #"WID": Out(6),
            "WDATA": Out(64),
            "WSTRB": Out(64 // 8, init=0b11111111),   
            "WLAST": Out(1),
            #"WUSER": Out(0),    # unused
            "WVALID": Out(1),
            "WREADY": In(1),

            # Write response channel
            #"BID": In(6),
            #"BRESP": In(2),    # unused
            #"BUSER": In(0),   # unused
            "BVALID": In(1),
            "BREADY": Out(1),

            # Read address channel
            #"ARID": Out(6),
            "ARADDR": Out(32),
            "ARLEN": Out(4),
            "ARSIZE": Out(3, init=0b011),   # 64 bit
            "ARBURST": Out(2, init=0b01),   # incrementing burst
            #"ARLOCK": Out(1),    # unused
            #"ARCACHE": Out(4),   # unused
            "ARPROT": Out(3),    # unused
            #"ARREGION": Out(4),  # unused
            #"ARQOS": Out(4),     # unused
            #"ARUSER": Out(0),   # unused
            "ARVALID": Out(1),
            "ARREADY": In(1),

            # Read data channel
            #"RID": In(6),
            "RDATA": In(64),
            #"RRESP": In(2),    # unused
            "RLAST": In(1),
            #"RUSER": In(0),   # unused
            "RVALID": In(1),
            "RREADY": Out(1),

            # onboard peripherals
            "buzzer": Out(1),

            # slot IO
            "slot_A_in": In(22),
            "slot_A_out": Out(22),
            "slot_A_out_enable": Out(22),
            "slot_B_in": In(22),
            "slot_B_out": Out(22),
            "slot_B_out_enable": Out(22),
            "slot_C_in": In(22),
            "slot_C_out": Out(22),
            "slot_C_out_enable": Out(22),
            "slot_D_in": In(22),
            "slot_D_out": Out(22),
            "slot_D_out_enable": Out(22),

            # interrupts
            "pl_ps_interrupts": Out(16),

        })

        # Assign attributes to the signals

        # self.ACLK = Signal()
        # self.ARESETN = Signal()

        self.ACLK.attrs["X_INTERFACE_INFO"] = f"xilinx.com:signal:clock:1.0 {interface_name} CLK"
        self.ARESETN.attrs["X_INTERFACE_INFO"] = f"xilinx.com:signal:reset:1.0 {interface_name} RST"
        #self.AWID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWID"
        self.AWADDR.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWADDR"
        self.AWLEN.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWLEN"
        self.AWSIZE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWSIZE"
        self.AWBURST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWBURST"
        #self.AWLOCK.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWLOCK"
        #self.AWCACHE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWCACHE"
        self.AWPROT.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWPROT"
        #self.AWREGION.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWREGION"
        #self.AWQOS.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWQOS"
        #self.AWUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWUSER"
        self.AWVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWVALID"
        self.AWREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWREADY"
        #self.WID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WID"
        self.WDATA.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WDATA"
        self.WSTRB.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WSTRB"
        self.WLAST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WLAST"
        #self.WUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WUSER"
        self.WVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WVALID"
        self.WREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WREADY"
        #self.BID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BID"
        #self.BRESP.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BRESP"
        #self.BUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BUSER"
        self.BVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BVALID"
        self.BREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BREADY"
        #self.ARID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARID"
        self.ARADDR.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARADDR"
        self.ARLEN.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARLEN"
        self.ARSIZE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARSIZE"
        self.ARBURST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARBURST"
        #self.ARLOCK.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARLOCK"
        #self.ARCACHE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARCACHE"
        self.ARPROT.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARPROT"
        #self.ARREGION.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARREGION"
        #self.ARQOS.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARQOS"
        #self.ARUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARUSER"
        self.ARVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARVALID"
        self.ARREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARREADY"
        #self.RID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RID"
        self.RDATA.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RDATA"
        #self.RRESP.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RRESP"
        self.RLAST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RLAST"
        #self.RUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RUSER"
        self.RVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RVALID"
        self.RREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RREADY"
        
        # Assign interface-level attributes to one of the signals
        self.RREADY.attrs["X_INTERFACE_PARAMETER"] = interface_params

        # OCM memory that the PS can access, we are only allowed to write to this address range
        self.OCM_BASE_ADDR = 0x000F0000
        self.OCM_SIZE = 0x8000  # 32KB

        # OCM layout, this must match the layout in fpga_interface.h

        self.PS_TO_PL_CONTROL_OFFSET = 0x0000
        self.PS_TO_PL_CONTROL_SIZE = 0x40   # 64 bytes
        self.PL_TO_PS_CONTROL_OFFSET = 0x800
        self.PL_TO_PS_CONTROL_SIZE = 0x40   # 64 bytes

        # these sizes must be equal
        self.PS_TO_PL_DATA_OFFSET = 0x1000
        self.PS_TO_PL_DATA_SIZE = 0x1000   # 4KB
        self.PL_TO_PS_DATA_OFFSET = 0x2000
        self.PL_TO_PS_DATA_SIZE = 0x1000   # 4KB

        self.PS_TO_PL_DMA_INSTRUCTION_OFFSET = 0x3000
        self.PS_TO_PL_DMA_INSTRUCTION_SIZE = 0x800  # 2KB

        self.LARGEST_MEMORY_REGION = 0x1000 # 4KB

        if(self.sim):   # use smaller memory regions for simulation
            self.PS_TO_PL_DATA_SIZE = 0x100   # 256 bytes
            self.PL_TO_PS_DATA_SIZE = 0x100   # 256 bytes
            self.PS_TO_PL_DMA_INSTRUCTION_SIZE = 0x100  # 256 bytes
            self.LARGEST_MEMORY_REGION = 0x100   # 256 bytes

        # actual memory sizes may be larger than the above access sizes
        self.INSTRUCTION_MEMORY_SIZE = self.PS_TO_PL_DMA_INSTRUCTION_SIZE // 8  # 64 bit instructions
        self.DATA_MEMORY_SIZE = self.PS_TO_PL_DATA_SIZE // 4 # 32 bit data, size of read and write blocks (each)

        if(self.INSTRUCTION_MEMORY_SIZE < self.PS_TO_PL_DMA_INSTRUCTION_SIZE // 8):
            raise Exception("Instruction memory size is smaller than the instruction memory access size")
        if(self.DATA_MEMORY_SIZE < self.PS_TO_PL_DATA_SIZE // 4):
            raise Exception("Data memory size is smaller than the data memory access size")

        # self.desc.addDriverData("OCM_BASE_ADDR", self.OCM_BASE_ADDR)
        # self.desc.addDriverData("OCM_SIZE", self.OCM_SIZE)
        # self.desc.addDriverData("PS_TO_PL_CONTROL_OFFSET", self.PS_TO_PL_CONTROL_OFFSET)
        # self.desc.addDriverData("PS_TO_PL_CONTROL_SIZE", self.PS_TO_PL_CONTROL_SIZE)
        # self.desc.addDriverData("PL_TO_PS_CONTROL_OFFSET", self.PL_TO_PS_CONTROL_OFFSET)
        # self.desc.addDriverData("PL_TO_PS_CONTROL_SIZE", self.PL_TO_PS_CONTROL_SIZE)
        # self.desc.addDriverData("PS_TO_PL_DATA_OFFSET", self.PS_TO_PL_DATA_OFFSET)
        # self.desc.addDriverData("PS_TO_PL_DATA_SIZE", self.PS_TO_PL_DATA_SIZE)
        # self.desc.addDriverData("PL_TO_PS_DATA_OFFSET", self.PL_TO_PS_DATA_OFFSET)
        # self.desc.addDriverData("PL_TO_PS_DATA_SIZE", self.PL_TO_PS_DATA_SIZE)
        # self.desc.addDriverData("PS_TO_PL_DMA_INSTRUCTION_OFFSET", self.PS_TO_PL_DMA_INSTRUCTION_OFFSET)
        # self.desc.addDriverData("PS_TO_PL_DMA_INSTRUCTION_SIZE", self.PS_TO_PL_DMA_INSTRUCTION_SIZE)
        # self.desc.addDriverData("INSTRUCTION_MEMORY_SIZE", self.INSTRUCTION_MEMORY_SIZE)
        # self.desc.addDriverData("DATA_MEMORY_SIZE", self.DATA_MEMORY_SIZE)

        driver_settings = {
            "OCM_BASE_ADDR": self.OCM_BASE_ADDR,
            "OCM_SIZE": self.OCM_SIZE,
            "PS_TO_PL_CONTROL_OFFSET": self.PS_TO_PL_CONTROL_OFFSET,
            "PS_TO_PL_CONTROL_SIZE": self.PS_TO_PL_CONTROL_SIZE,
            "PL_TO_PS_CONTROL_OFFSET": self.PL_TO_PS_CONTROL_OFFSET,
            "PL_TO_PS_CONTROL_SIZE": self.PL_TO_PS_CONTROL_SIZE,
            "PS_TO_PL_DATA_OFFSET": self.PS_TO_PL_DATA_OFFSET,
            "PS_TO_PL_DATA_SIZE": self.PS_TO_PL_DATA_SIZE,
            "PL_TO_PS_DATA_OFFSET": self.PL_TO_PS_DATA_OFFSET,
            "PL_TO_PS_DATA_SIZE": self.PL_TO_PS_DATA_SIZE,
            "PS_TO_PL_DMA_INSTRUCTION_OFFSET": self.PS_TO_PL_DMA_INSTRUCTION_OFFSET,
            "PS_TO_PL_DMA_INSTRUCTION_SIZE": self.PS_TO_PL_DMA_INSTRUCTION_SIZE,
            "INSTRUCTION_MEMORY_SIZE": self.INSTRUCTION_MEMORY_SIZE,
            "DATA_MEMORY_SIZE": self.DATA_MEMORY_SIZE,
        }
        self.rm = RegisterMapGenerator("controller", ["controller"], driver_settings)
        self.rm.generate()
        
        self.address = 0


    def elaborate(self, platform):
        m = Module()

        # Create a clock domains
        #m.domains.sync = ClockDomain("sync", async_reset=True)
        m.domains.sync_200 = ClockDomain("sync_200", async_reset=True)
        m.domains.sync_100 = ClockDomain("sync_100", async_reset=True)
        m.domains.sync_50 = ClockDomain("sync_50", async_reset=True)
        m.domains.sync_25 = ClockDomain("sync_25", async_reset=True)
        
        # about enough memory to use up an entire update period at 50% utilization (hopefully more than we'll ever need)
        m.submodules.instruction_memory = self.instruction_memory = Memory(shape=unsigned(64), depth=(self.INSTRUCTION_MEMORY_SIZE), init=[])
        m.submodules.data_memory_read = self.data_memory_read = Memory(shape=unsigned(32), depth=(self.DATA_MEMORY_SIZE), init=[])
        m.submodules.data_memory_write = self.data_memory_write = Memory(shape=unsigned(32), depth=(self.DATA_MEMORY_SIZE), init=[])

        m.submodules.shift_dma = self.shift_dma = shift_dma_controller(instruction_memory_depth=self.INSTRUCTION_MEMORY_SIZE)

        self.instruction_read_port = self.instruction_memory.read_port(domain="sync_100")
        self.instruction_write_port = self.instruction_memory.write_port(domain="sync_100")

        # data ports for dma use, these also get used for axi transfers when the dma is not active to get 64 bit data
        self.data_read_read_port_dma = self.data_memory_read.read_port(domain="sync_100")
        self.data_read_write_port_dma = self.data_memory_read.write_port(domain="sync_100")

        self.data_write_read_port_dma = self.data_memory_write.read_port(domain="sync_100")
        self.data_write_write_port_dma = self.data_memory_write.write_port(domain="sync_100")

        # axi only data ports
        self.data_read_port_axi = self.data_memory_read.read_port(domain="sync_100")
        self.data_write_port_axi = self.data_memory_write.write_port(domain="sync_100")


        self.instruction_read_address = Signal(range(self.INSTRUCTION_MEMORY_SIZE))
        self.instruction_read_data = Signal(64)
        self.instruction_write_address = Signal(range(self.INSTRUCTION_MEMORY_SIZE))
        self.instruction_write_data = Signal(64)
        self.instruction_write_en = Signal()


        self.data_read_dma_address = Signal(range(self.DATA_MEMORY_SIZE))
        self.data_read_dma_read_data = Signal(32)
        self.data_read_dma_write_data = Signal(32)
        self.data_read_dma_write_en = Signal()

        self.data_write_dma_address = Signal(range(self.DATA_MEMORY_SIZE))
        self.data_write_dma_read_data = Signal(32)
        self.data_write_dma_write_data = Signal(32)
        self.data_write_dma_write_en = Signal()


        self.data_read_axi_address = Signal(range(self.DATA_MEMORY_SIZE))
        self.data_read_axi_data = Signal(64)
        self.data_write_axi_address = Signal(range(self.DATA_MEMORY_SIZE))
        self.data_write_axi_data = Signal(64)
        self.data_write_axi_enable = Signal()

        self.memory_update_running = self.pl_ps_interrupts[0]
        self.memory_update_done = self.pl_ps_interrupts[1]
        self.dma_cycle_running = self.pl_ps_interrupts[2]
        self.dma_cycle_done = self.pl_ps_interrupts[3]

        self.axi_transfer_start = Signal()
        self.axi_transfer_busy = Signal()

        self.debug_pins = Signal(8)
        m.d.comb += [
            self.slot_A_out[0:8].eq(self.debug_pins),
            self.slot_A_out_enable[0:8].eq(0xFF),
        #     # self.debug_pins[0].eq(self.AWVALID),
        #     # self.debug_pins[1].eq(self.AWREADY),
        #     # self.debug_pins[2].eq(self.WVALID),
        #     # self.debug_pins[3].eq(self.WREADY),
        #     # self.debug_pins[4].eq(self.WLAST),
        #     # self.debug_pins[5].eq(self.BVALID),
        #     # self.debug_pins[6].eq(self.BREADY),
        #     # self.debug_pins[0].eq(self.pl_ps_interrupts[0]),
        #     # self.debug_pins[1].eq(self.pl_ps_interrupts[1]),
        #     # self.debug_pins[2].eq(self.pl_ps_interrupts[2]),
        #     # self.debug_pins[3].eq(self.pl_ps_interrupts[3]),
        #     # self.debug_pins[0].eq(self.slot_B_out[20]),
        #     # self.debug_pins[1].eq(self.slot_B_out[21]),
        #     # self.debug_pins[2].eq(self.slot_B_out_enable[20]),
        #     # self.debug_pins[3].eq(self.slot_B_out_enable[21]),
        ]



        m.d.comb += [
            self.data_read_read_port_dma.addr.eq(self.data_read_dma_address),
            self.data_read_write_port_dma.addr.eq(self.data_read_dma_address),
            self.data_read_dma_read_data.eq(self.data_read_read_port_dma.data),
            self.data_read_write_port_dma.data.eq(self.data_read_dma_write_data),
            self.data_read_write_port_dma.en.eq(self.data_read_dma_write_en),

            self.data_write_read_port_dma.addr.eq(self.data_write_dma_address),
            self.data_write_write_port_dma.addr.eq(self.data_write_dma_address),
            self.data_write_dma_read_data.eq(self.data_write_read_port_dma.data),
            self.data_write_write_port_dma.data.eq(self.data_write_dma_write_data),
            self.data_write_write_port_dma.en.eq(self.data_write_dma_write_en),

            self.instruction_read_port.addr.eq(self.instruction_read_address),
            self.instruction_read_data.eq(self.instruction_read_port.data),
            self.instruction_write_port.addr.eq(self.instruction_write_address),
            self.instruction_write_port.data.eq(self.instruction_write_data),
            self.instruction_write_port.en.eq(self.instruction_write_en),


            self.shift_dma.instruction_memory_read_data.eq(self.instruction_read_data),
            self.instruction_read_address.eq(self.shift_dma.instruction_memory_address),
        ]

        self.dma_memory_half = Signal()
        self.dma_memory_half_comb = Signal()

        with m.If(self.shift_dma.data_memory_address[len(self.data_read_dma_address)]):
            m.d.sync_100 += self.dma_memory_half.eq(1)
            m.d.comb += self.dma_memory_half_comb.eq(1)
        with m.Else():
            m.d.sync_100 += self.dma_memory_half.eq(0)

        with m.If(~self.axi_transfer_busy):   # if axi is not transfering data, link the memmory ports to the dma
            # check if the address is in the read or write memory
            m.d.comb += [
                self.data_read_dma_address.eq(self.shift_dma.data_memory_address[0:len(self.data_read_dma_address)]),
                self.data_write_dma_address.eq(self.shift_dma.data_memory_address[0:len(self.data_read_dma_address)]),
                self.data_read_dma_write_data.eq(self.shift_dma.data_memory_write_data),
                self.data_write_dma_write_data.eq(self.shift_dma.data_memory_write_data),
            ]

            with m.If(self.dma_memory_half == 0):  # read memory signals must be delayed by one cycle
                m.d.comb += [
                    #self.data_read_dma_address.eq(self.shift_dma.data_memory_address[0:len(self.data_read_dma_address)]),
                    self.shift_dma.data_memory_read_data.eq(self.data_read_dma_read_data),
                    #self.data_read_dma_write_data.eq(self.shift_dma.data_memory_write_data),
                ]

            with m.Else():  # write memory
                m.d.comb += [
                    #self.data_write_dma_address.eq(self.shift_dma.data_memory_address[0:len(self.data_read_dma_address)]),
                    self.shift_dma.data_memory_read_data.eq(self.data_write_dma_read_data),
                    #self.data_write_dma_write_data.eq(self.shift_dma.data_memory_write_data),
                ]

            with m.If(self.dma_memory_half_comb == 0):  # write memory signals must be switched immediately
                m.d.comb += [
                    self.data_read_dma_write_en.eq(self.shift_dma.data_memory_write_enable),
                    self.data_write_dma_write_en.eq(0),
                ]
            with m.Else():
                m.d.comb += [
                    self.data_write_dma_write_en.eq(self.shift_dma.data_memory_write_enable),
                    self.data_read_dma_write_en.eq(0),
                ]

        with m.Else():   # if axi is transfering data, link the memory ports to the axi interface
            m.d.comb += [
                # pack read data into 64 bit data
                self.data_read_port_axi.addr.eq(self.data_read_axi_address << 1),
                self.data_read_dma_address.eq(self.data_read_axi_address << 1 | 0b1),
                self.data_read_axi_data.eq(self.data_read_port_axi.data | (self.data_read_dma_read_data << 32)),

                # unpack 64 bit data into 32 bit data
                self.data_write_port_axi.addr.eq(self.data_write_axi_address << 1),
                self.data_write_dma_address.eq(self.data_write_axi_address << 1 | 0b1),
                self.data_write_port_axi.data.eq(self.data_write_axi_data[0:32]),
                self.data_write_dma_write_data.eq(self.data_write_axi_data[32:64]),
                self.data_write_port_axi.en.eq(self.data_write_axi_enable),
                self.data_write_dma_write_en.eq(self.data_write_axi_enable),
            ]


        if(not self.sim):
            m.d.comb += [
                ClockSignal("sync_200").eq(self.clk_200M),
                ClockSignal("sync_100").eq(self.clk_100M),
                ClockSignal("sync_50").eq(self.clk_50M),
                ClockSignal("sync_25").eq(self.clk_25M),
                #ClockSignal("sync").eq(self.clk_200M),
                ResetSignal("sync_200").eq(~self.reset),
                ResetSignal("sync_100").eq(~self.reset),
                ResetSignal("sync_50").eq(~self.reset),
                ResetSignal("sync_25").eq(~self.reset),
                #ResetSignal("sync").eq(~self.reset),
                #self.ACLK.eq(self.clk_200M),
                #self.ARESETN.eq(~self.reset)
            ]

        

        # TODO: make these control registers be defined by the register address map
        # internal control signals
        # PL to PS
        self.status = Signal(32)  # status register

        # PS to PL
        self.cycle_timer_config = Signal(16, reset=0xFFFF)   # main timer for triggering FPGA updates, 25Mhz clock, lowest possible update frequency is ~380hz

        if(self.sim):
            self.cycle_timer_config = Signal(16, reset=0x0010)
        self.cycle_timer = Signal(16)  # current timer value
        self.dma_instruction_block_select = Signal(4)  # select which block of instructions to use


        # AXI transfer section, this reads and writes to the OCM

        # NOTE: single transfers may not cross a 4KB boundary (usually not an issue as the bursts should always be boundary aligned)

        self.axi_write_busy = Signal()
        self.axi_read_busy = Signal()
        m.d.sync_100 += self.axi_transfer_busy.eq(self.axi_write_busy | self.axi_read_busy)

        self.write_stages = {
            0: {"offset": self.PL_TO_PS_CONTROL_OFFSET, "burst_size": self.PL_TO_PS_CONTROL_SIZE // 8},
            1: {"offset": self.PL_TO_PS_DATA_OFFSET, "burst_size": self.PL_TO_PS_DATA_SIZE // 8},
        }
        self.write_stage = Signal(range(len(self.write_stages)+2))

        self.read_stages = {
            0: {"offset": self.PS_TO_PL_CONTROL_OFFSET, "burst_size": self.PS_TO_PL_CONTROL_SIZE // 8},
            1: {"offset": self.PS_TO_PL_DATA_OFFSET, "burst_size": self.PS_TO_PL_DATA_SIZE // 8},
            2: {"offset": self.PS_TO_PL_DMA_INSTRUCTION_OFFSET, "burst_size": self.PS_TO_PL_DMA_INSTRUCTION_SIZE // 8},
        }
        self.read_stage = Signal(range(len(self.read_stages)+2))

        self.write_bursts_remaining = Signal(range(self.LARGEST_MEMORY_REGION // 8 + 1))
        self.read_bursts_remaining = Signal(range(self.LARGEST_MEMORY_REGION // 8 + 1))
        self.write_current_burst = Signal(4)
        self.read_current_burst = Signal(4)

        self.write_addr_complete = Signal()
        self.write_data_complete = Signal()
        m.d.comb += self.write_addr_complete.eq(self.AWVALID & self.AWREADY)
        m.d.comb += self.write_data_complete.eq(self.WVALID & self.WREADY)

        self.read_addr_complete = Signal()
        m.d.comb += self.read_addr_complete.eq(self.ARVALID & self.ARREADY)


        # write
        self.internal_axi_read_address = Signal(range(self.LARGEST_MEMORY_REGION // 8 + 1))     # 64 bit block address
        self.internal_axi_read_data = Signal(64)
        self.internal_axi_read_valid = Signal()
        self.last_axi_read_data = Signal(64)
        self.axi_read_data_incremented = Signal()
        self.timed_axi_read_data = Signal(64)

        with m.If(self.axi_read_data_incremented):
            m.d.sync_100 += self.axi_read_data_incremented.eq(0)
            m.d.comb += self.timed_axi_read_data.eq(self.internal_axi_read_data)
            m.d.sync_100 += self.last_axi_read_data.eq(self.internal_axi_read_data)
        with m.Else():
            m.d.comb += self.timed_axi_read_data.eq(self.last_axi_read_data)

        with m.Switch(self.write_stage):
            with m.Case(0): # control
                with m.Switch(self.internal_axi_read_address):
                    with m.Case(0):
                        m.d.comb += self.internal_axi_read_data.eq(self.status)
                    with m.Default():
                        m.d.comb += self.internal_axi_read_data.eq(0)
                m.d.sync_100 += self.internal_axi_read_valid.eq(1)

            with m.Case(1): # data
                m.d.comb += self.data_read_axi_address.eq(self.internal_axi_read_address)
                m.d.comb += self.internal_axi_read_data.eq(self.data_read_axi_data)
                m.d.sync_100 += self.internal_axi_read_valid.eq(1)
                pass

        
        with m.FSM(init="idle", domain="sync_100"):
            with m.State("idle"):
                #m.d.sync_100 += self.debug_pins.eq(0b00000000)
                m.d.sync_100 += self.BREADY.eq(0)
                m.d.sync_100 += self.AWVALID.eq(0)
                m.d.sync_100 += self.WVALID.eq(0)
                m.d.sync_100 += self.WLAST.eq(0)
                m.d.sync_100 += self.axi_write_busy.eq(0)
                m.d.sync_100 += self.write_stage.eq(0)
                with m.If(self.axi_transfer_start):
                    m.d.sync_100 += self.axi_write_busy.eq(1)
                    m.next = "get_write_config"
            
            with m.State("get_write_config"):
                #m.d.sync_100 += self.debug_pins.eq(0b00000001)
                with m.Switch(self.write_stage):
                    for i, stage in self.write_stages.items():
                        with m.Case(i):
                            m.d.sync_100 += self.AWADDR.eq(int(stage["offset"] + self.OCM_BASE_ADDR))
                            m.d.sync_100 += self.write_bursts_remaining.eq(stage["burst_size"])

                m.d.sync_100 += self.internal_axi_read_address.eq(0)
                m.next = "set_write_address"

            with m.State("set_write_address"):
                #m.d.sync_100 += self.debug_pins.eq(0b00000010)
                with m.If(self.write_bursts_remaining >= 16):
                    m.d.sync_100 += self.AWLEN.eq(16-1)  # up to 16 burst length
                    m.d.sync_100 += self.write_bursts_remaining.eq(self.write_bursts_remaining - 16)
                with m.Else():
                    m.d.sync_100 += self.AWLEN.eq(self.write_bursts_remaining-1)
                    m.d.sync_100 += self.write_bursts_remaining.eq(0)
                m.d.sync_100 += self.write_current_burst.eq(0)
                m.d.sync_100 += self.AWVALID.eq(1)

                m.next = "set_write_data"

            with m.State("set_write_data"):
                #m.d.sync_100 += self.debug_pins.eq(0b00000100)
                with m.If(self.write_addr_complete):
                    m.d.sync_100 += self.AWVALID.eq(0)

                with m.If(self.write_current_burst == self.AWLEN):
                    m.d.sync_100 += self.WLAST.eq(1)
                with m.Else():
                    m.d.sync_100 += self.WLAST.eq(0)

                with m.If(self.internal_axi_read_valid):
                    #m.d.sync_100 += self.WDATA.eq(self.internal_axi_read_data)
                    m.d.comb += self.WDATA.eq(self.timed_axi_read_data)
                    m.d.sync_100 += self.axi_read_data_incremented.eq(1)
                    m.d.sync_100 += self.WVALID.eq(1)
                    m.d.sync_100 += self.internal_axi_read_address.eq(self.internal_axi_read_address + 1)
                    #m.d.sync_100 += self.last_axi_read_data.eq(self.internal_axi_read_data)
                    m.d.sync_100 += self.write_current_burst.eq(self.write_current_burst + 1)
                    m.next = "wait_write"
            
            with m.State("wait_write"):
                #m.d.sync_100 += self.debug_pins.eq(0b00001000)

                with m.If(self.write_addr_complete):
                    m.d.sync_100 += self.AWVALID.eq(0)
                with m.If(self.write_data_complete):
                    m.d.sync_100 += self.WVALID.eq(0)
                    with m.If(~self.WLAST):
                        m.d.sync_100 += self.internal_axi_read_address.eq(self.internal_axi_read_address + 1)
                        m.d.sync_100 += self.axi_read_data_incremented.eq(1)
                with m.If((~self.AWVALID) & (~self.WVALID) | (self.write_addr_complete & self.write_data_complete) | (~self.AWVALID & self.write_data_complete)):
                    with m.If(self.WLAST):
                        m.d.sync_100 += self.BREADY.eq(1)
                        m.d.comb += self.WDATA.eq(self.timed_axi_read_data)
                        m.next = "write_response_wait"
                        
                    with m.Else():
                        with m.If(self.write_current_burst == self.AWLEN):
                            m.d.sync_100 += self.WLAST.eq(1)
                        with m.Else():
                            m.d.sync_100 += self.WLAST.eq(0)

                        with m.If(self.internal_axi_read_valid):
                            #m.d.sync_100 += self.WDATA.eq(self.timed_axi_read_data)
                            m.d.comb += self.WDATA.eq(self.timed_axi_read_data)
                            m.d.sync_100 += self.WVALID.eq(1)
                            m.d.sync_100 += self.internal_axi_read_address.eq(self.internal_axi_read_address + 1)
                            m.d.sync_100 += self.last_axi_read_data.eq(self.internal_axi_read_data)
                            m.d.sync_100 += self.write_current_burst.eq(self.write_current_burst + 1)
                
                with m.Else():
                    m.d.comb += self.WDATA.eq(self.timed_axi_read_data)

            with m.State("write_response_wait"):
                #m.d.sync_100 += self.debug_pins.eq(0b00010000)
                
                with m.If(self.BVALID):

                    # TODO: do something with the response here

                    with m.If(self.write_bursts_remaining != 0):
                        m.d.sync_100 += self.AWADDR.eq(self.AWADDR + 16*8)
                        m.next = "set_write_address"
                    with m.Elif(self.write_stage != len(self.write_stages)-1):
                        m.d.sync_100 += self.write_stage.eq(self.write_stage + 1)
                        m.next = "get_write_config"
                    with m.Else():
                        m.next = "idle"


        # read
        self.internal_axi_write_address = Signal(range(self.LARGEST_MEMORY_REGION // 8 + 1))     # 64 bit block address
        self.internal_axi_write_data = Signal(64)
        self.internal_axi_write_ready = Signal()
        self.internal_axi_write_enable = Signal()

        with m.Switch(self.read_stage):
            with m.Case(0): # control
                with m.Switch(self.internal_axi_write_address):
                    with m.Case(0):
                        with m.If(self.internal_axi_write_enable):
                            m.d.sync_100 += self.cycle_timer_config.eq(self.internal_axi_write_data[0:16])
                            m.d.sync_100 += self.dma_instruction_block_select.eq(self.internal_axi_write_data[16:20])
                    with m.Default():
                        pass
                m.d.sync_100 += self.internal_axi_write_ready.eq(1)

            with m.Case(1): # data
                m.d.comb += self.data_write_axi_address.eq(self.internal_axi_write_address)
                m.d.comb += self.data_write_axi_data.eq(self.internal_axi_write_data)
                m.d.comb += self.data_write_axi_enable.eq(self.internal_axi_write_enable)
                m.d.sync_100 += self.internal_axi_write_ready.eq(1)

            with m.Case(2): # dma instructions
                m.d.comb += self.instruction_write_address.eq(self.internal_axi_write_address | self.dma_instruction_block_select << 12)    # TODO: verify this works
                m.d.comb += self.instruction_write_data.eq(self.internal_axi_write_data)
                m.d.comb += self.instruction_write_en.eq(self.internal_axi_write_enable)
                m.d.sync_100 += self.internal_axi_write_ready.eq(1)

        with m.FSM(init="idle", domain="sync_100"):
            with m.State("idle"):
                m.d.sync_100 += self.RREADY.eq(0)
                m.d.sync_100 += self.ARVALID.eq(0)
                m.d.sync_100 += self.axi_read_busy.eq(0)
                m.d.sync_100 += self.read_stage.eq(0)

                with m.If(self.axi_transfer_start):
                    m.d.sync_100 += self.axi_read_busy.eq(1)
                    m.next = "get_read_config"
            
            with m.State("get_read_config"):
                with m.Switch(self.read_stage):
                    for i, stage in self.read_stages.items():
                        with m.Case(i):
                            m.d.sync_100 += self.ARADDR.eq(int(stage["offset"] + self.OCM_BASE_ADDR))
                            m.d.sync_100 += self.read_bursts_remaining.eq(stage["burst_size"])

                m.d.sync_100 += self.internal_axi_write_address.eq(0)
                
                m.next = "set_read_address"

            with m.State("set_read_address"):
                with m.If(self.read_bursts_remaining >= 16):
                    m.d.sync_100 += self.ARLEN.eq(16-1)  # up to 16 burst length
                    m.d.sync_100 += self.read_bursts_remaining.eq(self.read_bursts_remaining - 16)
                with m.Else():
                    m.d.sync_100 += self.ARLEN.eq(self.read_bursts_remaining-1)
                    m.d.sync_100 += self.read_bursts_remaining.eq(0)
                m.d.sync_100 += self.read_current_burst.eq(0)
                m.d.sync_100 += self.ARVALID.eq(1)
                m.next = "get_read_data_wait"

            with m.State("get_read_data_wait"):
                with m.If(self.read_addr_complete):
                    m.d.sync_100 += self.ARVALID.eq(0)
                with m.If(self.RVALID & self.internal_axi_write_ready):
                    m.d.sync_100 += self.RREADY.eq(1)
                    m.d.comb += self.internal_axi_write_data.eq(self.RDATA)
                    
                with m.Else():
                    m.d.sync_100 += self.RREADY.eq(0)
                    #m.d.comb += self.internal_axi_write_enable.eq(0)

                with m.If(self.RVALID & self.RREADY):
                    m.d.sync_100 += self.internal_axi_write_address.eq(self.internal_axi_write_address + 1)
                    m.d.sync_100 += self.read_current_burst.eq(self.read_current_burst + 1)
                    m.d.comb += self.internal_axi_write_enable.eq(1)
                    

                    with m.If(self.RLAST):
                        #m.d.comb += self.internal_axi_write_enable.eq(0)
                        with m.If(self.read_bursts_remaining != 0):
                            m.d.sync_100 += self.ARADDR.eq(self.ARADDR + 16*8)
                            m.next = "set_read_address"
                        with m.Elif(self.read_stage != len(self.read_stages)-1):
                            m.d.sync_100 += self.read_stage.eq(self.read_stage + 1)
                            m.next = "get_read_config"
                        with m.Else():
                            m.next = "idle"
                        
                with m.Else():
                    m.d.comb += self.internal_axi_write_enable.eq(0)

        # main cycle trigger timer
        with m.If(self.cycle_timer == 0):
            m.d.sync_25 += self.cycle_timer.eq(self.cycle_timer_config)
        with m.Else():
            m.d.sync_25 += self.cycle_timer.eq(self.cycle_timer - 1)


        with m.FSM(init="idle", domain="sync_25"):
            with m.State("idle"):
                m.d.sync_100 += self.pl_ps_interrupts[0].eq(0)
                with m.If(self.cycle_timer == 0):
                    with m.If(self.cycle_timer_config != 0):    # writing zero to the timer will permanently stop the system (must be done before FPGA reconfiguration)
                        m.next = "start_dma"

            
            with m.State("start_dma"):
                m.d.sync_100 += self.shift_dma.start.eq(1)
                
                m.d.sync_100 += self.memory_update_running.eq(0)
                m.d.sync_100 += self.memory_update_done.eq(0)
                m.d.sync_100 += self.dma_cycle_done.eq(0)
                m.d.sync_100 += self.dma_cycle_running.eq(1)
                m.next = "run_dma"
            
            with m.State("run_dma"):
                m.d.sync_100 += self.shift_dma.start.eq(0)
                with m.If(~self.shift_dma.busy):
                    m.d.sync_100 += self.dma_cycle_done.eq(1)
                    m.d.sync_100 += self.dma_cycle_running.eq(0)
                    m.next = "start_axi_transfer"

            with m.State("start_axi_transfer"):
                m.d.sync_100 += self.axi_transfer_start.eq(1)
                m.d.sync_100 += self.memory_update_running.eq(1)
                m.next = "wait_axi_transfer"

            with m.State("wait_axi_transfer"):
                m.d.sync_100 += self.axi_transfer_start.eq(0)
                with m.If(~self.axi_transfer_busy):
                    m.d.sync_100 += self.memory_update_running.eq(0)
                    m.d.sync_100 += self.memory_update_done.eq(1)
                    m.d.sync_100 += self.pl_ps_interrupts[0].eq(1)
                    m.next = "idle"



        # connect rtl nodes

        device_map = {}

        device_map["controller"] = self.rm.export()

        previous_node_outputs = {
            "read_address" : self.shift_dma.read_bram_address_output,
            "write_address" : self.shift_dma.write_bram_address_output,
            "read_node" : self.shift_dma.read_node_address_output,
            "write_node" : self.shift_dma.write_node_address_output,
            "data" : self.shift_dma.data_output,
            "read_complete" : self.shift_dma.read_complete_output,
            "write_complete" : self.shift_dma.write_complete_output
        }
        node_address = 1
        for node_name, node_object in self.nodes.items(): # nodes are rtl modules that are linked together by the shift dma, they must have a shift dma node interface
            # add node to submodule list

            try:
                temp = m.submodules[node_name]
                raise Exception(f"Node name {node_name} is already in use, choose another name for external node")
            except AttributeError:
                pass
                
            
            if(node_address >= 256):
                raise Exception("Too many nodes, max is 255")
            
            

            m.submodules[node_name] = node_object

            #TODO: make these connections use amaranth's connect function

            # connect shift dma signals
            try:
                node_object.address = node_address
                m.d.sync_100 += [
                    node_object.read_bram_address_input.eq(previous_node_outputs["read_address"]),
                    node_object.write_bram_address_input.eq(previous_node_outputs["write_address"]),
                    node_object.read_node_address_input.eq(previous_node_outputs["read_node"]),
                    node_object.write_node_address_input.eq(previous_node_outputs["write_node"]),
                    node_object.data_input.eq(previous_node_outputs["data"]),
                    node_object.read_complete_input.eq(previous_node_outputs["read_complete"]),
                    node_object.write_complete_input.eq(previous_node_outputs["write_complete"]),
                ]
                previous_node_outputs = {
                    "read_address" : node_object.read_bram_address_output,
                    "write_address" : node_object.write_bram_address_output,
                    "read_node" : node_object.read_node_address_output,
                    "write_node" : node_object.write_node_address_output,
                    "data" : node_object.data_output,
                    "read_complete" : node_object.read_complete_output,
                    "write_complete" : node_object.write_complete_output
                }
            except AttributeError:
                # node does not have dma interface, attempt to connect using a bram interface
                
                m.submodules[f"{node_name}_shift_dma_{node_address}"] = shift_dma = shift_dma_node(node_address)

                try:
                    m.d.sync_100 += [
                        shift_dma.read_bram_address_input.eq(previous_node_outputs["read_address"]),
                        shift_dma.write_bram_address_input.eq(previous_node_outputs["write_address"]),
                        shift_dma.read_node_address_input.eq(previous_node_outputs["read_node"]),
                        shift_dma.write_node_address_input.eq(previous_node_outputs["write_node"]),
                        shift_dma.data_input.eq(previous_node_outputs["data"]),
                        shift_dma.read_complete_input.eq(previous_node_outputs["read_complete"]),
                        shift_dma.write_complete_input.eq(previous_node_outputs["write_complete"]),
                    ]
                    m.d.comb += [
                        node_object.bram_address.eq(shift_dma.bram_address),
                        node_object.bram_write_data.eq(shift_dma.bram_write_data),
                        node_object.bram_write_enable.eq(shift_dma.bram_write_enable),
                        shift_dma.bram_read_data.eq(node_object.bram_read_data),
                    ]
                    previous_node_outputs = {
                        "read_address" : shift_dma.read_bram_address_output,
                        "write_address" : shift_dma.write_bram_address_output,
                        "read_node" : shift_dma.read_node_address_output,
                        "write_node" : shift_dma.write_node_address_output,
                        "data" : shift_dma.data_output,
                        "read_complete" : shift_dma.read_complete_output,
                        "write_complete" : shift_dma.write_complete_output
                    }
                except AttributeError:
                    # node does not have dma interface or bram interface, cannot connect
                    raise Exception(f"Node {node_name} does not have a dma or bram interface, cannot connect to shift dma")


            

            # TODO: get register map from node and add to the main register map

            device_map[f"node_{node_address}_{node_name}"] = {
                "node_address" : node_address,
                "node" : node_object.rm.export()
            }

            # TODO: figure out how to handle card/slot IO (muxes?)

            node_address += 1


        # temporary hack to hardcode serial card to slot IO
        card = m.submodules["serial_card"]
        m.d.comb += [
            card.slotIn.eq(self.slot_B_in),
            self.slot_B_out.eq(card.slotOut),
            self.slot_B_out_enable.eq(card.slotOutEnable),
        ]

        encoders = m.submodules["fanuc_encoders"]
        m.d.comb += [
            encoders.rx[0].eq(card.rs422_rx[0]),
            card.rs422_tx[0].eq(encoders.tx[0]),
        ]

        timers = m.submodules["global_timers"]
        m.d.comb += [
            timers.trigger.eq(self.cycle_timer == 0),
            encoders.trigger.eq(timers.timer_pulse[0]),
            #self.debug_pins[0].eq(encoders.rx[0]),
            #self.debug_pins[1].eq(encoders.bram_read_data[0]),
            #self.debug_pins.eq(encoders.bram_read_data),
        ]

        serial_controller = m.submodules["em_serial_controller"]
        m.d.comb += [
            serial_controller.rx.eq(card.rs422_rx[9]),
            card.rs422_tx[9].eq(serial_controller.tx),

            self.debug_pins[0].eq(serial_controller.tx),
            self.debug_pins[1].eq(serial_controller.rx),
        ]
        
        #m.d.comb += self.debug_pins[2:].eq(serial_controller.debugPins)

        # with m.If((self.shift_dma.read_node_address_input == 4) & (self.shift_dma.read_bram_address_input == 0x1)): # read dev 0 status
        #     m.d.comb += self.debug_pins[2].eq(1)
        #     m.d.comb += self.debug_pins[3].eq(self.shift_dma.data_input[0])
        #     m.d.comb += self.debug_pins[4].eq(self.shift_dma.data_input[1])
        #     m.d.comb += self.debug_pins[5].eq(self.shift_dma.data_input[2])
        #     m.d.comb += self.debug_pins[6].eq(self.shift_dma.data_input[3])

        # with m.If((self.shift_dma.write_node_address_input == 0) & (self.shift_dma.write_bram_address_input == 6)):
        #     m.d.comb += self.debug_pins[3].eq(1)

        with m.If((self.shift_dma.data_memory_address == 8) & (self.shift_dma.data_memory_write_enable == 1)):
            m.d.comb += self.debug_pins[2].eq(1)
        m.d.comb += self.debug_pins[3].eq(self.shift_dma.data_memory_write_data[0])
        m.d.comb += self.debug_pins[4].eq(self.shift_dma.data_memory_write_data[1])
        m.d.comb += self.debug_pins[5].eq(self.shift_dma.data_memory_write_data[2])
        m.d.comb += self.debug_pins[6].eq(self.shift_dma.data_memory_write_data[3])

        #m.d.comb += self.debug_pins[5].eq(self.axi_transfer_busy)
        

        # m.d.comb += self.debug_pins[3].eq(serial_controller.bram_write_data[0])
        # m.d.comb += self.debug_pins[4].eq(serial_controller.bram_write_enable)
        # m.d.comb += self.debug_pins[3].eq(self.instruction_read_address[1])
        # m.d.comb += self.debug_pins[4].eq(self.instruction_read_address[2])
        # m.d.comb += self.debug_pins[5].eq(self.instruction_read_address[3])
        # m.d.comb += self.debug_pins[6].eq(self.instruction_read_address[4])
        # m.d.comb += self.debug_pins[7].eq(self.instruction_read_address[5])

        #m.d.comb += self.debug_pins.eq(m.submodules.fanuc_encoders.debug)

        # connect last node back to dma controller
        m.d.sync_100 += [
            self.shift_dma.read_node_address_input.eq(previous_node_outputs["read_node"]),
            self.shift_dma.write_node_address_input.eq(previous_node_outputs["write_node"]),
            self.shift_dma.read_bram_address_input.eq(previous_node_outputs["read_address"]),
            self.shift_dma.write_bram_address_input.eq(previous_node_outputs["write_address"]),
            self.shift_dma.data_input.eq(previous_node_outputs["data"]),
            self.shift_dma.read_complete_input.eq(previous_node_outputs["read_complete"]),
            self.shift_dma.write_complete_input.eq(previous_node_outputs["write_complete"]),
        ]

        import json
        with open("controller_config.json", "w") as file:
            json.dump(device_map, file, indent=4)


        return m

def create_instruction(source_node, destination_node, source_address, destination_address, instruction):
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    return data

sim = 0

nodes = {
    "serial_card" : serial_interface_card(),
    "fanuc_encoders" : Fanuc_Encoders(6),
    "global_timers" : Global_Timers(),
    "em_serial_controller" : EM_Serial_Controller(max_packet_size=64, max_number_of_devices=16),
}


dut = Controller(nodes, sim)

async def controller_test(ctx):

    ocm = {}
    for i in range(dut.PS_TO_PL_CONTROL_SIZE // 8):
        ocm[f"0x{dut.OCM_BASE_ADDR + dut.PS_TO_PL_CONTROL_OFFSET + i*8:08x}"] = 0x10
    for i in range(dut.PL_TO_PS_CONTROL_SIZE // 8):
        ocm[f"0x{dut.OCM_BASE_ADDR + dut.PL_TO_PS_CONTROL_OFFSET + i*8:08x}"] = 0x4
    for i in range(dut.PS_TO_PL_DATA_SIZE // 8):
        ocm[f"0x{dut.OCM_BASE_ADDR + dut.PS_TO_PL_DATA_OFFSET + i*8:08x}"] = 0x5
    for i in range(dut.PL_TO_PS_DATA_SIZE // 8):
        ocm[f"0x{dut.OCM_BASE_ADDR + dut.PL_TO_PS_DATA_OFFSET + i*8:08x}"] = 0x6
    for i in range(dut.PS_TO_PL_DMA_INSTRUCTION_SIZE // 8):
        instruction = create_instruction(2, 0, 0, 0, dut.shift_dma.Instruction.COPY)
        if(i != 0):
            instruction = create_instruction(0, 0, 0, 0, dut.shift_dma.Instruction.NOP)
        ocm[f"0x{dut.OCM_BASE_ADDR + dut.PS_TO_PL_DMA_INSTRUCTION_OFFSET + i*8:08x}"] = instruction
   

    #ocm["0x000F0000"] = 0x10 # cycle timer config
    

    RVALID_OFF = 0
    read_burst_count = 0
    write_base_addr = 0
    read_base_addr = 0

    await ctx.tick("sync_200").repeat(2)
    for i in range(1000):
        await ctx.tick("sync_200").repeat(2)

        ctx.set(dut.RVALID, not RVALID_OFF)

        for x in range(1):
            # write axi
            if(ctx.get(dut.AWVALID)):
                ctx.set(dut.AWREADY, 1)
                write_base_addr = ctx.get(dut.AWADDR)
                break
            ctx.set(dut.AWREADY, 0)
            if(ctx.get(dut.WVALID)):
                #await ctx.tick("sync_200").repeat(2)
                ctx.set(dut.WREADY, 1)
                if(f"0x{write_base_addr:08x}" not in ocm):
                    raise Exception(f"Write address 0x{write_base_addr:08x} not in OCM")
                ocm[f"0x{write_base_addr:08x}"] = ctx.get(dut.WDATA)
                write_base_addr += 8
                break
            ctx.set(dut.WREADY, 0)
            if(ctx.get(dut.BREADY) & ctx.get(dut.WLAST)):
                ctx.set(dut.BVALID, 1)
                break
            ctx.set(dut.BVALID, 0)

        for x in range(1):
            # read axi
            if(ctx.get(dut.ARVALID)):
                ctx.set(dut.ARREADY, 1)
                ctx.set(dut.RVALID, 1)
                read_burst_count = ctx.get(dut.ARLEN)-1
                read_base_addr = ctx.get(dut.ARADDR)
                if(f"0x{read_base_addr:08x}" not in ocm):
                    raise Exception(f"Read address 0x{read_base_addr:08x} not in OCM")
                ctx.set(dut.RDATA, ocm[f"0x{read_base_addr:08x}"])
                ctx.set(dut.RLAST, 0)
                RVALID_OFF = 0
                break
            ctx.set(dut.ARREADY, 0)
            
            if(ctx.get(dut.RREADY) and ctx.get(dut.RVALID) and read_burst_count != 0):
                #RVALID_OFF = 1
                read_burst_count -= 1
                read_base_addr += 8
                if(f"0x{read_base_addr:08x}" not in ocm):
                    raise Exception(f"Read address 0x{read_base_addr:08x} not in OCM")
                ctx.set(dut.RDATA, ocm[f"0x{read_base_addr:08x}"])
                break
            if(read_burst_count == 0):
                RVALID_OFF = 1
                ctx.set(dut.RLAST, 1)
                break

    return



if __name__ == "__main__":

    #print(f"0x{create_instruction(0, 0, 0x1000//4, 0x0, shift_dma_controller.Instruction.COPY):016x}")

    if(sim):

        from amaranth.sim import Simulator
        sim = Simulator(dut)
        sim.add_clock(1/200e6, domain="sync_200")
        sim.add_clock(1/100e6, domain="sync_100")
        sim.add_clock(1/50e6, domain="sync_50")
        sim.add_clock(1/25e6, domain="sync_25")
        sim.add_testbench(controller_test)
        with sim.write_vcd("controller_test.vcd"):
            sim.run()

    if (not sim):  # export
        top = Controller(nodes, sim)

        from amaranth.back import verilog
        with open("controller-firmware/Vivado/autogen_sources/controller.v", "w") as f:
            f.write(verilog.convert(top, name="Controller"))
