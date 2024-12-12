



class compiler:
    def __init__(self):
        self.high_level_instructions = []
        self.output = []

    def copy(self, src_node, src_addr, dest_node, dest_addr):
        self.high_level_instructions.append([src_node, dest_node, "copy"])
def compile():
    output = []
    return output