from amaranth import *
from amaranth.sim import Simulator
from amaranth.back import verilog


class controller(Elaboratable):

    def __init__(self, clock) -> None:
        self.clock = clock
       
    def elaborate(self, platform):
        m = Module()

        return m
    

            
if __name__ == "__main__":

    if (True):  # export
        top = controller(100e6)
        with open("controller-firmware/src/amaranth sources/controller.v", "w") as f:
            f.write(verilog.convert(top, name="controller", ports=top.ports))