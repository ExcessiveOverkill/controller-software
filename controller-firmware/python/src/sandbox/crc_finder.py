from test1 import *




def crc_noref(bitstring, poly, init, xorout, width=5):
    """
    Computes a non-reflected CRC over the given bitstring.
    
    Parameters:
      bitstring (str): A string of '0' and '1'.
      poly (int): The CRC polynomial (e.g. 0x09 for CRC-5/EPC).
      init (int): The initial CRC value.
      xorout (int): The final XOR value.
      width (int): The CRC width (default 5).
    
    Returns:
      int: The computed CRC value.
    """
    crc = init
    mask = (1 << width) - 1
    topbit = 1 << (width - 1)
    for bit in bitstring:
        b = int(bit)
        # Grab the MSB of crc.
        msb = (crc & topbit) >> (width - 1)
        # Shift left and add the new bit.
        crc = ((crc << 1) & mask) | b
        if msb:
            crc ^= poly
    return crc ^ xorout


def crc_ref(bitstring, poly, init, xorout, width=5):
    """
    Computes a reflected CRC over the given bitstring.
    
    Parameters:
      bitstring (str): A string of '0' and '1'.
      poly (int): The CRC polynomial. For reflected CRCs, this is usually the reflected version.
      init (int): The initial CRC value.
      xorout (int): The final XOR value.
      width (int): The CRC width (default 5).
    
    Returns:
      int: The computed CRC value.
    """
    crc = init
    mask = (1 << width) - 1
    for bit in bitstring:
        b = int(bit)
        # XOR in the new bit into the LSB of crc.
        # (Note: for a fully bitwise algorithm, there are several approaches.
        # Here we process one bit at a time, shifting right.)
        lsb = crc & 1
        crc = (crc >> 1) | (b << (width - 1))
        if lsb:
            crc ^= poly
    return crc ^ xorout


# Define a dictionary of common CRC-5 variants.
# The parameters below are examples; please verify them against your protocol.
crc5_variants = {
    # CRC-5/EPC: Commonly used in RFID applications.
    "CRC-5/EPC": {"poly": 0x09, "init": 0x09, "refin": False, "xorout": 0x00},
    # CRC-5/ITU: Used in certain telecommunication protocols.
    "CRC-5/ITU": {"poly": 0x15, "init": 0x00, "refin": True,  "xorout": 0x00},
    # CRC-5/USB: Used in USB protocols.
    "CRC-5/USB": {"poly": 0x05, "init": 0x1F, "refin": True,  "xorout": 0x1F},
    # CRC-5/ROHC: Used in ROHC (Robust Header Compression).
    "CRC-5/ROHC": {"poly": 0x15, "init": 0x1F, "refin": True,  "xorout": 0x00},
    # CRC-5/BBC: Often used in older BBC/ARC protocols.
    "CRC-5/BBC": {"poly": 0x09, "init": 0x00, "refin": False, "xorout": 0x00},
}


def find_crc5_candidates(rx_packet, variant_parameters, start_range, end_range):
    """
    Given an RX packet as a binary string, this function will try various candidate slices
    (i.e. different assumed boundaries for the data+CRC) and run each through all defined
    CRC-5 variants. If the computed CRC equals zero, it is considered a candidate match.
    
    Parameters:
      rx_packet (str): The entire RX packet as a binary string (e.g., "110101001...")
      variant_parameters (dict): A dictionary of CRC-5 variants (like crc5_variants).
      start_range (tuple): A tuple (min_start, max_start) for candidate start indices.
      end_range (tuple): A tuple (min_end, max_end) for candidate end indices. (The candidate
                         is rx_packet[start:end]. Typically you need at least 5 bits at the end.)
    
    Returns:
      list of dicts: Each dict contains keys: 'variant', 'start', 'end', 'candidate' (the binary
                     substring tested), and 'crc' (which should be zero for a valid candidate).
    """
    results = []
    # Iterate over each CRC-5 variant.
    for variant_name, params in variant_parameters.items():
        poly = params["poly"]
        init = params["init"]
        refin = params["refin"]
        xorout = params["xorout"]
        
        # Try various candidate boundaries.
        for start in range(start_range[0], min(start_range[1], len(rx_packet)) + 1):
            for end in range(end_range[0], min(end_range[1], len(rx_packet)) + 1):
                # Make sure the candidate is long enough to include a 5-bit CRC.
                if end - start < 5:
                    continue
                candidate = rx_packet[start:end]
                # Compute the CRC using the appropriate algorithm.
                if not refin:
                    crc_val = crc_noref(candidate, poly, init, xorout, width=5)
                else:
                    crc_val = crc_ref(candidate, poly, init, xorout, width=5)
                if crc_val == 0:
                    results.append({
                        "variant": variant_name,
                        "start": start,
                        "end": end,
                        "candidate": candidate,
                        "crc": crc_val
                    })
    return results


# =============================================================================
# Example usage:
#
# Suppose you have an RX packet from your decoded data as a binary string.
# You are not 100% sure where the CRC portion starts/ends, so you decide to try
# candidate segments with different boundaries. For example:
#
#   - Try candidate segments starting between indices 0 and 5.
#   - Try candidate segments ending between (len(rx_packet) - 10) and len(rx_packet).
#
# Replace 'rx_packet' below with your actual binary data.
# =============================================================================
if __name__ == "__main__":
    # Example RX packet (replace with your actual decoded binary string).
    #rx_packet = "010101101100000111001101101110001001111111111111000000111000000011100101001000001001110101"

    all_candidates = {}

    total_valid = 0

    for idx in range(0, min(len(decoded_packets), 1000)):
        rx_packet = decoded_packets[idx][1]


        # Define the candidate ranges.
        # start_range = (0, 0)
        # end_range = (len(rx_packet) - 1, len(rx_packet))
        # end_range = (100, 100)
        
        # candidates = find_crc5_candidates(rx_packet, crc5_variants, start_range, end_range)
        
        # if candidates:
        #     #print("Found candidate(s) with a zero CRC:")
        #     for candidate in candidates:
        #         #print(f"Variant: {candidate['variant']}, "
        #         #    f"start index: {candidate['start']}, end index: {candidate['end']}, "
        #         #    f"candidate bits: {candidate['candidate']}, computed CRC: {candidate['crc']}")
                
        #         name = f"{candidate['variant']}_{candidate['start']}_{candidate['end']}"
        #         if name in all_candidates:
        #             all_candidates[name] += 1
        #         else:
        #             all_candidates[name] = 1
        #     total_valid += 1
        # else:
        #     pass
        #     #print("No valid CRC candidate found in the specified range.")


        # add bad bit, find any bit between 1 and 99 and flip it
        if 0:
            for bit in range(1, 100):
                rx_packet = rx_packet[:bit] + str(int(rx_packet[bit]) ^ 1) + rx_packet[bit + 1:]


        found = False
        for poly in range(11, 12):
            for init in range(0, 1):
                for xorout in range(0, 1):
                    crc = crc_noref(rx_packet[0:100], poly, init, xorout, width=5)
                    if(crc == 0):
                        name = f"{poly}_{init}_{xorout}"
                        #print(f"Found candidate with a zero CRC: {name}")
                        if name in all_candidates:
                            all_candidates[name] += 1
                        else:
                            all_candidates[name] = 1
                        found = True

        if found:
            total_valid += 1

    print(f"Total valid packets: {total_valid}")

    for candidate, count in sorted(all_candidates.items(), key=lambda item: item[1], reverse=True):
        print(f"{candidate}: {count}")
