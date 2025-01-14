import numpy as np

END = 0
NOP = 1
COPY = 2


def create_instruction(source_node, destination_node, source_address, destination_address, instruction):
    data = source_node | (destination_node << 8) | (source_address << 16) | (destination_address << 32) | (instruction << 48)
    return data


def extract_instruction(data):
    source_node = data & 0xff
    destination_node = (data >> 8) & 0xff
    source_address = (data >> 16) & 0xffff
    destination_address = (data >> 32) & 0xffff
    instruction = (data >> 48) & 0xf

    print('source_node:', source_node)
    print('destination_node:', destination_node)
    print('source_address:', source_address)
    print('destination_address:', destination_address)
    print('instruction:', instruction)

    return source_node, destination_node, source_address, destination_address, instruction

# extract_instruction(562950020530432)
# print("\n\n")
# extract_instruction(562954315563264)
# print("\n\n")
# extract_instruction(562962905563392)
# print("\n\n")
# extract_instruction(562949953552385)
# print("\n\n")
# extract_instruction(562954248650753)
# print("\n\n")

extract_instruction(562958543355906)
print("\n\n")

extract_instruction(562962838388738)
print("\n\n")

extract_instruction(562967133421570)
print("\n\n")

extract_instruction(562971428454402)
print("\n\n")

extract_instruction(562950020530432)
print("\n\n")

print(np.log2(256))


#0xb6fc8008
#0xb6fc800c
