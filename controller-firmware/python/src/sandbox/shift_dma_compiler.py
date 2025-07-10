
# Higher level node interpreter
"""
Convert the FPGA node diagram into a sequence of high level instructions instructions.

each node in the diagram must contain the following information:
- Trigger instruction: running this instruction will trigger the node to start executing (a NOP can be used if the execution starts automatically)
- Trigger delay: the delay between the trigger instruction and the start of the target action
- Completion delay: the maximum time it takes for the node to complete its action and any resulting data to be available to use

"""

# Low level compiler
"""
Convert high level instructions into a sequence of low level instructions that the FPGA DMA can execute.


"""

class copy_instruction:
    def __init__(self, src_node:int, dst_node:int, src_addr:int, dst_addr:int):
        """
        params:
        src_node: the source node to copy from
        dst_node: the destination node to copy to
        src_addr: the address in the source node to copy from
        dst_addr: the address in the destination node to copy to
        """
        self.src_node = src_node
        self.dst_node = dst_node
        self.src_addr = src_addr
        self.dst_addr = dst_addr

    def __str__(self):
        return f"COPY {self.src_node}:{self.src_addr} -> {self.dst_node}:{self.dst_addr}"
    
    def create_instruction(self):
        return self.src_node | (self.dst_node << 8) | (self.src_addr << 16) | (self.dst_addr << 32) | (2 << 48)
    
class wait_instruction:
    def __init__(self, time:int):
        """
        params:
        time: the number of cycles to wait
        """

        if(time < 0):
            raise ValueError("Invalid time value")

        self.time = time

    def __str__(self):
        return f"WAIT {self.time}"
    
    def create_instruction(self):
        return self.time | (3 << 48)
    
class end_instruction:
    def __str__(self):
        return "END"
    
    def create_instruction(self):
        return 0
    
class nop_instruction:
    def __str__(self):
        return "NOP"
    
    def create_instruction(self):
        return 1 << 48



class high_level_instruction:
    def __init__(self, instruction:copy_instruction, time_A:int, time_B:int, type:str):
        """
        params:
        instruction: the low level instruction to execute, only COPY is supported for now
        time_A: the earliest time at which the instruction should be executed (in clock cycles)
        time_B: the latest time at which the instruction should be executed (in clock cycles)
        
        """
        self.instruction = instruction
        self.time_A = time_A
        self.time_B = time_B
        self.type = type
        self.exact_time = None

        if(type not in ["write", "read", "write_window", "read_window"]):
            raise ValueError("Invalid instruction type")

        if(time_A < 0 or time_B < 0):
            raise ValueError("Invalid time value")
        
        if(time_A > time_B and (type == "write_window" or type == "read_window")):
            raise ValueError("Invalid time values for window")
        
        if(time_A == time_B):
            self.exact_time = time_A

    def __str__(self):
        return f"{self.type} {self.instruction} @ {self.time_A} - {self.time_B}"
    
    def __repr__(self):
        return self.__str__()
    
    def create_instruction(self):
        return self.instruction.create_instruction()



class ll_compiler:
    def __init__(self):
        self.window_instructions = []
        self.exact_instructions = []
        self.current_time = 0

        self.inter_node_cycles = 1  # number of cycles for data to move between nodes
        self.intra_node_cycles = 2  # number of cycles for data to move within a node
        self.dma_cycles = 1  # number of cycles the DMA adds to the chain before node 0
        self.total_nodes = 5    # including the DMA node

        self.full_cycle_time = self.intra_node_cycles*self.total_nodes + self.inter_node_cycles*self.total_nodes + self.dma_cycles

        self.timeline = {}

        self.compiled = False


    def add_instruction(self, instruction:high_level_instruction):
        """
        Add a high level instruction to the list of instructions to compile
        """
        if(instruction.exact_time != None):
            self.exact_instructions.append(instruction)
        else:
            self.window_instructions.append(instruction)

    def compile(self):
        """
        Compile the list of high level instructions into a sequence of low level instructions
        """
        
        # place exact instructions first since they don't have any flexibility
        self.place_exact_instructions()

        # place window instructions
        self.place_window_instructions()

        # sort the timeline from earliest to latest
        self.timeline = dict(sorted(self.timeline.items()))

        # condense the timeline by adding wait instructions where necessary
        self.condense()

        self.compiled = True

    def place_exact_instructions(self):
        """
        Place exact instructions in the timeline
        """

        for inst in self.exact_instructions:

            send_time = inst.time_A
            write = False
            src_node = inst.instruction.src_node
            dst_node = inst.instruction.dst_node

            if(inst.type == "write" or inst.type == "write_window"):
                write = True


            # handle time from dma to target
            if(write):
                send_time -= self.dma_cycles
                send_time -= (self.intra_node_cycles * dst_node)
                send_time -= (self.inter_node_cycles * dst_node)
            else:
                send_time -= self.dma_cycles
                send_time -= (self.intra_node_cycles * src_node)
                send_time -= (self.inter_node_cycles * src_node)

            if(send_time < 0):
                raise ValueError(f"Not enough begin time for instruction: {inst}")

            
            # handle wrapping cases
            if(write and dst_node <= src_node):

                if(send_time in self.timeline.keys() or send_time-self.full_cycle_time in self.timeline.keys()):
                    raise ValueError(f"Overlapping exact instructions: {inst} and {self.timeline[send_time]}")
                
                self.timeline[send_time] = [inst, "block"] 
                send_time -= self.full_cycle_time
                self.timeline[send_time] = [inst, "send"]
            else:
                if(send_time in self.timeline.keys()):
                    raise ValueError(f"Overlapping exact instructions: {inst} and {self.timeline[send_time]}")
                self.timeline[send_time] = [inst, "send"]

    def place_window_instructions(self):
        """
        Place window instructions in the timeline
        """

        for inst in self.window_instructions:
            write = False
            src_node = inst.instruction.src_node
            dst_node = inst.instruction.dst_node

            if(inst.type == "write" or inst.type == "write_window"):
                write = True

            success = False

            for i in range(inst.time_A, inst.time_B+1):
                send_time = i

                # handle time from dma to target
                if(write):
                    send_time -= self.dma_cycles
                    send_time -= (self.intra_node_cycles * dst_node)
                    send_time -= (self.inter_node_cycles * dst_node)
                else:
                    send_time -= self.dma_cycles
                    send_time -= (self.intra_node_cycles * src_node)
                    send_time -= (self.inter_node_cycles * src_node)

                if(send_time < 0):
                    continue

                
                # handle wrapping cases
                if(write and dst_node <= src_node):
                    if(send_time in self.timeline.keys() or send_time-self.full_cycle_time in self.timeline.keys()):
                        continue

                    self.timeline[send_time] = [inst, "block"]
                    send_time -= self.full_cycle_time
                    self.timeline[send_time] = [inst, "send"]
                else:
                    if(send_time in self.timeline.keys()):
                        continue
                    self.timeline[send_time] = [inst, "send"]

                success = True
                break

            if(not success):
                # TODO: try moving around already placed instructions to make space
                raise ValueError(f"Could not place instruction within window, no available space: {inst}")
            
    def condense(self):
        """
        Condense the timeline by adding wait instructions
        """
        condensed_timeline = {}
        index = 0
        time = 0

        prev_send_time = 0
        for i, data in self.timeline.items():
            if(data[1] == "block"):
                continue
            
            diff = i - prev_send_time
            if(diff > 2):
                # more than 2 cycles of wait time, add wait instruction
                time += diff-1
                condensed_timeline[index] = [wait_instruction(time), "wait"]
                index += 1
            # elif(diff > 1):
            #     # only 1 cycle of wait time, add nop instruction
            #     condensed_timeline[index] = [nop_instruction(), "wait"]
            #     time += 1
            #     index += 1

            condensed_timeline[index] = data
            index += 1
            time += 1

            prev_send_time = i

        # add a final wait instruction to ensure that all instructions are complete before finishing the timeline
        condensed_timeline[index] = [wait_instruction(self.full_cycle_time + time), "wait"]
        index += 1

        # add an end instruction to finish the timeline
        condensed_timeline[index] = [end_instruction(), "end"]

        self.timeline = condensed_timeline
            

    def visualize(self):
        """
        Visualize the timeline at the data entry point along the DMA chain
        """

        print("\nDMA TIMELINE\n")
        print("TIME\tTYPE\tINSTRUCTION")

        for index, data in self.timeline.items():
            p = f"{index}\t"
            if(data[1] == "send"):
                p += "SEND\t"
            elif(data[1] == "block"):
                p += "BLOCK\t"
            elif(data[1] == "wait"):
                p += "WAIT\t"

            p += str(data[0])
            print(p)


    def export(self):
        """
        Export the timeline to a format that can be used by the DMA
        """

        if(not self.compiled):
            raise ValueError("Timeline has not been compiled yet")

        output = []
        for index, data in self.timeline.items():
            print(f"{data[0].create_instruction()},")
            output.append(int(data[0].create_instruction()))

        return output
    
        






if __name__ == "__main__":
    c = ll_compiler()
    c.add_instruction(high_level_instruction(copy_instruction(0, 1, 0, 0), 10, 10, "write"))
    c.add_instruction(high_level_instruction(copy_instruction(0, 2, 5, 5), 10, 10, "write"))

    c.add_instruction(high_level_instruction(copy_instruction(0, 1, 1, 1), 20, 20, "write"))

    c.add_instruction(high_level_instruction(copy_instruction(0, 1, 2, 2), 10, 15, "write"))
    c.add_instruction(high_level_instruction(copy_instruction(0, 1, 3, 3), 10, 15, "write"))
    c.add_instruction(high_level_instruction(copy_instruction(0, 1, 4, 4), 10, 15, "write"))

    c.compile()
    c.visualize()
    c.export()
            
        

    

 