from enum import IntEnum, auto

# tools for creating register maps

# types of registers
class RegisterDataType(IntEnum):
    BOOL = 0    # 1 bit
    SIGNED = auto()    # 1-32 bits
    UNSIGNED = auto()   # 1-32 bits
    #FLOAT = auto()  # 32 bit       # not supported yet
    PACKED = auto() # for containing multiple fields

# read/write permissions
class ReadWritePermissions(IntEnum):
    READ = 0
    WRITE = auto()
    READ_WRITE = auto()

MAX_ADDRESS = 0xFFFF

class Register:
    """
    Class for RTL register.
    """
    def __init__(self, name: str, dataType: RegisterDataType, readWrite: ReadWritePermissions, description: str, unit: str = "", width: int = 32, forcedAddress: int = None, allignedTo: int = None):
        self.name = name
        self.dataType = dataType
        self.readWrite = readWrite
        self.description = description
        self.unit = unit
        self.width = width
        self.count = 1

        self.group_name = ""
        self.group_index = 0

        if self.dataType == RegisterDataType.BOOL:
            self.width = 1

        if self.dataType == RegisterDataType.PACKED:
            self.width = 0
        
        self.forcedAddress = forcedAddress
        self.allignedTo = allignedTo

        self.address = None
        self.metadata = {}
        self.startingBit = None

        self.packedRegisters = []


    def pack(self, reg, forcedOffset: int = None):
        """
        pack smaller register into a larger register
        """
        if self.dataType != RegisterDataType.PACKED:
            raise ValueError("Only packed registers can be packed into")
        
        if reg.dataType == RegisterDataType.PACKED:
            raise ValueError("Cannot pack packed register into another register")
        
        if reg.width + self.width > 32:
            raise ValueError(f"Not enough space to pack register ({reg.width} + {self.width} > 32)")\
        
        reg.startingBit = self.width
        self.packedRegisters.append(reg)
        self.width += reg.width

    def getAddress(self):
        if self.address is None:
            raise ValueError("Address not set, registers must be built into a RTL block before getting address info")
        return self.address
    
    def getStartAddress(self):
        return self.getAddress()
    
    def getEndAddress(self):
        return self.getAddress() + self.count - 1

    def getBits(self, packedRegName:str = None):
        """
        return the starting and ending bit of the value
        """
        if self.startingBit is None:
            raise ValueError("Bits not set, registers must be built into a block before getting address info")
        
        if packedRegName is None:
            return self.startingBit, self.startingBit + self.width - 1
        
        if self.dataType != RegisterDataType.PACKED:
            raise ValueError("not a packed register")
        
        if packedRegName not in [reg.name for reg in self.packedRegisters]:
            raise ValueError(f"Register {packedRegName} not found in packed register {self.name}")
        
        reg = [reg for reg in self.packedRegisters if reg.name == packedRegName][0]
        
        return reg.startingBit, reg.startingBit + reg.width - 1



    def setMetadata(self, key, value):
        self.metadata[key] = value

    def getData(self):
        """
        get data for register
        """
        data = {
            "address": self.address,
            "name": self.name,
            "count": self.count,
            "group_name": self.group_name,
            "group_index": self.group_index,
            "dataType": self.dataType.name,
            "readWrite": self.readWrite.name,
            "description": self.description,
            "unit": self.unit,
            "startingBit": self.startingBit,
            "width": self.width,
            "metadata": self.metadata
        }

        if self.dataType == RegisterDataType.PACKED:
            data["packedRegisters"] = [reg.getData() for reg in self.packedRegisters]
        
        return data


class RTL_Block:
    """
    Main class for RTL block. Contains name, docs, and list of registers.
    """
    def __init__(self, name: str):
        self.name = name
        self.registers = {}
        self.registerGroups = {}
        self.groupCounts = {}
        
        self.generated = False
        self.addressMap = {}

        self.compatible_drivers = []
        self.driverData = {}


        self.usedAddresses = []

    def addDriverData(self, name:str, data):
        """
        This data will be directly added to the driver file as a #define \n
        Usefull for adding configuration specifiers
        """
        if name in self.driverData:
            raise ValueError(f"Driver data {name} already exists in block {self.name}")
        self.driverData[name] = data

    def addRegister(self, reg: Register):
        if reg.name in self.registers:
            raise ValueError(f"Register {reg.name} already exists in block {self.name}")
        
        self.registers[reg.name] = reg

    def createRegisterGroup(self, groupName: str, forcedBaseAddress: int = None, allignedTo: int = None):  # TODO: add a way to align to a specific address increments (makes addressing each group easier)
        if groupName in self.registerGroups:
            raise ValueError(f"Register group {groupName} already exists in block {self.name}")
        
        self.registerGroups[groupName] = {}
        self.registerGroups[groupName]["registers"] = {}
        self.registerGroups[groupName]["register_count"] = 0
        self.registerGroups[groupName]["base_address"] = None
        self.registerGroups[groupName]["alligned_to"] = 0
        
        if allignedTo is not None:
            if (allignedTo & (allignedTo - 1)) != 0:    # not a power of 2, this is still allowed but normally is not useful so a warning is given
                print(f"WARNING: Allignment size {allignedTo} is not a power of 2, this is usually not expected")

            self.registerGroups[groupName]["alligned_to"] = allignedTo

        if forcedBaseAddress is not None:
            self.registerGroups[groupName]["base_address"] = forcedBaseAddress

            if allignedTo is not None:
                if forcedBaseAddress % allignedTo != 0:
                    raise ValueError("Base address must be alligned to the allignment size")
            
        self.groupCounts[groupName] = 0

    def addRegisterToGroup(self, groupName: str, reg):
        if groupName not in self.registerGroups:
            raise ValueError(f"Register group {groupName} does not exist in block {self.name}")
        
        if reg.name in self.registers:
            raise ValueError(f"Register {reg.name} already exists in individual registers {self.name}")
        
        if reg.name in self.registerGroups[groupName]["registers"]:
            raise ValueError(f"Register {reg.name} already exists in group {groupName}")
        
        reg.group = groupName
        self.registerGroups[groupName]["registers"][reg.name] = reg
        self.registerGroups[groupName]["register_count"] += 1

    def setGroupCount(self, groupName: str, count: int):
        """how many times the group should be repeated"""
        if groupName not in self.registerGroups:
            raise ValueError(f"Register group {groupName} does not exist in {self.name}")
        
        self.groupCounts[groupName] = count

    def packInto(self, packRegName, singleReg, groupName: str=None):
        """
        pack a single register into a packed register
        """

        

        if groupName is not None:
            if groupName not in self.registerGroups:
                raise ValueError(f"Register group {groupName} does not exist in {self.name}")
            
            if packRegName not in self.registerGroups[groupName]["registers"]:
                raise ValueError(f"Register {packRegName} not found in group {groupName}")
            
            packReg = self.registerGroups[groupName]["registers"][packRegName]
        else:
            if packRegName not in self.registers:
                raise ValueError(f"Register {packRegName} not found in {self.name}")
            
            packReg = self.registers[packRegName]
        

        if packReg.dataType != RegisterDataType.PACKED:
            raise ValueError("Only packed registers can be packed into")
        
        if singleReg.dataType == RegisterDataType.PACKED:
            raise ValueError("Cannot pack packed register into another register")
        
        if singleReg.width + packReg.width > 32:
            raise ValueError(f"Not enough space to pack register ({singleReg.width} + {packReg.width} > 32)")\
        
        packReg.pack(singleReg)
        self.registers[packRegName] = packReg

    def addRegisterBank(self, reg: Register, count: int):
        """
        add multiple registers with same name, they will fill contiguous addresses
        """
        if reg.name in self.registers:
            raise ValueError(f"Register {reg.name} already exists in block {self.name}")
        
        if reg.dataType == RegisterDataType.PACKED:
            raise ValueError("packed register banks are not supported")
        
        reg.count = count
        self.registers[reg.name] = reg

    def generateAddressMap_groupRegs(self, groupName, group, i):
        groupSize = max(group["register_count"], group["alligned_to"])

        # add registers with specified addresses
        for reg in group["registers"].values():
            reg.startingBit = 0
            reg.group_name = groupName
            reg.group_index = i

            if reg.forcedAddress is None:
                continue
            if reg.allignedTo is not None:
                raise ValueError(f"Cannot force address and allign to a specific address increment at the same time for group: {groupName}, register: {reg.name}")
            
            # in grouped registers, a register's forced address will be its offset from its group's base address, not its absolute address
            reg.address = reg.forcedAddress + group["base_address"] + i * groupSize

            for n in range(reg.count):
                if reg.address + n in self.usedAddresses:
                    raise ValueError(f"Address {reg.address + n} already exists, unable to force address assignment for register: {reg.name}")
                self.usedAddresses.append(reg.address + n)

            # update addresses for packed registers
            if reg.dataType == RegisterDataType.PACKED:
                for packedReg in reg.packedRegisters:
                    packedReg.address = reg.address
        
            self.addressMap[f"0x{reg.address:X}"] = reg.getData()

        # add registers with alligned addresses
        for reg in group["registers"].values():
            reg.startingBit = 0
            reg.group_name = groupName
            reg.group_index = i

            if reg.forcedAddress is not None:   # skip if address is forced, it has already been added
                continue
            if reg.allignedTo is None:  # skip if allignment is not set
                continue
        
            # find an alligned region of size reg.count that is not used
            reg.address = None
            for n in range(groupSize - reg.count + 1, step = reg.allignedTo):
                n = n + group["base_address"] + i * groupSize
                if all((n + j) not in self.usedAddresses for j in range(reg.count)):
                    reg.address = n
                    break
            if reg.address is None:
                raise ValueError(f"No available address for register: {reg.name}")
            

            for n in range(reg.count):
                if reg.address + n in self.usedAddresses:
                    raise ValueError(f"Address {reg.address + n} already exists, unable to force address assignment for register: {reg.name}")
                self.usedAddresses.append(reg.address + n)

            # update addresses for packed registers
            if reg.dataType == RegisterDataType.PACKED:
                for packedReg in reg.packedRegisters:
                    packedReg.address = reg.address
            self.addressMap[f"0x{reg.address:X}"] = reg.getData()

        # add registers with unspecified addresses
        for reg in group["registers"].values():
            reg.startingBit = 0
            reg.group_name = groupName
            reg.group_index = i

            if reg.forcedAddress is not None:   # skip if address is forced, it has already been added
                continue
            if reg.allignedTo is not None:  # skip if allignment is set, it has already been added
                continue
            
            # find a region of size reg.count that is not used
            reg.address = None
            for n in range(groupSize - reg.count + 1):
                n = n + group["base_address"] + i * groupSize
                if all((n + j) not in self.usedAddresses for j in range(reg.count)):
                    reg.address = n
                    break
            if reg.address is None:
                raise ValueError(f"No available address for register: {reg.name}")
            
            for n in range(reg.count):
                if reg.address + n in self.usedAddresses:
                    raise ValueError(f"Address {reg.address + n} already exists, unable to force address assignment for register: {reg.name}")
                self.usedAddresses.append(reg.address + n)

            # update addresses for packed registers
            if reg.dataType == RegisterDataType.PACKED:
                for packedReg in reg.packedRegisters:
                    packedReg.address = reg.address
            self.addressMap[f"0x{reg.address:X}"] = reg.getData()

        for n in range(groupSize):   # use up all registers in the block, even if they are not used. this prevents other unspecifed registers from being assigned to the same block
            n = n + group["base_address"] + i * groupSize
            if n in self.usedAddresses:
                continue
            self.usedAddresses.append(n)

    def generateAddressMap(self):
        """
        generate address map for all registers in block, registers cannot be modifgied after this
        """
        address = 0
        self.addressMap = {}
        

        # add forced address registers first
        # add individual registers
        for reg in self.registers.values():
            reg.startingBit = 0
            if reg.forcedAddress is None:
                continue
            if reg.allignedTo is not None:
                raise ValueError("Cannot force address and allign to a specific address increment at the same time")
            
            reg.address = reg.forcedAddress
            
            for i in range(reg.count):
                if reg.address + i in self.usedAddresses:
                    raise ValueError(f"Address {reg.address + i} already exists in {self.name}, unable to force address assignment for register: {reg.name}")
                self.usedAddresses.append(reg.address + i)

            # update addresses for packed registers
            if reg.dataType == RegisterDataType.PACKED:
                for packedReg in reg.packedRegisters:
                    packedReg.address = reg.address
            self.addressMap[f"0x{reg.address:X}"] = reg.getData()

        # add register groups
        for groupName, group in self.registerGroups.items():

            if group["base_address"] is None:   # skip if base address is not forced
                continue

            if group["alligned_to"] != 0:
                if group["base_address"] is not None and group["base_address"] % group["alligned_to"] != 0:
                    raise ValueError("Base address must be alligned to the allignment size")
                if group["register_count"] > group["alligned_to"]:
                    raise ValueError(f"Register group size must be less than or equal to the allignment size for group: {groupName}")
                
            for i in range(self.groupCounts[groupName]):
                self.generateAddressMap_groupRegs(groupName, group, i)


        # add alligned registers
        # add individual registers
        for reg in self.registers.values():
            reg.startingBit = 0
            if reg.forcedAddress is not None:   # skip if address is forced, it has already been added
                continue
            if reg.allignedTo is None:  # skip if allignment is not set
                continue
            
            # find an alligned region of size reg.count that is not used
            reg.address = None
            for n in range(MAX_ADDRESS - reg.count + 1, step = reg.allignedTo):
                if all((n + j) not in self.usedAddresses for j in range(reg.count)):
                    reg.address = n
                    break
            if reg.address is None:
                raise ValueError(f"No available address for register: {reg.name}")
            

            for i in range(reg.count):
                if reg.address + i in self.usedAddresses:
                    raise ValueError(f"Address {reg.address + i} already exists, unable to force address assignment for register: {reg.name}")
                self.usedAddresses.append(reg.address + i)

            # update addresses for packed registers
            if reg.dataType == RegisterDataType.PACKED:
                for packedReg in reg.packedRegisters:
                    packedReg.address = reg.address
            self.addressMap[f"0x{reg.address:X}"] = reg.getData()

        # add register groups
        for groupName, group in self.registerGroups.items():

            if group["base_address"] is not None:   # skip if base address is forced, it has already been added
                continue

            if group["alligned_to"] == 0:    # skip if allignment is not set
                continue

            if group["register_count"] > group["alligned_to"]:
                raise ValueError(f"Register group size must be less than or equal to the allignment size for group: {groupName}")
            
            
            # find a region for groups that is not in usedAddresses
            totalSize = group["alligned_to"] * self.groupCounts[groupName]
            group["base_address"] = None
            for i in range(MAX_ADDRESS - totalSize + 1, step = group["alligned_to"]):
                if all((i + j) not in self.usedAddresses for j in range(totalSize)):
                    group["base_address"] = i
                    break
            if group["base_address"] is None:
                raise ValueError(f"No available base address for group: {groupName}")
            

            for i in range(self.groupCounts[groupName]):
                self.generateAddressMap_groupRegs(groupName, group, i)


        # add unspecifed registers (automatically find addresses for them)
        # add individual registers
        for reg in self.registers.values():
            reg.startingBit = 0
            if reg.forcedAddress is not None:   # skip if address is forced, it has already been added
                continue
            if reg.allignedTo is not None:  # skip if allignment is set, it has already been added
                continue
            
            # find a region of size reg.count that is not used
            reg.address = None
            for i in range(MAX_ADDRESS - reg.count + 1):
                if all((i + j) not in self.usedAddresses for j in range(reg.count)):
                    reg.address = i
                    break
            if reg.address is None:
                raise ValueError(f"No available address for register: {reg.name}")
            

            for i in range(reg.count):
                if reg.address + i in self.usedAddresses:
                    raise ValueError(f"Address {reg.address + i} already exists, unable to force address assignment for register: {reg.name}")
                self.usedAddresses.append(reg.address + i)

            # update addresses for packed registers
            if reg.dataType == RegisterDataType.PACKED:
                for packedReg in reg.packedRegisters:
                    packedReg.address = reg.address
            self.addressMap[f"0x{reg.address:X}"] = reg.getData()

        # add register groups
        for groupName, group in self.registerGroups.items():

            if group["base_address"] is not None:   # skip if base address is forced, it has already been added
                continue

            if group["alligned_to"] != 0:    # skip if allignment is set, it has already been added
                continue
            
            
            # find a region for groups that is not used
            totalSize = group["register_count"] * self.groupCounts[groupName]
            group["base_address"] = None
            for i in range(MAX_ADDRESS - totalSize + 1):
                if all((i + j) not in self.usedAddresses for j in range(totalSize)):
                    group["base_address"] = i
                    break
            if group["base_address"] is None:
                raise ValueError(f"No available base address for group: {groupName}")

            for i in range(self.groupCounts[groupName]):
                self.generateAddressMap_groupRegs(groupName, group, i)


        print(f"Generated address map for {self.name} with {address} registers (32 bit)")

        self.generated = True

    def getRegistor(self, regName:str) -> Register:
        """
        get register
        """
        if not self.generated:
            raise ValueError("Address map not generated yet")
        
        if regName not in self.registers:
            raise ValueError(f"Register {regName} does not exist in {self.name}")
        
        return self.registers[regName]
    
    def getGroupAddress(self, groupName:str, index:int = 0):

        if groupName not in self.registerGroups:
            raise ValueError(f"Register group {groupName} does not exist in {self.name}")
        
        if not self.generated:
            raise ValueError("Address map not generated yet")
        
        if abs(index) >= self.groupCounts[groupName]:
            raise ValueError(f"index {index} is out of range for group {groupName} with {self.groupCounts[groupName]} count")
        
        offset = max(self.registerGroups[groupName]["register_count"], self.registerGroups[groupName]["alligned_to"])

        if index >= 0:
            return self.registerGroups[groupName]["base_address"] + offset * index
        
        return self.registerGroups[groupName]["base_address"] + offset * (self.groupCounts[groupName] + index)

    def getGroupRegisterCount(self, groupName:str):
        """Get the number of registers in a single group element, usefull for automatic address offsetting"""
        if groupName not in self.registerGroups:
            raise ValueError(f"Register group {groupName} does not exist in {self.name}")
        
        if not self.generated:
            raise ValueError("Address map not generated yet")
        
        return max(self.registerGroups[groupName]["register_count"], self.registerGroups[groupName]["alligned_to"])
    
    def getGroupRegisterOffset(self, groupName:str, registerName:str):
        """Get the offset of a specific register from the group base address"""

        if groupName not in self.registerGroups:
            raise ValueError(f"Register group {groupName} does not exist in {self.name}")
        
        if not self.generated:
            raise ValueError("Address map not generated yet")
        
        if registerName not in self.registerGroups[groupName]["registers"]:
            raise ValueError(f"Register {registerName} not found in {groupName} group")
        
        return self.registerGroups[groupName]["registers"][registerName].address - self.getGroupAddress(groupName, -1)

    def printAddressMap(self):
        """
        print address map for all registers in block
        """
        if not self.generated:
            raise ValueError("Address map not generated yet")
        
        print(self.addressMap)

    def exportDataJSON(self, filename):
        """
        export register data to JSON file
        """
        import json

        with open(filename, "w") as file:
            json.dump(self.getData(), file, indent=4)

    def getData(self):
        """
        get all data for the block
        """
        export_data = {
            "name": self.name,
            "compatible_drivers": self.compatible_drivers,
            "driver_data": self.driverData,
            "address_map": self.addressMap,
        }
        return export_data

    def addCompatibleDriver(self, driver:str):
        """
        add compatible c++ driver for block \n
        more than one driver can be \n
        if none are assigned then it will be given a generic driver
        """
        self.compatible_drivers.append(driver)

    def getCompatibleDrivers(self):
        """
        get compatible drivers for the block
        """
        if len(self.compatible_drivers) == 0:
            return ["generic"]
        
        return self.compatible_drivers
    
    def getAddressMap(self):
        """
        get address map for the block
        """
        return self.addressMap


if __name__ == "__main__":
    # create block
    block = RTL_Block("test_block")


    # single register
    block.addRegister(Register("bitrate", RegisterDataType.UNSIGNED, ReadWritePermissions.READ_WRITE, "bitrate setting", unit="bps"))
    
    # packed register
    block.addRegister(Register("control", RegisterDataType.PACKED, ReadWritePermissions.READ_WRITE, "control register"))
    block.packInto("control", Register("start", RegisterDataType.BOOL, ReadWritePermissions.WRITE, "start"))
    block.packInto("control", Register("stop", RegisterDataType.BOOL, ReadWritePermissions.WRITE, "stop"))
    block.packInto("control", Register("busy", RegisterDataType.BOOL, ReadWritePermissions.READ, "busy flag"))
    block.packInto("control", Register("setting", RegisterDataType.UNSIGNED, ReadWritePermissions.READ_WRITE, "setting", width=4))

    # register group
    block.createRegisterGroup("data", forcedBaseAddress=0x0, allignedTo=0x10)   # TODO: add ways to use this object when making the accompanying switch statements
    block.addRegisterToGroup("data", Register("data0", RegisterDataType.SIGNED, ReadWritePermissions.READ, "data 0"))
    block.addRegisterToGroup("data", Register("data1", RegisterDataType.SIGNED, ReadWritePermissions.READ, "data 1"))
    block.addRegisterToGroup("data", Register("data2", RegisterDataType.SIGNED, ReadWritePermissions.READ, "data 2"))
    block.setGroupCount("data", 3)

    # register bank
    block.addRegisterBank(Register("capture_mem", RegisterDataType.UNSIGNED, ReadWritePermissions.READ, "status register"), 64)

    # generate address map
    block.generateAddressMap()

    # save map to JSON
    block.exportDataJSON("test_block.json")


    # get address of a single register
    print("bitrate address: ", block.getRegistor("bitrate").address)
    print("control address: ", block.getRegistor("control").address)

    # bit positions of a single register
    print("bitrate start and end bits: ", block.getRegistor("bitrate").getBits())

    # get bit positions of data in a packed register
    print("control start and end bits: ", block.getRegistor("control").getBits())   # full register
    print("control:start start and end bits: ", block.getRegistor("control").getBits("start"))   # start bit
    print("control:stop start and end bits: ", block.getRegistor("control").getBits("stop"))   # stop bits
    print("control:busy start and end bits: ", block.getRegistor("control").getBits("busy"))   # busy bit
    print("control:setting start and end bits: ", block.getRegistor("control").getBits("setting"))   # setting bits

    # get group element base address
    print("data group 0 base address: ", block.getGroupAddress("data"))
    print("data group 0 base address: ", block.getGroupAddress("data", 0))
    print("data group 1 base address: ", block.getGroupAddress("data", 1))
    print("data group 2 base address: ", block.getGroupAddress("data", 2))

    # get group register count (usefull for automatic offsetting)
    print("data group register count: ", block.getGroupRegisterCount("data"))

    # get individual offsets for group registers from the base address
    print("data0 address offset: ", block.getGroupRegisterOffset("data", "data0"))
    print("data1 address offset: ", block.getGroupRegisterOffset("data", "data1"))
    print("data2 address offset: ", block.getGroupRegisterOffset("data", "data2"))

    # get bank start and end addresses
    print("capture_mem addresses: ", block.getRegistor("capture_mem").getStartAddress(), block.getRegistor("capture_mem").getEndAddress())
