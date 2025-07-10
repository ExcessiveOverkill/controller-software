import libscrc

# load in csv with decoded bytes from manchester signal

# sort packets by finding the start cmd from the controller

file = "controller-firmware/python/src/sandbox/t.txt"

def bin_to_bytes(s):
    return bytes(int(s[i:i+8][::-1], 2) for i in range(0, len(s), 8))

def get_packets(filename):
    with open(filename) as f:
        next(f) # skip header
        packet = []
        prev_ts = None
        for line in f:
            ts, value = line.strip().split(',')
            ts = float(ts)
            if prev_ts is not None and ts - prev_ts > 500e-9:
                yield packet
                packet = []
            prev_ts = ts
            packet.append(int(value))
        yield packet

results = []

for packet in get_packets(file):
    s = ''.join(str(x) for x in packet)
    if not s.startswith('0101010101010101001111110') or not s.count('111111') == 2 or not s.endswith('01111110'):
        print("bad packet")
        continue
    payload = s[25:-8].replace('111110', '11111')
    if payload == '1111111111111111':
        continue

    print(payload)
    b = bin_to_bytes(payload)

    #results.append(b)
    #print("\t".join(f"{byte:d}" for byte in b))

#print(results)

# 10100000 00100100 01011000 10010001 11011001 10110100 00000000 00100010 01011111 10010000 00000000 00000000 00100110 00010111