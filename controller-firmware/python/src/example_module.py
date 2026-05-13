from amaranth import *
from amaranth.sim import Simulator
from amaranth.lib.wiring import Component, In, Out

from registers2 import* # register mapping


class Example_Module(Component):
    # Example module that does nothing

    def __init__(self, number_of_instances=1):
        """
        Init is called upon instantiation of the module.
        Any custom parameters can be passed here.
        """

        # Validate the number of instances
        assert number_of_instances > 0
        assert number_of_instances <= 4
        
        # just an example, not actually used
        self.number_of_instances = number_of_instances

        super().__init__({  # create any ports into/out of the module here
            
            # memory access ports, required for all modules
            "bram_address": In(16),
            "bram_write_data": In(32),
            "bram_read_data": Out(32),
            "bram_write_enable": In(1),
            # TODO: make this an amaranth interface

            # math/processing modules may have only the memory access ports

            # modules with external IO will have additional ports
            "tx": Out(1),
            "rx": In(1),

            
            # debug is useful for connecting to a logic analyzer for final testing, not required
            "debug": Out(8)
        })

        # Any driver settings will be passed to the software driver
        # typically for specifying how the driver is configured (ex: how many encoders were instantiated)
        driver_settings = {
            "number_of_instances": self.number_of_instances,
            "version": "1.0.0",  # add versioning if multiple versions of the module are used
        }

        # Create the register map for this module
        self.rm = RegisterMapGenerator("example_module", compatible_drivers=["example_module"], driver_settings=driver_settings, desc="Example Module Description")
        # compatible_drivers is a list of software driver names that can use this module,
        #   if none are available, the module will not automatically be acessible from the software


        # Add registers to the register map

        # All physical registers are 32 bits wide, but actual data may be less than that

        # individual registers
        self.rm.add(Register("example_register_0", rw="r", type="signed", width=32, desc="Example register description 0"))
        self.rm.add(Register("example_register_1", rw="w", type="unsigned", width=16, desc="Example register description 1"))
        self.rm.add(Register("example_register_2", rw="r", type="unsigned", width=5, desc="Example register description 2"))

        # array of registers
        self.rm.add(Register("example_register_array", rw="w", type="signed", width=32, desc="Example register array description", bank_size=32))
        # 32 registers in the array, registers may be of any type or size, but will still take 32 consecutive addresses

        # packed registers
        # it is a waste of space to have a full 32 bit memory space if you only have boolean or small values
        # so you can use a packed register to save space
        # it combines multiple sub-registers into a single register
        self.rm.add(Register("example_packed_register", rw="r", desc="Encoder status", sub_registers=[
            Register("small_flag_0", type="bool", desc="single bit"),
            Register("small_flag_1", type="bool", desc="single bit"),
            Register("small_flag_2", type="bool", desc="single bit"),
            Register("small_value_0", type="unsigned", width=8, desc="small value"),
            Register("small_value_1", type="unsigned", width=8, desc="another small value"),
        ]))
        # note that all sub-registers will inherit the rw permissions of the parent register
        # sub-registers may be placed at specific starting bits using the 'start_address'

        # any registers may be placed at a specific address using the 'start_address' parameter, placement is automatic otherwise


        # groups
        # in many cases it is useful to group registers together and have multiple sets of them
        example_group = Group("example_group", count=4, start_address=0x0, desc="Group of registers example")

        # you may add as many registers as you like to a group, including arrays and packed registers
        # TODO: do groups of groups work?
        example_group.add(Register("group_register_0", rw="r", type="unsigned", width=16, desc="Group register 0"))
        example_group.add(Register("group_register_1", rw="r", type="unsigned", width=16, desc="Group register 1"))
        example_group.add(Register("group_register_2", rw="w", type="unsigned", width=16, desc="Group register 2"))
    
        # you can also specify the start address of the group, if not placement is automatic
        # if the 'alignement' parameter is not set, it will be set to the smallest size that fits all registers and is a power of 2

        # don't forget to add the group to the register map after filling it
        self.rm.add(example_group)

        # generate the register map
        self.rm.generate()


    def elaborate(self, platform):
        """
        Elaborate is called to create the hardware description of the module.
        This is where you define the logic of the module.
        """

        m = Module()

        m.d.sync_100 += self.debug.eq(0)

        # no logic is shown in this example
        # see the amaranth documentation for more information on how to create these.

        # there are 4 syncronized clocks to choose from:
        # m.d.sync_200  # 200 MHz clock
        # m.d.sync_100  # 100 MHz clock (memory interface uses this clock)
        # m.d.sync_50   # 50 MHz clock
        # m.d.sync_25   # 25 MHz clock

        # most modules will use only the 100 MHz clock as it is still pretty easy to meet timing requirements with it
        

        # accessing the generated register map

        """
        Registers all include the following properties:
            - address_offset: the address offset of the register in the memory map,
                relative to the start of the containing group if applicable
            - width: the width of the register in bits
            - starting_bit: the starting bit of the register (0 for full registers, will vary for sub-registers)
            - bank_size: the size of the register bank, (1 for individual registers, larger for arrays)
            - description: a description of the group
        """
        # get the address of a register
        example_register_0_address = self.rm.example_register_0.address_offset
        
        # get the width of a register
        example_register_0_width = self.rm.example_register_0.width


        """
        Groups include the following properties:
            - offset: the starting address of the group in the memory map,
                relative to the start of the containing group if applicable
            - count: the number of instances of the group
            - alignment: the alignment of the group address space (default is the smallest power of 2 that fits all registers)
                power of 2 alignment makes addressing significantly easier and more efficient than using the smallest size
            - description: a description of the group
        """
        # get the address of a group
        example_group_address = self.rm.example_group.offset
        # get the count of a group
        example_group_count = self.rm.example_group.count
        # get the alignment of a group
        example_group_alignment = self.rm.example_group.alignment

        # resulting actual absolute address of the group set would be:
        group_index = 0  # index of the group instance, 0 for the first instance
        absolute_address = example_group_address + (example_group_alignment * group_index)

        # then that address can be used when referencing the registers in the group
        # for example, to access the group_register_0 register absolute address:
        group_0_register_0_address = absolute_address + self.rm.example_group.group_register_0.address_offset



        return m
    


dut = Example_Module(number_of_instances=2)

async def bench(ctx):
    # testbench for the Example_Module

    # run your own tests here
    # check the amaranth documentation for more information on how to create a testbench

    pass



if __name__ == "__main__":

    #dut.rm.exportJSON("example_module.json")  # just for viewing, not required to be done

    sim = Simulator(dut)

    #sim.add_clock(1/200e6, domain="sync_200")  # no need to add if we aren't using it
    sim.add_clock(1/100e6, domain="sync_100")
    #sim.add_clock(1/50e6, domain="sync_50")
    #sim.add_clock(1/25e6, domain="sync_25")
    

    # add the testbench
    sim.add_testbench(bench)

    # run the simulation
    with sim.write_vcd("example_module.vcd"):
        sim.run()

    # you can now open the generated VCD file in a waveform viewer


# JSON export register map for reference
"""
{
    "name": "example_module",
    "compatible_drivers": [
        "example_module"
    ],
    "driver_settings": {
        "number_of_instances": 2,
        "version": "1.0.0"
    },
    "base_group": {
        "name": "base_group",
        "address_offset": 0,
        "description": "Base group for all registers",
        "alignment": 65536,
        "count": 1,
        "groups": {
            "example_group": {
                "name": "example_group",
                "address_offset": 0,
                "description": "Group of registers example",
                "alignment": 4,
                "count": 4,
                "groups": {},
                "registers": {
                    "group_register_0": {
                        "name": "group_register_0",
                        "address_offset": 0,
                        "type": "unsigned",
                        "bank_size": 1,
                        "description": "Group register 0",
                        "width": 16,
                        "starting_bit": 0,
                        "sub_registers": {},
                        "rw": "r"
                    },
                    "group_register_1": {
                        "name": "group_register_1",
                        "address_offset": 1,
                        "type": "unsigned",
                        "bank_size": 1,
                        "description": "Group register 1",
                        "width": 16,
                        "starting_bit": 0,
                        "sub_registers": {},
                        "rw": "r"
                    },
                    "group_register_2": {
                        "name": "group_register_2",
                        "address_offset": 2,
                        "type": "unsigned",
                        "bank_size": 1,
                        "description": "Group register 2",
                        "width": 16,
                        "starting_bit": 0,
                        "sub_registers": {},
                        "rw": "w"
                    }
                }
            }
        },
        "registers": {
            "example_register_0": {
                "name": "example_register_0",
                "address_offset": 16,
                "type": "signed",
                "bank_size": 1,
                "description": "Example register description 0",
                "width": 32,
                "starting_bit": 0,
                "sub_registers": {},
                "rw": "r"
            },
            "example_register_1": {
                "name": "example_register_1",
                "address_offset": 17,
                "type": "unsigned",
                "bank_size": 1,
                "description": "Example register description 1",
                "width": 16,
                "starting_bit": 0,
                "sub_registers": {},
                "rw": "w"
            },
            "example_register_2": {
                "name": "example_register_2",
                "address_offset": 18,
                "type": "unsigned",
                "bank_size": 1,
                "description": "Example register description 2",
                "width": 5,
                "starting_bit": 0,
                "sub_registers": {},
                "rw": "r"
            },
            "example_register_array": {
                "name": "example_register_array",
                "address_offset": 19,
                "type": "signed",
                "bank_size": 32,
                "description": "Example register array description",
                "width": 32,
                "starting_bit": 0,
                "sub_registers": {},
                "rw": "w"
            },
            "example_packed_register": {
                "name": "example_packed_register",
                "address_offset": 51,
                "type": "unsigned",
                "bank_size": 1,
                "description": "Encoder status",
                "width": 32,
                "starting_bit": 0,
                "sub_registers": {
                    "small_flag_0": {
                        "name": "small_flag_0",
                        "address_offset": 51,
                        "type": "bool",
                        "bank_size": 1,
                        "description": "single bit",
                        "width": 1,
                        "starting_bit": 0,
                        "sub_registers": {},
                        "rw": "r"
                    },
                    "small_flag_1": {
                        "name": "small_flag_1",
                        "address_offset": 51,
                        "type": "bool",
                        "bank_size": 1,
                        "description": "single bit",
                        "width": 1,
                        "starting_bit": 1,
                        "sub_registers": {},
                        "rw": "r"
                    },
                    "small_flag_2": {
                        "name": "small_flag_2",
                        "address_offset": 51,
                        "type": "bool",
                        "bank_size": 1,
                        "description": "single bit",
                        "width": 1,
                        "starting_bit": 2,
                        "sub_registers": {},
                        "rw": "r"
                    },
                    "small_value_0": {
                        "name": "small_value_0",
                        "address_offset": 51,
                        "type": "unsigned",
                        "bank_size": 1,
                        "description": "small value",
                        "width": 8,
                        "starting_bit": 3,
                        "sub_registers": {},
                        "rw": "r"
                    },
                    "small_value_1": {
                        "name": "small_value_1",
                        "address_offset": 51,
                        "type": "unsigned",
                        "bank_size": 1,
                        "description": "another small value",
                        "width": 8,
                        "starting_bit": 11,
                        "sub_registers": {},
                        "rw": "r"
                    }
                },
                "rw": "r"
            }
        }
    }
}
"""