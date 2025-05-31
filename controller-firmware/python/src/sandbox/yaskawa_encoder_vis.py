



packetDecoding = {
    "single_turn": (60, 75),
    "multi_turn": (76, 91),
    "unknown_1": (56, 59),
    "unknown_2": (40, 47),
}



def get_packets(filename):
    with open(filename) as f:
        next(f) # skip header
        packet = {}
        byteData = ""
        for line in f:
            t, status, timestamp, duration, mosi, miso = line.strip().split(',')
            status = status[1:-1]
            if status == "enable":
                packet = {}
                timestamp = float(timestamp)
                packet["timestamp"] = timestamp
                byteData = ""
                continue
            
            if status == "disable":
                if len(byteData) != 13*8:   # 13 bytes, less than this means we have a bad packet
                    continue

                # interpret the byte data
                for key, (start, end) in packetDecoding.items():
                    bits = byteData[start:end+1]
                    bits = bits[::-1]
                    number = int("0b"+bits, base=0)
                    packet[key] = number
                yield packet
                continue
            
            miso = miso[2:]
            byteData += miso[::-1]



if __name__ == "__main__":
    file = "controller-firmware/python/src/sandbox/yaskawa_decoded_bytes.csv"
    packets = []
    for packet in get_packets(file):
        packets.append(packet)

    packets = packets[0:100]  # limit the number of packets for testing

    # print the packets
    # for packet in packets:
    #     print(f"timestamp: {packet['timestamp']}, single_turn: {packet['single_turn']}, multi_turn: {packet['multi_turn']}, unknown_1: {packet['unknown_1']}, unknown_2: {packet['unknown_2']}")

    # graph the packet values
    import matplotlib.pyplot as plt

    timestamps = [pkt["timestamp"] for pkt in packets]
    single_turn = [pkt["single_turn"] for pkt in packets]
    multi_turn = [pkt["multi_turn"] for pkt in packets]
    unknown_1 = [pkt["unknown_1"] for pkt in packets]
    unknown_2 = [pkt["unknown_2"]*256 for pkt in packets]

    # bits0 = []
    # bits1 = []
    # bits2 = []
    # bits3 = []

    # for i in unknown_1:
    #     bits0.append((i >> 0) & 1)
    #     bits1.append((i >> 1) & 1)
    #     bits2.append((i >> 2) & 1)
    #     bits3.append((i >> 3) & 1)

    plt.figure(figsize=(10, 6))
    plt.plot(timestamps, unknown_2, label='unknown_2', color='blue')
    plt.plot(timestamps, single_turn, label='single_turn', color='red')
    plt.xlabel('seconds')
    plt.ylabel('unknown_2')
    plt.title('Unknown 2 vs Seconds')
    plt.grid(True)
    plt.show()