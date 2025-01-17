from amaranth import *
from amaranth.lib import wiring
from amaranth.lib.wiring import In, Out



class AXI_Master(wiring.Component):
    def __init__(self, data_width=64, addr_width=32, id_width=4, user_width=0, interface_name="AXI_Master"):

        # Define the interface parameters
        interface_params = (
            f"XIL_INTERFACENAME {interface_name}, "
            "CLK_DOMAIN controller_firmware_processing_system7_0_0_FCLK_CLK3, "
            "FREQ_HZ 200000000, "
            "PHASE 0.0, "
            "PROTOCOL AXI4, "
            f"DATA_WIDTH {data_width}, "
            f"ID_WIDTH {id_width}, "
            f"ADDR_WIDTH {addr_width}, "
            "HAS_BURST 0, "
            "HAS_CACHE 0, "
            "HAS_LOCK 0, "
            "HAS_PROT 0, "
            "HAS_QOS 0, "
            "HAS_REGION 0, "
            "HAS_WSTRB 1, "
            "HAS_BRESP 1, "
            "HAS_RRESP 0, "
            "SUPPORTS_NARROW_BURST 1, "
            "MAX_BURST_LENGTH 256, "
            "NUM_READ_OUTSTANDING 1, "
            "NUM_WRITE_OUTSTANDING 1, "
            "READ_WRITE_MODE READ_WRITE"
        )

        super().__init__({
            # Clock and Reset
            "ACLK": In(1),
            "ARESETN": In(1),

            # Write address channel
            "AWID": Out(id_width),
            "AWADDR": Out(addr_width),
            "AWLEN": Out(8),
            "AWSIZE": Out(3),
            "AWBURST": Out(2),
            "AWLOCK": Out(1),    # unused
            "AWCACHE": Out(4),   # unused
            "AWPROT": Out(3),    # unused
            "AWREGION": Out(4),  # unused
            "AWQOS": Out(4),     # unused
            "AWUSER": Out(user_width),
            "AWVALID": Out(1),
            "AWREADY": In(1),

            # Write data channel
            "WID": Out(id_width),
            "WDATA": Out(data_width),
            "WSTRB": Out(data_width // 8),   
            "WLAST": Out(1),
            "WUSER": Out(user_width),    # unused
            "WVALID": Out(1),
            "WREADY": In(1),

            # Write response channel
            "BID": In(id_width),
            "BRESP": In(2),    # unused
            "BUSER": In(user_width),   # unused
            "BVALID": In(1),
            "BREADY": Out(1),

            # Read address channel
            "ARID": Out(id_width),
            "ARADDR": Out(addr_width),
            "ARLEN": Out(8),
            "ARSIZE": Out(3),
            "ARBURST": Out(2),
            "ARLOCK": Out(1),    # unused
            "ARCACHE": Out(4),   # unused
            "ARPROT": Out(3),    # unused
            "ARREGION": Out(4),  # unused
            "ARQOS": Out(4),     # unused
            "ARUSER": Out(user_width),   # unused
            "ARVALID": Out(1),
            "ARREADY": In(1),

            # Read data channel
            "RID": In(id_width),
            "RDATA": In(data_width),
            "RRESP": In(2),    # unused
            "RLAST": In(1),
            "RUSER": In(user_width),   # unused
            "RVALID": In(1),
            "RREADY": Out(1),

            "testTrigger": In(1),
            "LEDdebug": Out(3, init=0b111)
        })

        # Assign attributes to the signals

        self.ACLK.attrs["X_INTERFACE_INFO"] = f"xilinx.com:signal:clock:1.0 {interface_name} CLK"
        self.ARESETN.attrs["X_INTERFACE_INFO"] = f"xilinx.com:signal:reset:1.0 {interface_name} RST"
        self.AWID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWID"
        self.AWADDR.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWADDR"
        self.AWLEN.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWLEN"
        self.AWSIZE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWSIZE"
        self.AWBURST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWBURST"
        self.AWLOCK.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWLOCK"
        self.AWCACHE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWCACHE"
        self.AWPROT.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWPROT"
        self.AWREGION.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWREGION"
        self.AWQOS.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWQOS"
        self.AWUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWUSER"
        self.AWVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWVALID"
        self.AWREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} AWREADY"
        self.WID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WID"
        self.WDATA.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WDATA"
        self.WSTRB.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WSTRB"
        self.WLAST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WLAST"
        self.WUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WUSER"
        self.WVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WVALID"
        self.WREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} WREADY"
        self.BID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BID"
        self.BRESP.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BRESP"
        self.BUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BUSER"
        self.BVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BVALID"
        self.BREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} BREADY"
        self.ARID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARID"
        self.ARADDR.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARADDR"
        self.ARLEN.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARLEN"
        self.ARSIZE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARSIZE"
        self.ARBURST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARBURST"
        self.ARLOCK.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARLOCK"
        self.ARCACHE.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARCACHE"
        self.ARPROT.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARPROT"
        self.ARREGION.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARREGION"
        self.ARQOS.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARQOS"
        self.ARUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARUSER"
        self.ARVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARVALID"
        self.ARREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} ARREADY"
        self.RID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RID"
        self.RDATA.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RDATA"
        self.RRESP.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RRESP"
        self.RLAST.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RLAST"
        self.RUSER.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RUSER"
        self.RVALID.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RVALID"
        self.RREADY.attrs["X_INTERFACE_INFO"] = f"xilinx.com:interface:aximm:1.0 {interface_name} RREADY"
        
        # Assign interface-level attributes to one of the signals
        self.ACLK.attrs["X_INTERFACE_PARAMETER"] = interface_params

    def elaborate(self, platform):
        m = Module()

        # Create a clock domain and connect ACLK and ARESETN
        m.domains.sync = ClockDomain("sync", async_reset=True)
        m.d.comb += [
            ClockSignal("sync").eq(self.ACLK),
            ResetSignal("sync").eq(~self.ARESETN)
        ]

        self.write_address = 0x000F0060
        self.write_data = 0x12345678

        self.awready_set = Signal()
        self.wready_set = Signal()

        self.write_trigger = Signal()
        self.last_testTrigger = Signal()

        with m.If((self.testTrigger != self.last_testTrigger) & (self.testTrigger == 1)):
            m.d.sync += self.write_trigger.eq(1)
        with m.Else():
            m.d.sync += self.write_trigger.eq(0)

        m.d.sync += self.last_testTrigger.eq(self.testTrigger)


        with m.FSM(init="write_idle"):

            with m.State("write_idle"):
                m.d.sync += self.LEDdebug.eq(0b111)
                with m.If(self.write_trigger):
                    m.d.sync += self.awready_set.eq(0)
                    m.d.sync += self.wready_set.eq(0)
                    m.next = "write_start"

            with m.State("write_start"):
                # set address and data and signify both valid
                m.d.sync += self.AWID.eq(0)
                m.d.sync += self.WID.eq(0)
                m.d.sync += self.WSTRB.eq(0b11111111)
                m.d.sync += self.AWSIZE.eq(0b110)   # 64 bit
                m.d.sync += self.AWADDR.eq(self.write_address)
                m.d.sync += self.AWVALID.eq(1)
                m.d.sync += self.WDATA.eq(self.write_data)
                m.d.sync += self.WVALID.eq(1)
                m.d.sync += self.WLAST.eq(1)
                m.d.sync += self.BREADY.eq(1)
                m.d.sync += self.LEDdebug.eq(0b110)
                with m.If(self.AWREADY):
                    m.d.sync += self.awready_set.eq(1)
                with m.If(self.WREADY):
                    m.d.sync += self.wready_set.eq(1)
                m.next = "write_wait"

            with m.State("write_wait"):
                with m.If(self.AWREADY | self.awready_set):    # address write done
                    m.d.sync += self.AWVALID.eq(0)
                with m.If(self.WREADY | self.wready_set ):    # data write done
                    m.d.sync += self.WVALID.eq(0)
                    #m.d.sync += self.WLAST.eq(0)
                    m.next = "write_response"
                m.d.sync += self.LEDdebug.eq(0b101)

            with m.State("write_response"):
                m.d.sync += self.LEDdebug.eq(0b011)
                with m.If(self.BVALID):
                    m.d.sync += self.BREADY.eq(0)
                    m.next = "write_idle"
                

        return m

clock = 100e6
dut = AXI_Master()

async def axi_master_test(ctx):
    #ctx.set(dut.AWREADY, 1)
    #ctx.set(dut.WREADY, 1)

    await ctx.tick()
    await ctx.tick()
    ctx.set(dut.testTrigger, 1)
    await ctx.tick()
    #ctx.set(dut.testTrigger, 0)
    await ctx.tick().repeat(4)
    ctx.set(dut.AWREADY, 1)
    await ctx.tick()
    #ctx.set(dut.AWREADY, 0)
    #await ctx.tick().repeat(4)
    ctx.set(dut.WREADY, 1)
    await ctx.tick()
    #ctx.set(dut.WREADY, 0)
    #await ctx.tick().repeat(1)
    ctx.set(dut.BVALID, 1)
    await ctx.tick()
    ctx.set(dut.BVALID, 0)
    await ctx.tick().repeat(4)
    ctx.set(dut.testTrigger, 1)
    await ctx.tick()
    ctx.set(dut.testTrigger, 0)
    await ctx.tick().repeat(4)

if __name__ == "__main__":

    # from amaranth.sim import Simulator
    # sim = Simulator(dut)
    # sim.add_clock(1/clock)
    # sim.add_testbench(axi_master_test)
    # with sim.write_vcd("axi_master_test.vcd"):
    #     sim.run()

    if (True):  # export
        top = AXI_Master()

        from amaranth.back import verilog
        with open("controller-firmware/Vivado/autogen_sources/axi_master.v", "w") as f:
            f.write(verilog.convert(top, name="AXI_Master"))
