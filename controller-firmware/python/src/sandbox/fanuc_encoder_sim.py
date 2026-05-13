import csv
import os

class rs422_sim():
    def __init__(self, data_file, clock_frequency):
        self.file = data_file

        self.current_transfer = 0
        self.current_edge = 0

        self.transfers = []
        self.clock_period = 1/clock_frequency

        self.time = 0
        self.tx_level = 1
        self.req_signal = 0
        self.error = False


        inactive_threshold = 30e-6   # 30us, used to detect when a transfer is not active

        self.req_width = 8e-6   # 8us, width of the request signal
        self.req_tolerance = .5e-6  # .5us, tolerance for the request signal (how much the request signal can be off from the expected width)
        self.req_response_time = 1e-6  # 1us, time it takes for the transfer to start after the request signal is received

        self.measured_req_width = 0

        with open(self.file, 'r') as f:
            reader = csv.reader(f)
            temp = []        
            for index, row in enumerate(reader):
                if index == 0:
                    continue    # skip the header row

                temp.append([float(row[0]), bool(int(row[1]))])

                if len(temp) > 2 and temp[-1][0] - temp[-2][0] > inactive_threshold:
                    self.transfers.append(temp[1:-1])
                    temp = temp[-2:]
                    

    def tick(self):
        # increment the time by the clock period and update internal state

        if self.req_signal:
            self.measured_req_width += self.clock_period
        elif self.measured_req_width > 0:
            if abs(self.measured_req_width - self.req_width) < self.req_tolerance:
                print("Request signal received, starting transfer")
                self.start_response()
            else:
                print("Request signal width not within tolerance")
            
            self.measured_req_width = 0

        

        self.time += self.clock_period

    def set_request_level(self, level):
        self.req_signal = (level != 0)

    def get_tx_level(self):
        return self.tx_level
    
    def inject_error(self): # inject an error into the transfer, this skips an edge somewhere in the transfer
        self.error = True
    
    def start_response(self):
        self.current_transfer += 1
        self.current_edge = 0

        if self.current_transfer >= len(self.transfers):    # loop back to the beginning if we reach the end
            self.current_transfer = 0

        self.time = self.transfers[self.current_transfer][0][0] - self.req_response_time

        self.tx_level = 1

    def get_tx_level(self):
        if self.current_edge >= len(self.transfers[self.current_transfer]):   # nothing to do once we reach the end of the transfer
            self.tx_level = 1   # return to idle state, even though the transfer should have set it to zero already
        
        elif self.time >= self.transfers[self.current_transfer][self.current_edge][0]:
            self.tx_level = self.transfers[self.current_transfer][self.current_edge][1]
            self.current_edge += 1

            if self.error and (5 < self.current_edge < len(self.transfers[self.current_transfer]) - 5):    # flip an edge in the transfer
                self.error = False
                self.tx_level = not self.tx_level

        return self.tx_level
    

HIGH = 1
LOW = 0
FLOATING = -1

class rs485_sim():
    def __init__(self, data_file, clock_frequency):
        self.file = data_file

        self.current_transfer = 0
        self.current_edge = 0

        self.transfers = []
        self.clock_period = 1/clock_frequency

        self.time = 0
        
        self.signal_in_level = FLOATING
        self.signal_out_level = FLOATING
        self.error = False

        self.last_signal_in_level = FLOATING
        self.last_signal_in_time = 0

        self.input_edges = []

        self.out_edge = 0

        self.transmit = False
        self.start_time = 0

        with open(self.file, 'r') as f:
            reader = csv.reader(f)
            temp = []
            last_floating_time = None
            last_level = LOW
            record_next = False
            for index, row in enumerate(reader):
                if index == 0:
                    continue    # skip the header row

                if(row[1] == "high"):
                    level = HIGH
                elif(row[1] == "low"):
                    level = LOW
                else:                    
                    level = FLOATING
                
                time = float(row[0])

                if last_level == FLOATING and level != FLOATING:
                    last_floating_time = time

                if record_next:
                    temp.append([time, level])
                    if level == FLOATING:
                        record_next = False
                        if (time - last_floating_time < 55e-6 and time - last_floating_time > 40e-6):
                            self.transfers.append(temp)
                        else:
                            # print("Warning: detected request signal, but transfer timing is not within expected range, skipping transfer")
                            pass
                        temp = []

                # save only the edges from the encoder packet
                if last_floating_time is not None and level == FLOATING and (time - last_floating_time < 5e-6 and time - last_floating_time > 4e-6):
                    record_next = True

                last_level = level
    
    def tick(self):
        
        # handle request signal

        new_edge = False
        if self.signal_in_level != self.last_signal_in_level:
            self.last_signal_in_level = self.signal_in_level
            self.last_signal_in_time = self.time
            new_edge = True

        if self.last_signal_in_level == FLOATING and self.signal_in_level == FLOATING and self.time - self.last_signal_in_time > 20e-6:
            # timeout, reset input state
            self.input_edges = []
            self.input_bits = []

        if new_edge:
            self.input_edges.append((self.time, self.signal_in_level))

        if len(self.input_edges) > 0 and self.signal_in_level == FLOATING:
            # process the input edges to extract bits, and validate timing
            edges = self.input_edges
            self.input_edges = []
            bits = []
            bit_time = 370e-9
            enable_delay = 480e-9

            invalid = False
            
            # remove the start enable delay
            level = edges[0][1]
            duration = edges[1][0] - edges[0][0]
            if level == HIGH and duration > enable_delay*0.8 and duration < enable_delay*1.2:
                edges = edges[1:]
            else:
                invalid = True
                print("invalid start enable delay")

            # remove the end enable delay
            level = edges[-2][1]
            duration = edges[-1][0] - edges[-2][0]
            if len(edges) > 0 and level == HIGH and duration > enable_delay*0.8 and duration < enable_delay*1.2:
                edges = edges[:-1]

            else:
                invalid = True
                print("invalid end enable delay")

            for i in range(1, len(edges)):
                length = edges[i][0] - edges[i-1][0]
                bit_count = round(length / bit_time)
                if length / bit_count < bit_time*0.9 or length / bit_count > bit_time*1.1:
                    invalid = True
                    print(f"invalid bit timing detected, length: {length}, bit_count: {bit_count}, expected bit_time: {bit_time}, actual bit_time: {length/bit_count}")
                    break
                bits.extend([edges[i-1][1]] * bit_count)
            
            if invalid:
                print("Warning: invalid input signal detected, timing does not match expected values, ignoring input")
            else:
                # check if the bits match the expected default pattern
                if bits != [0, 0, 0, 0, 1, 1, 1, 0, 0]: # default signal from rj3ib controller, used to trigger the encoder response
                    print(f"Warning: invalid input signal detected, first 8 bits do not match expected pattern, ignoring ({bits})")
                else:
                    self.transmit = True
                    self.start_time = self.time
                    print("Valid request signal detected, starting transfer")


        if self.transmit:
            start_edge_time = self.transfers[self.current_transfer][0][0]
            edge_time, edge_level = self.transfers[self.current_transfer][self.out_edge]
            edge_time -= start_edge_time
            
            if self.time - self.start_time >= edge_time:
                self.signal_out_level = edge_level
                self.out_edge += 1

                if self.out_edge >= len(self.transfers[self.current_transfer]):
                    self.out_edge = 0
                    self.current_transfer += 1
                    if self.current_transfer >= len(self.transfers):
                        self.current_transfer = 0
                    self.transmit = False

        if self.signal_in_level != FLOATING and self.signal_out_level != FLOATING:
            print("Error: both input and output signals are active, this should not happen in rs485 mode")

        self.time += self.clock_period
    
    def set_inputs(self, tx, tx_enable):
        self.signal_in_level = tx if tx_enable else FLOATING

    def get_tx_level(self):
        return self.signal_out_level

if __name__ == "__main__":
    print("Current Directory:", os.getcwd())
    # sim = rs422_sim("controller-firmware/python/src/sandbox/fanuc_encoder_rs422.csv", 25e6)
    # print(sim.transfers)
    sim = rs485_sim("controller-firmware/python/src/sandbox/fanuc_rs485_detected_edges.csv", 25e6)
    print(sim.transfers)