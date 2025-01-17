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


if __name__ == "__main__":
    print("Current Directory:", os.getcwd())
    sim = rs422_sim("controller-firmware/python/src/sandbox/fanuc_encoder_rs422.csv", 25e6)
    print(sim.transfers)