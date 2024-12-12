from enum import IntEnum, auto
from typing import Union
import traceback

"""
Things to support:

- Register read/write:
    - Read only
    - Write only
    sub-registers may not have different permissions than the parent register

- Variable types:
    - unsigned
    - signed
    - bool

- Variable sizes:
    - 1-32 bits

- Single value registers:
    - 1-32 bits
    - fixed/auto start address
    - starting bit is always 0 (no bit offsets)

- Sub-registers:
    - 1-32 bits
    - fixed/auto start bit offset (inside the parent register)
    - same type options as single value registers
    - read/write permissions are inherited from the parent register
    - sub-registers cannot be nested
    - sub-registers cannot be banks or groups

- Register banks:
    many registers with the same configuration, like an array. used to store many values of the same type
    - 1-65536 registers
    - consecutive addresses
    - fixed/auto start address
    - single value or sub-registers supported
    - compressed to a single entry in the register map

- Register groups:
    - multiple registers with different configurations
    - fixed/auto start address
    - fixed/auto address alignment
    - all packing types supported (single value, sub-registers, register banks, and register groups) (group nesting is allowed)

- Register map:
    - should contain an entry for every register, sub-register, register bank, and register group specified

"""




"""
example functions:

a single register:
Register("name", type="unsigned", width=8, rw="r", start_address=0x0, desc="description")

sub-registers:
Register("name", rw="r", start_address=0x0, desc="description", sub_registers=[     # type and width are not set in the main register
    Register("sub1", "unsigned", width=4),  # rw is inherited from the parent register
    Register("sub2", "unsigned", width=4, start_address=0x4),  # start_address is relative to the parent register and becomes the bit offset
])

register bank:
any register can be turned into a bank by specifying the bank size
Register("name", rw="r", start_address=0x0, desc="description", bank_size=4)
Register("name", rw="r", start_address=0x0, desc="description", bank_size=4, sub_registers=[
    Register("sub1", "unsigned", width=4),
    Register("sub2", "unsigned", width=4, start_address=0x4),
])

register group:
g = Group("name", start_address=0x0, desc="description", alignment=4, count=4)  # group needs created before anything can be added to it

g.add(Register("name", "unsigned", width=8, rw="r", desc="description"), start_address=0x0)    # any of the above register types can be added to a group, start_address is relative to the group instance
g.add(Register("name", rw="r", desc="description", sub_registers=[
    Register("sub1", "unsigned", width=4),
    Register("sub2", "unsigned", width=4, start_address=0x4),
]), start_address=0x4)
g.add(Register("name", rw="r", desc="description", bank_size=4))    # ommitted start_address will be auto-assigned

g.add(Group("name", desc="description", alignment=4, count=4), start_address=0x80)  # groups can be nested

groups without an allignment will have the smallest alignment automatically set (power of 2)
if allignment is specified, the group will be padded to the specified alignment
allignment must be a power of 2


"""

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class Register:
    def __init__(self, name: str, rw: str="", type: str="unsigned", width: int=32, start_address: int=None, desc:str="", bank_size:int=1, sub_registers:list=[]):
        """
        Initialize a Register object.
        Args:
            name (str): The name of the register.
            rw (str): Read/Write access type, either 'r' for read or 'w' for write.
            type (str, optional): The data type of the register. Must be one of "unsigned", "signed", or "bool". Defaults to "unsigned".
            width (int, optional): The bit width of the register. Must be between 1 and 32. Defaults to 32.
            start_address (int, optional): The starting address of the register. Must be between 0 and 0xFFFF.
            desc (str, optional): A description of the register.
            bank_size (int, optional): The size of the register bank. Must be between 1 and 65536. Defaults to 1.
            sub_registers (list, optional): A list of sub-registers. Each sub-register must be an instance of Register.
        Raises:
            ValueError: If any of the provided arguments are invalid.
        """

        if type not in ["unsigned", "signed", "bool"]:
            raise ValueError("Invalid type")
        
        if width not in range(1, 33):
            raise ValueError("Invalid width")
        
        if start_address is not None and (start_address < 0 or start_address > 0xFFFF):
            raise ValueError("Invalid start_address")
        
        if bank_size is not None and bank_size not in range(1, 0xFFFF+1):
            raise ValueError("Invalid bank_size")

        if rw != "" and rw not in ["r", "w"]:
            raise ValueError("Invalid rw")
        
        for sub_register in sub_registers:
            if not isinstance(sub_register, Register):
                raise ValueError("Invalid sub_register")
        
        self.name = name
        self.type = type
        self.width = width
        self.rw = rw
        self.start_address = start_address
        self.desc = desc
        self.bank_size = bank_size

        self.map = {}

        # convert sub_registers list to a dictionary for easy access
        self.sub_registers = {}
        for sub_register in sub_registers:
            self.sub_registers[sub_register.name] = sub_register


        self.used_bits = []
        self.unassigned_regs = []

        self.used_addresses = []

        self.is_sub_register = False


    def __gen(self, sub_register: 'Register', starting_bit: int):
        """
        INTERNAL\n
        Generate the register map for a sub-register.
        Args:
            sub_register (Register): The sub-register to generate the map for.
            starting_bit (int): The starting bit of the sub-register.
        """
            
        sub_register.rw = self.rw   # inherit rw from parent register
        sub_register.generate()
        sub_register.map["address_offset"] = self.map["address_offset"]  # inherit address_offset from parent register
        sub_register.map["starting_bit"] = starting_bit  # set starting_bit to the start_address of the sub-register

        #self.used_addresses = range(self.map["address_offset"], self.map["address_offset"] + self.bank_size)

        if not self.__bits_available(starting_bit, sub_register.width):
            raise ValueError("Invalid sub_register, bits are already in use")

        self.map["sub_registers"][sub_register.name] = sub_register.map
        self.__use_bits(starting_bit, sub_register.width)

        print(f"{bcolors.OKGREEN}Register gen: Register '{sub_register.name}' has been placed at bits {starting_bit}:{starting_bit+sub_register.width-1} in Register '{self.name}'{bcolors.ENDC}")

    def generate(self):
        """
        INTERNAL\n
        Generate the register map for the register.
        """

        if self.type == "bool" and self.width != 1:
            print(f"{bcolors.WARNING}Register gen: Register '{self.name}' is a bool type, width will be set to 1{bcolors.ENDC}")
            self.width = 1

        self.generated = False
        self.used_bits = []
        self.unassigned_regs = []

        # these values are not dependent on the sub-registers, so they can be set here
        self.map["name"] = self.name
        self.map["address_offset"] = self.start_address
        self.map["type"] = self.type
        self.map["bank_size"] = self.bank_size
        self.map["description"] = self.desc
        self.map["width"] = self.width
        self.map["starting_bit"] = 0

        if self.start_address is None:
            self.used_addresses = range(self.bank_size)
        else:
            self.used_addresses = range(self.start_address, self.start_address + self.bank_size)

        # empty sub_registers dict to start
        self.map["sub_registers"] = {}

        if self.rw == "":
            raise ValueError("rw must be set for non-sub-registers")
        self.map["rw"] = self.rw

        if not self.sub_registers:
            self.generated = True
            return
        

        # place sub-registers
        for sub_register in self.sub_registers.values():

            if sub_register.type == "bool" and sub_register.width != 1:
                print(f"{bcolors.WARNING}Register gen: Register '{sub_register.name}' is a bool type, width will be set to 1{bcolors.ENDC}")
                sub_register.width = 1

            sub_register.is_sub_register = True

            if sub_register.rw != "" and sub_register.rw != self.rw:
                raise ValueError("Sub-register rw must match parent register rw")
            
            if sub_register.bank_size != 1:
                raise ValueError("Sub-registers cannot be banks")

            if sub_register.start_address is None:   # if start_address is not set, add to unassigned_regs list, it will be auto-assigned later
                self.unassigned_regs.append(sub_register)
                continue

            self.__gen(sub_register, sub_register.start_address)
            
        # auto-assign start_address for sub-registers
        for sub_register in self.unassigned_regs:
            success = False
            for starting_bit in range(0, 32-sub_register.width+1):
                if self.__bits_available(starting_bit, sub_register.width):
                    self.__gen(sub_register, starting_bit)
                    success = True
                    break

            if not success:
                raise ValueError(f"Unable to auto-assign start_address for sub-register: {sub_register}, space not available")
            

        # make sure parent width is large enough to hold all sub-registers
        if self.width < max(self.used_bits) + 1:
            print(f"{bcolors.WARNING}Register gen: Parent register '{self.name}' width {self.width} is too small to hold all sub-registers, width will be increased to {max(self.used_bits) + 1}{bcolors.ENDC}")

        self.generated = True


    def __bits_available(self, starting_bit, width):
        """
        INTERNAL\n
        Check if a range of bits is available.
        Args:
            starting_bit (int): The starting bit of the range.
            width (int): The width of the range.
            used_bits (list): A list of used bits.
        Returns:
            bool: True if the range is available, False otherwise.
        """

        for i in range(starting_bit, starting_bit + width):
            if i in self.used_bits:
                return False
        
        return True
    
    def __use_bits(self, starting_bit, width):
        """
        INTERNAL\n
        Mark a range of bits as used.
        Args:
            starting_bit (int): The starting bit of the range.
            width (int): The width of the range. 
             used_bits (list): A list of used bits.
        """

        self.used_bits.extend(range(starting_bit, starting_bit + width))

    def post_assign_address_offset(self, address_offset):
        """
        INTERNAL\n
        Assign a base address to the register and all sub-registers.
        Args:
            address_offset (int): The base address to assign.
        """

        if not self.generated:
            raise ValueError("Register map must be generated before assigning a base address")
        
        self.map["address_offset"] = address_offset
        for sub_register in self.sub_registers.values():
            sub_register.map["address_offset"] = address_offset


        self.used_addresses = range(self.map["address_offset"], self.map["address_offset"] + self.bank_size)


    def __getattr__(self, name):

        # handle internal attributes
        if name == "address_offset":
            if not self.is_sub_register:
                return self.map["address_offset"]
            else:
                print(f"{bcolors.WARNING}Register gen: Register '{self.name}' is a sub-register, its address_offset will always be zero, use starting_bit if you want the bit offset\nTraceback:{traceback.walk_stack()[-2]}{bcolors.ENDC}")
                return 0
        
        if name == "starting_bit":
            if not self.is_sub_register:
                print(f"{bcolors.WARNING}Register gen: Register '{self.name}' is not a sub-register, its starting_bit will always be zero\nTraceback: {traceback.format_stack()[-2]}{bcolors.ENDC}")
                return 0
            else:
                return self.map["starting_bit"]
        elif name == "width":
            return self.map["width"]
        elif name == "bank_size":
            return self.map["bank_size"]
        elif name == "description":
            return self.map["description"]

        if name in self.sub_registers:
            return self.sub_registers[name]
            
        raise AttributeError(f"Register gen: '{self.name}' map has no item '{name}', did you reference it from the correct containing object?\nTraceback: {traceback.format_stack()[-2]}")

    def __repr__(self) -> str:
        return f"Register(Name: {self.name}, rw: {self.rw}, type: {self.type}, width: {self.width}, address_offset: {self.start_address}, desc: {self.desc}, bank_size: {self.bank_size}, sub_registers: {self.sub_registers})"







class Group:
    def __init__(self, name: str, count:int=1, start_address: int=None, desc:str="", alignment:int=None):
        """
        Initialize a Group object.
        Args:
            name (str): The name of the group.
            count (int, optional): The number of instances of the group. Must be between 1 and 0xFFFF. Defaults to 1.
            start_address (int, optional): The starting address of the group. Must be between 0 and 0xFFFF.
            desc (str, optional): A description of the group.
            alignment (int, optional): The alignment of the group. Must be a power of 2. If not specified, the smallest alignment will be used automatically.
        Raises:
            ValueError: If any of the provided arguments are invalid
        """

        if start_address is not None and (start_address < 0 or start_address > 0xFFFF):
            raise ValueError("Invalid start_address")
        
        if alignment is not None and alignment & (alignment - 1) != 0:  # check if alignment is a power of 2
            raise ValueError("Invalid alignment")
        
        if count is not None and (count < 1 or count > 0xFFFF):
            raise ValueError("Invalid count")

        self.name = name
        self.start_address = start_address
        self.desc = desc
        self.alignment = alignment
        self.count = count

        self.contents = {}

        self.map = {}

        self.unassigned_items = []
        self.used_addresses = []

        self.generated = False


    def add(self, item: Union[Register, 'Group']):
        """
        Add a Register or Group to the Group.
        Args:
            item (Register or Group): The Register or Group to add.
        Raises:
            ValueError: If the provided item is invalid.
        """

        if self.generated:
            raise ValueError("Map is already generated, you may not add more items")

        if not isinstance(item, Register) and not isinstance(item, Group):
            raise ValueError("Invalid item")

        if item.name in self.contents:
            raise ValueError(f"Item name '{item.name}' already exists in the group '{self.name}'")
        
        self.contents[item.name] = item

    def get_address_offset(self):
        """
        Get the address offset of the group.
        Returns:
            int: The address offset of the group.
        """

        return self.map["address_offset"]
    
    def get_address_alignment(self):
        """
        Get the address alignment of the group.
        Returns:
            int: The address alignment of the group.
        """

        return self.alignment

    def generate(self):
        """
        INTERNAL\n
        Generate the register map for the group.
        """

        self.generated = False

        self.map = {}
        self.map["name"] = self.name
        self.map["address_offset"] = self.start_address
        self.map["description"] = self.desc
        self.map["alignment"] = self.alignment
        self.map["count"] = self.count
        self.map["groups"] = {}
        self.map["registers"] = {}

        # place items
        for name, item in self.contents.items():

            item.generate()

            if item.map["address_offset"] is None:    # skip items without a fixed base address for now, they will be auto-assigned later
                self.unassigned_items.append(item)
                continue

            if not self.__addresses_available(item.used_addresses):
                raise ValueError(f"Invalid item '{item}', addresses are already in use")
            
            if isinstance(item, Register):
                self.map["registers"][name] = item.map
                print(f"{bcolors.OKGREEN}Register gen: Register '{item.name}' has been placed at offset 0x{item.map['address_offset']:X} in group '{self.name}'{bcolors.ENDC}")
            else:
                if item.start_address % self.alignment != 0:
                    raise ValueError(f"Invalid group address '{item}', start_address must be aligned to the group's alignment ({item.alignment})")
                self.map["groups"][name] = item.map

            

            self.__use_addresses(item.used_addresses)


        # auto-assign base addresses for items
        success = False
        for item in self.unassigned_items:
            if isinstance(item, Register):
                alignment = 1
            else:
                alignment = item.alignment

            for starting_address in range(0, 0xFFFF - len(item.used_addresses) + 1, alignment):
                if self.__addresses_available(item.used_addresses, starting_address):
                    item.post_assign_address_offset(starting_address)
                    if isinstance(item, Register):
                        self.map["registers"][item.name] = item.map
                        print(f"{bcolors.OKGREEN}Register gen: Register '{item.name}' has been placed at offset 0x{item.map['address_offset']:X} in group '{self.name}'{bcolors.ENDC}")
                    else:
                        self.map["groups"][item.name] = item.map

                    self.__use_addresses(item.used_addresses)
                    
                    success = True
                    break

            if not success:
                raise ValueError(f"Unable to auto-assign base address for item: {item}")
            

        if self.alignment is not None and len(self.used_addresses) > self.alignment:
            raise ValueError(f"Group '{self}' alignment is too small to hold requested items ({self.alignment} < {len(self.used_addresses)})")
        
        if self.alignment is None:
            # find a power of 2 alignment that is large enough to hold the item
            self.alignment = 1

            while self.alignment < max(len(self.used_addresses), max(self.used_addresses)+1):                                                                                               
                self.alignment *= 2
            print(f"{bcolors.OKGREEN}Register gen: Group '{self.name}' alignment has been automatically set to 0x{self.alignment:X}{bcolors.ENDC}")
            self.map["alignment"] = self.alignment
            
        # groups use up their entire address space, regardless of what is inside
        if self.start_address is None:
            self.used_addresses = list(range(0, self.alignment*self.count))
        else:
            self.used_addresses = list(range(self.start_address, self.start_address + self.alignment*self.count))


        self.generated = True

    def __use_addresses(self, addresses):
        """
        INTERNAL
        Mark a range of addresses as used.
        Args:
            addresses (list): A list of addresses to mark as used.
        """

        self.used_addresses.extend(addresses)

    def __addresses_available(self, addresses, offset=0):
        """
        INTERNAL
        Check if all addresses in a list are available.
        Args:
            addresses (list): A list of addresses to check.
        Returns:
            bool: True if the range is available, False otherwise.
        """

        for address in addresses:
            if address+offset in self.used_addresses:
                return False
        
        return True
    
    def post_assign_address_offset(self, address_offset):

        self.map["address_offset"] = address_offset
        for i in range(len(self.used_addresses)):
            self.used_addresses[i] = i + address_offset

    def __getattr__(self, name):

        # handle internal attributes
        if name == "offset":
            return self.map["address_offset"]
        elif name == "alignment":
            return self.map["alignment"]
        elif name == "count":
            return self.map["count"]
        elif name == "description":
            return self.map["description"]

        if name in self.contents:
            return self.contents[name]
            
        raise AttributeError(f"Register gen: '{self.name}' map has no item '{name}', did you reference it from the correct containing object?\nTraceback: {traceback.format_stack()[-2]}")
    
    def __repr__(self) -> str:
        return f"Group(Name: {self.name}, count: {self.count}, address_offset: {self.start_address}, desc: {self.desc}, alignment: {self.alignment}, contents: {self.contents})"





class RegisterMapGenerator:
    def __init__(self, name: str, compatible_drivers: list, driver_settings: dict={}, desc: str=""):
        """
        Handle all registers and information about a module
        Args:
            name (str): The name of the module.
            compatible_drivers (list): A list of compatible drivers.
            driver_settings (dict): A dictionary of driver settings.
            desc (str, optional): A description of the module.
        """
        self.name = name
        self.compatible_drivers = compatible_drivers
        self.driver_settings = driver_settings
        self.desc = desc

        self.base_group = Group("base_group", 1, 0, "Base group for all registers", 0x10000)

        self.generated = False

        self.map = {}

    def add(self, item: Union[Register, Group]):
        """
        Add a Register or Group to the base group.
        Args:
            item (Register or Group): The Register or Group to add.
        Raises:
            ValueError: If the provided item is invalid.
        """

        if self.generated:
            raise ValueError("Map is already generated, you may not add more items")

        self.base_group.add(item)


    def generate(self):
        """
        Generate the register map
        the module may not be modified after this is called
        """

        

        if self.generated:
            raise ValueError("Map is already generated")
        
        self.map["name"] = self.name
        print(f"{bcolors.OKBLUE}Register gen: Creating register map for module '{self.name}'{bcolors.ENDC}")
        
        if self.compatible_drivers != {}:
            print(f"{bcolors.OKGREEN}Register gen: Compatible drivers set to {self.compatible_drivers}{bcolors.ENDC}")
        else:
            print(f"{bcolors.WARNING}Register gen: No compatible drivers set, controller will not be able to automatically use this module!{bcolors.ENDC}")
        self.map["compatible_drivers"] = self.compatible_drivers

        for setting, value in self.driver_settings.items():
            print(f"{bcolors.OKGREEN}Register gen: Driver setting '{setting}' = {value} added to '{self.name}'{bcolors.ENDC}")
        self.map["driver_settings"] = self.driver_settings

        self.base_group.generate()

        self.map["base_group"] = self.base_group.map

        self.generated = True

        print(f"{bcolors.OKBLUE}Register gen: Done creating register map for module '{self.name}'{bcolors.ENDC}")

    def exportJSON(self, filename):
        """
        Export the register map to a JSON file.
        Args:
            filename (str): The name of the file to export to.
        """

        if not self.generated:
            raise ValueError("Map must be generated before exporting")

        import json

        with open(filename, 'w') as f:
            json.dump(self.map, f, indent=4)

    def export(self) -> dict:
        """
        Export the register map as a dictionary.
        Returns:
            dict: The register map.
        """

        if not self.generated:
            raise ValueError("Map must be generated before exporting")

        return self.map

    def __getattr__(self, name):
        if name in self.base_group.contents:
            return self.base_group.contents[name]
            
        raise AttributeError(f"Register gen: '{self.name}' map has no item '{name}', did you reference it from the correct containing object?")



if __name__ == "__main__":

    rm = RegisterMapGenerator("module", ["driver1", "driver2"], {"setting1": 1, "setting2": 2}, "Module description")


    sub_regs = [
        Register("sub1", type="unsigned", width=4),
        Register("sub2", type="unsigned", width=4, start_address=6)
    ]
    r = Register("name1", "r", "unsigned", 8, 0x0, "description", sub_registers=sub_regs)
    rm.add(r)

    rm.add(Register("name2", "r", "unsigned"))
    rm.add(Register("name3", "r", "unsigned"))
    rm.add(Register("name4", "r", "unsigned", bank_size=4))
    rm.add(Register("name5", "r", "unsigned"))

    g = Group("group1", 4)
    g.add(Register("name6", "r", "unsigned"))
    g.add(Register("name7", "r", "unsigned", start_address=0x4))
    g.add(Register("name8", "r", "unsigned", bank_size=2))
    rm.add(g)



    rm.generate()
    #print(rm.map)
    rm.exportJSON("test.json")

    #print(g.get_address_offset())

    print(rm.name1.sub1.width)

    #r.generate()

    #print(r.map)

    g = Group("name", 4)

