

# load in the json file
import json
import os
def load_json_file(file_path):
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File {file_path} does not exist.")
    with open(file_path, 'r') as file:
        return json.load(file)
    
# save the json file
def save_json_file(file_path, data):
    with open(file_path, 'w') as file:
        json.dump(data, file, indent=4)

def camel_to_snake(input: str) -> str:
    out = ""
    for i, c in enumerate(input):
        if c.isupper() and i > 0:
            out += "_" + c.lower()
        else:
            out += c.lower()
    return out

def convert_io_type(ioType: str) -> str:
    """
    Convert the input/output type from the input format to the output format.
    """
    if ioType == "U8":
        return "uint8"
    elif ioType == "U16":
        return "uint16"
    elif ioType == "U32":
        return "uint32"
    elif ioType == "I8":
        return "int8"
    elif ioType == "I16":
        return "int16"
    elif ioType == "I32":
        return "int32"
    elif ioType == "F32":
        return "float"
    elif ioType == "F64":
        return "double"
    elif ioType == "Bool":
        return "bool"
    else:
        raise ValueError(f"Unknown IO type: {ioType}")

def convert_node(nodeType: str, data: dict, id: str) -> dict:
    """
    Convert a node from the input format to the output format.
    """
    outNode = {}
    pinData = {
        "in": {},
        "out": {}
    }
    key = ""
    
    if nodeType == "Constant":
        outNode["type"] = "constant"
        outNode["config"] = {
            "value": data["value"],
            "type": convert_io_type(data["itype"])
        }
        key = data['node_name']
        pinData["out"]["0"] = "output"

    elif nodeType == "ApiInput":
        outNode["type"] = "api_input"
        outNode["config"] = {
            "type": convert_io_type(data["itype"])
            }
        if "min" in data:
            outNode["config"]["min"] = data["min"]
        if "max" in data:
            outNode["config"]["max"] = data["max"]
        if "default" in data:
            outNode["config"]["default"] = data["default"]
        if "timeout" in data:
            outNode["config"]["timeout"] = data["timeout"]
        
        key = data['node_name']
        pinData["out"]["0"] = "output"

    elif nodeType == "ApiOutput":
        outNode["type"] = "api_output"
        outNode["config"] = {
            "type": convert_io_type(data["otype"])
        }
        
        key = data['node_name']
        pinData["in"]["0"] = "input"

    elif nodeType == "PIController":
        outNode["type"] = "pi_controller"
        outNode["config"] = {
            "Kp" : data["p"],
            "Ki" : data["i"],
            "i_limit" : data["i_limit"],
            "output_min" : data["output_min"],
            "output_max" : data["output_max"],
        }

        key = data['node_name']
        pinData["in"]["0"] = "input"
        pinData["out"]["0"] = "output"
    
    elif nodeType == "VelEstimator":
        outNode["type"] = "vel_estimator"
        outNode["config"] = {
            "alpha": data["filter_alpha"]
        }

        key = f"vel_estimator_{id}"
        pinData["in"]["0"] = "input"
        pinData["out"]["0"] = "output"

    elif nodeType == "LogicGate":
        outNode["type"] = "logic_gate"
        # get the first item in the “gType” dict
        gateType, inCount = next(iter(data["gType"]))
        gateType = gateType.lower()
        inCount = int(inCount)
        outNode["config"] = {
            "gate_type": gateType,
            "input_count": inCount
        }
        key = f"logic_gate_{id}"
        for i in range(inCount):
            pinData["in"][str(i)] = f"input_{i}"
        pinData["out"]["0"] = "output"

    elif nodeType == "Comparator":
        outNode["type"] = "comparator"
        compType = camel_to_snake(data["comparison"])
        outNode["config"] = {
            "type": convert_io_type(data["itype"]),
            "comparator_type": compType,
        }
        key = f"comparator_{id}"
        pinData["in"]["0"] = "input_0"
        pinData["in"]["1"] = "input_1"
        pinData["out"]["0"] = "output"
    
    elif nodeType == "MathOperation":
        outNode["type"] = "math_operation"
        outNode["config"] = {
            "type": convert_io_type(data["itype"]),
        }
        key = f"math_operation_{id}"
        if "Nary" in data["operator"]:
            outNode["config"]["op_type"] = "nary"
            inCount = int(data["operator"]["Nary"][1])
            op = data["operator"]["Nary"][0].lower()
            outNode["config"]["op"] = op
            outNode["config"]["input_count"] = inCount
            for i in range(inCount):
                pinData["in"][str(i)] = f"input_{i}"
        elif "BinaryOperation" in data["operator"]:
            outNode["config"]["op_type"] = "binary"
            op = data["operator"]["BinaryOperation"].lower()
            outNode["config"]["op"] = op
            pinData["in"]["0"] = "input_0"
            pinData["in"]["1"] = "input_1"
        elif "UnaryOperation" in data["operator"]:
            outNode["config"]["op_type"] = "unary"
            op = data["operator"]["UnaryOperation"].lower()
            outNode["config"]["op"] = op
            pinData["in"]["0"] = "input_0"
        
        pinData["out"]["0"] = "output"

    elif nodeType == "Multiplexer":
        outNode["type"] = "multiplexer"
        bits = int(data["input_bits"])
        outNode["config"] = {
            "input_bits": bits,
            "type": convert_io_type(data["itype"]),
        }
        key = f"multiplexer_{id}"
        for i in range(bits):
            pinData["in"][str(i)] = f"select_bit_{i}"
        for i in range(2**bits):
            pinData["in"][str(i + bits)] = f"input_{i}"
        pinData["out"]["0"] = "output"

    elif nodeType == "BitwiseSplit":
        outNode["type"] = "bitwise_split"
        bits = 0
        if data["num_bits"] <= 8:
            itype = "U8"
            bits = 8
        elif data["num_bits"] <= 16:
            itype = "U16"
            bits = 16
        elif data["num_bits"] <= 32:
            itype = "U32"
            bits = 32
        else:
            raise ValueError(f"Unknown bitwise split data type: {data['itype']}")
        
        outNode["config"] = {
            "type": convert_io_type(itype),
        }
        key = f"bitwise_split_{id}"
        pinData["in"]["0"] = "input"
        for i in range(bits):
            pinData["out"][str(i)] = f"output_bit_{i}"

    elif nodeType == "BitwiseJoin":
        outNode["type"] = "bitwise_join"
        bits = 0
        if data["num_bits"] <= 8:
            itype = "U8"
            bits = 8
        elif data["num_bits"] <= 16:
            itype = "U16"
            bits = 16
        elif data["num_bits"] <= 32:
            itype = "U32"
            bits = 32
        else:
            raise ValueError(f"Unknown bitwise join data type: {data['itype']}")
        
        outNode["config"] = {
            "type": convert_io_type(itype),
        }
        key = f"bitwise_join_{id}"
        pinData["out"]["0"] = "output"
        for i in range(bits):
            pinData["in"][str(i)] = f"input_bit_{i}"

    elif nodeType == "EdgeDelay":
        outNode["type"] = "edge_delay"
        outNode["config"] = {
            "rising_edge": data["rising_edge"],
            "falling_edge": data["falling_edge"],
            "cycles": data["delay_cycles"]
        }
        key = f"edge_delay_{id}"
        pinData["in"]["0"] = "input"
        pinData["out"]["0"] = "output"

    elif nodeType == "CycleDelay":
        outNode["type"] = "cycle_delay"
        outNode["config"] = {
            "cycles": data["cycles"],
            "type": convert_io_type(data["itype"])
        }
        key = f"cycle_delay_{id}"
        pinData["in"]["0"] = "input"
        pinData["out"]["0"] = "output"

    elif nodeType == "Converter":
        outNode["type"] = "converter"
        outNode["config"] = {
            "input_type": convert_io_type(data["input_type"]),
            "output_type": convert_io_type(data["output_type"]),
            "direct_mode": data["direct_mode"],
            "input_min": data["input_min"],
            "input_max": data["input_max"],
            "output_min": data["output_min"],
            "output_max": data["output_max"],
            "invert": data["invert"]
        }
        key = f"converter_{id}"
        pinData["in"]["0"] = "input"
        pinData["out"]["0"] = "output"

    elif nodeType == "SerialDevice":
        outNode["type"] = "em_serial_device"
        pinData["in"]["0"] = "em_device"
        outNode["config"] = {
            "enabled": data["enabled"],
            "comm": {
                "device_address": data["addr"],
                "timeout_tries": data["timeout"],
            },
            "device": {
                "device_descriptor": data["descriptor"],
                "cyclic_write_regs": [],
                "cyclic_read_regs": [],
                "async_write_regs": [],
                "async_read_regs": [],
                "set": []
            }
        }
        key = data['node_name']

        # pin data is unknown for raw serial devices
        # it depends on the connected serial read/write nodes

    elif nodeType == "SerialRead":
        if "AsyncReg" in data["dev"]:
            outNode["async_read_regs"] = {
                "name": data["name"],
                "update_rate_cycles": data["dev"]["AsyncReg"]["update_cycles"]
            }
        else:
            outNode["cyclic_read_regs"] = {
                "name": data["name"],
                "cyclic_index": data["dev"]["CyclicReg"]["cyclic_index"],
                "sync_with_node": data["dev"]["CyclicReg"]["sync_node"]
            }

    elif nodeType == "SerialWrite":
        if "AsyncReg" in data["dev"]:
            outNode["async_write_regs"] = {
                "name": data["name"],
                "update_rate_cycles": data["dev"]["AsyncReg"]["update_cycles"]
            }
        else:
            outNode["cyclic_write_regs"] = {
                "name": data["name"],
                "cyclic_index": data["dev"]["CyclicReg"]["cyclic_index"],
                "sync_with_node": data["dev"]["CyclicReg"]["sync_node"]
            }

    elif nodeType == "GetGlobalVariable":
        outNode["type"] = "get_global_variable"
        pinData["out"]["0"] = "output"
        outNode["config"] = {
            "variables": {
                "output": data['name']
            }
        }
        key = f"get_global_variable_{id}"

    elif nodeType == "SetGlobalVariable":
        outNode["type"] = "set_global_variable"
        pinData["in"]["0"] = "input"
        outNode["config"] = {
            "variables": {
                "input": data['name']
            }
        }
        key = f"set_global_variable_{id}"

    else:
        raise ValueError(f"Unknown node type: {nodeType}")

    return (key, outNode), pinData

if __name__ == "__main__":
    
    inConfig = load_json_file("controller-firmware/python/src/sandbox/test_config.json")

    outConfig = {}

    outConfig["info"] = inConfig["info"]    # info section is the same
    outConfig["networks"] = {}               # networks will be filled later

    pinData = {}

    # create user global variables
    outConfig["user_node_vars"] = {}
    for var in inConfig["set_node_vars"]:
        outConfig["user_node_vars"][var["name"]] = {
            "type": convert_io_type(var["type"])
        }


   # set global variables
    outConfig["node_var_values"] = {}  # not supported yet 


    for network in inConfig["networks"]:

        newNetwork = {}
        serialDeviceConfigs = {
            "writes": {},
            "reads": {}
        }

        # copy the network info
        newNetwork["enable"] = network["enabled"]
        newNetwork["type"] = network["net_type"].lower()
        newNetwork["timeout_usec"] = network["timeout"]
        newNetwork["update_cycle_trigger_count"] = network["update_cycle_trigger_count"]
        newNetwork["execution_order"] = network["execution_index"]


        # copy the nodes
        nodes = {}
        for nodeID, node in network["nodes"]["nodes"].items():
            nodeData = node["value"]

            # get the first key and its node data
            nodetype, first_entry = next(iter(nodeData.items()))
            nodeData = nodeData[nodetype]

            (key, outNode), p = convert_node(nodetype, nodeData, nodeID)

            if nodetype == "SerialWrite" or nodetype == "SerialRead":
                # these are not actual nodes, but simplify the serial device config
                # they need to be added to the serial device config afterwards
                
                if nodetype == "SerialWrite":
                    if nodeID in serialDeviceConfigs["writes"]:
                        raise ValueError(f"Serial write node {nodeID} already exists")
                    serialDeviceConfigs["writes"][nodeID] = outNode
                elif nodetype == "SerialRead":
                    if nodeID in serialDeviceConfigs["reads"]:
                        raise ValueError(f"Serial read node {nodeID} already exists")
                    serialDeviceConfigs["reads"][nodeID] = outNode
                continue


            if key in nodes:
                raise ValueError(f"Node {key} already exists in network {network['name']}")

            nodes[key] = outNode
            p["name"] = key
            p["type"] = nodetype
            pinData[nodeID] = p

        
        # add the serial device configs
        removeKeys = []
        for i, wire in enumerate(network["nodes"]["wires"]):
            src = str(wire["out_pin"]["node"])
            dst = str(wire["in_pin"]["node"])

            if src in serialDeviceConfigs["writes"]:
                # write node leading to a serial device
                deviceName = pinData[dst]["name"]
                write = serialDeviceConfigs["writes"][src]
                serialDeviceConfigs["writes"][src]["device_name"] = deviceName
                for k, v in write.items():
                    if k == "device_name":
                        continue
                    nodes[deviceName]["config"]["device"][k].append(v)
                removeKeys.append(i)
            elif dst in serialDeviceConfigs["reads"]:
                # read node leading from a serial device
                deviceName = pinData[src]["name"]
                read = serialDeviceConfigs["reads"][dst]
                serialDeviceConfigs["reads"][dst]["device_name"] = deviceName
                for k, v in read.items():
                    if k == "device_name":
                        continue
                    nodes[deviceName]["config"]["device"][k].append(v)
                removeKeys.append(i)

        # remove the wires that were used for serial read/write nodes
        for i in sorted(removeKeys, reverse=True):
            network["nodes"]["wires"].pop(i)

        # make the remaining valid connections
        connections = {}
        for i, wire in enumerate(network["nodes"]["wires"]):
            src = str(wire["out_pin"]["node"])
            dst = str(wire["in_pin"]["node"])

            src_node = None
            dst_node = None

            src_pin = None
            dst_pin = None

            if src in serialDeviceConfigs["reads"]:
                src_node = serialDeviceConfigs["reads"][src]["device_name"] # use the serial device name
                src_pin = ""
                dst_pin = pinData[dst]["in"].get(str(wire["in_pin"]["input"]), None)

            if dst in serialDeviceConfigs["writes"]:
                dst_node = serialDeviceConfigs["writes"][dst]["device_name"] # use the serial device name
                dst_pin = ""
                src_pin = pinData[src]["out"].get(str(wire["out_pin"]["output"]), None)

            if src_node is None:
                src_node = pinData[src]["name"]
            if dst_node is None:
                dst_node = pinData[dst]["name"]

            
            if src_pin is None and dst_pin is None:
                src_pin = pinData[src]["out"].get(str(wire["out_pin"]["output"]), None)
                dst_pin = pinData[dst]["in"].get(str(wire["in_pin"]["input"]), None)


            
            if src_pin is None or dst_pin is None:
                raise ValueError(f"Invalid pin connection from {src} to {dst}")
            
            connections[i] = {
                "src_node": src_node,
                "src_port": src_pin,
                "dst_node": dst_node,
                "dst_port": dst_pin
            }
        

        newNetwork["connections"] = connections
        newNetwork["nodes"] = nodes

        # add the network to the output config
        if(outConfig["networks"].get(network["name"]) is not None):
            raise ValueError(f"Network {network['name']} already exists")
        outConfig["networks"][network["name"]] = newNetwork

    print(outConfig)

    # save the output config
    save_json_file("controller-firmware/python/src/sandbox/test_config_out.json", outConfig)