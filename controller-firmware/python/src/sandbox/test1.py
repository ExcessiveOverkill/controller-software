import pandas as pd
import numpy as np

def process_rs485_signal(input_csv, 
                         output_csv, 
                         edge_threshold, 
                         low_threshold, 
                         high_threshold, 
                         window_size=6, 
                         samples_to_average=10, 
                         refractory_samples=15):
    """
    Processes an analog RS485 capture CSV to detect signal edges and assign levels.

    Parameters:
      input_csv (str): Path to the input CSV file. The CSV should have at least two columns: 'time' and 'voltage'.
      output_csv (str): Path to the output CSV file that will contain the detected edges.
      edge_threshold (float): Minimum voltage change (over the past window_size samples) required to detect an edge.
      low_threshold (float): Upper bound for the averaged voltage to be considered "low".
      high_threshold (float): Lower bound for the averaged voltage to be considered "high".
      window_size (int): Number of previous samples to compare against for detecting a change (default: 5).
      samples_to_average (int): Number of samples to average after an edge is detected to determine the new level (default: 20).
      refractory_samples (int): Number of samples to skip after detecting an edge to avoid multiple triggers (default: 20).

    The function writes a CSV with two columns: 'time' (the time at which the edge was detected)
    and 'level' (the new state: "low", "high", or "floating").
    """
    # Load the CSV into a DataFrame.
    data = pd.read_csv(input_csv)
    
    # Ensure that the required columns are present.
    if not {"time", "voltage"}.issubset(data.columns):
        raise ValueError("Input CSV must contain 'time' and 'voltage' columns.")
    
    times = data['time'].values
    voltages = data['voltage'].values
    N = len(voltages)
    
    edges = []  # List to store detected edges as [time, level]
    i = window_size  # Start after the initial window

    # Loop until we have enough remaining samples for the averaging window.
    while i < N - samples_to_average:
        # Compare the current voltage with the voltage from 'window_size' samples ago.
        voltage_change = voltages[i] - voltages[i - window_size]
        if abs(voltage_change) >= edge_threshold:
            # Average the next 'samples_to_average' samples to determine the new level.
            avg_voltage = np.mean(voltages[i:i + samples_to_average])
            
            if avg_voltage < low_threshold:
                level = "low"
            elif avg_voltage > high_threshold:
                level = "high"
            else:
                level = "floating"
            
            # Record the time of the detected edge and its new level.
            edges.append([times[i], level])
            
            # Skip ahead by a refractory period to avoid multiple detections of the same edge.
            i += refractory_samples
        else:
            i += 1

    # Convert the list of edges to a DataFrame and write it to CSV.
    edges_df = pd.DataFrame(edges, columns=['time', 'level'])
    edges_df.to_csv(output_csv, index=False)



def decode_rs485_packets(edges_csv, packet_gap_threshold, bit_interval):
    """
    Decodes RS485 packets from an edges CSV file into a list of (tx_data, rx_data) pairs.
    
    Parameters:
      edges_csv (str): Path to the CSV file containing edges. The CSV should have two columns:
                       'time' (timestamps) and 'level' (string: "low", "high", or "floating").
      packet_gap_threshold (float): Time gap (in the same units as 'time') that indicates a new packet.
      bit_interval (float): Expected time duration of one bit period.
      
    Returns:
      List[Tuple[str, str]]: A list of tuples where each tuple is (tx_data, rx_data). 
                             Each is a binary string representing the data from that transfer.
    
    Assumptions:
      - The CSV rows are in chronological order.
      - Each packet consists of two transfers:
            * TX transfer: starts with a start bit (first edge) and then the TX data bits.
            * RX transfer: starts after a floating level is detected; its first edge is a start bit (and not a data bit).
      - Signal levels are interpreted as: "low" -> '0' and "high" -> '1'.
      - Edges occur on bit boundaries, but if the time between consecutive edges is greater than one bit_interval,
        we assume that the previous bit was held for the missing bit periods.
    """
    # Read the CSV file into a DataFrame.
    df = pd.read_csv(edges_csv)
    if not {"time", "level"}.issubset(df.columns):
        raise ValueError("CSV must contain 'time' and 'level' columns.")
    
    # Create a list of (time, level) tuples.
    edges = list(zip(df['time'], df['level']))
    
    # Partition the entire stream into packets based on a time gap threshold.
    packets = []
    current_packet = [edges[0]]
    for i in range(1, len(edges)):
        current_edge = edges[i]
        prev_edge = edges[i - 1]
        if (current_edge[0] - prev_edge[0]) > packet_gap_threshold:
            # A large time gap indicates a new packet.
            packets.append(current_packet)
            current_packet = [current_edge]
        else:
            current_packet.append(current_edge)
    if current_packet:
        packets.append(current_packet)
    
    def decode_segment(segment_edges, bit_interval):
        """
        Decodes a transfer segment (list of (time, level)) into a binary string.
        The first edge is assumed to be a start bit and is skipped.
        Uses timing differences to determine if a bit is held for multiple bit periods.
        
        Parameters:
          segment_edges (List[Tuple[float, str]]): Edges for one transfer (either TX or RX).
          bit_interval (float): The expected duration of one bit period.
          
        Returns:
          A binary string (e.g., "101010") representing the data bits.
          Returns an empty string if there are insufficient edges to decode.
        """
        if len(segment_edges) < 2:
            # Not enough edges to have a start bit and at least one data bit.
            return ""
        
        bits = []
        # Start with the second edge (first edge after the start bit).
        prev_time, prev_level = segment_edges[1]
        if prev_level.lower() == "floating":
            # If the first data edge is floating, we cannot decode a valid bit.
            return ""
        prev_bit = "0" if prev_level.lower() == "low" else "1"
        bits.append(prev_bit)
        last_time = prev_time

        # Process remaining edges.
        for (t, level) in segment_edges[2:]:
            # Skip any floating edges that might appear within the transfer.
            if level.lower() == "floating":
                continue
            dt = t - last_time
            # Determine how many bit periods have passed.
            # We use rounding to account for slight drift.
            n_intervals = max(1, int(round(dt / bit_interval)))
            # If more than one bit period elapsed, assume the previous bit was held.
            if n_intervals > 1:
                bits.extend([prev_bit] * (n_intervals - 1))
            # Append the new bit.
            current_bit = "0" if level.lower() == "low" else "1"
            bits.append(current_bit)
            last_time = t
            prev_bit = current_bit
        
        return "".join(bits)
    
    packets_data = []  # List to hold the decoded (tx_data, rx_data) pairs.
    
    # Process each packet.
    for packet in packets:
        # Look for the first occurrence of a floating edge.
        # This edge marks the boundary between TX and RX.
        floating_index = None
        for i, (t, level) in enumerate(packet):
            if level.lower() == "floating":
                floating_index = i
                break
        if floating_index is None:
            # No floating edge found in this packet; skip or handle as needed.
            continue
        
        # The TX segment is assumed to be all edges before the floating transition.
        tx_edges = packet[:floating_index]
        # The RX segment starts after the floating edge.
        rx_edges = packet[floating_index + 1:]
        
        # Decode each segment.
        tx_data = decode_segment(tx_edges, bit_interval)
        rx_data = decode_segment(rx_edges, bit_interval)
        
        packets_data.append((tx_data, rx_data))
    
    return packets_data

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def visualize_single_packet(analog_csv, edges_csv, decoded_packets, packet_index, packet_gap_threshold):
    """
    Visualizes a single packet by plotting the analog waveform (with edge markers)
    and displaying the decoded binary TX/RX data for that packet.
    
    Parameters:
      analog_csv (str): Path to the analog capture CSV file. Should contain "time" and "voltage" columns.
      edges_csv (str): Path to the edges CSV file. Should contain "time" and "level" columns.
      decoded_packets (List[Tuple[str, str]]): A list of (tx_data, rx_data) tuples, one per packet.
      packet_index (int): Index (0-indexed) of the packet to visualize.
      packet_gap_threshold (float): Time gap (in same units as 'time') that indicates a new packet.
      
    The function partitions the edges into packets, selects the specified packet, determines the
    time window covering that packet (with a margin), and plots:
        - The analog waveform (restricted to that window)
        - Vertical dashed lines at each edge (color-coded by level)
        - A text box below showing the TX and RX binary strings for that packet.
    """
    # Load the analog signal.
    analog_df = pd.read_csv(analog_csv)
    time_data = analog_df['time'].values
    voltage = analog_df['voltage'].values

    # Load the edges.
    edges_df = pd.read_csv(edges_csv)
    edges = list(zip(edges_df['time'].values, edges_df['level'].values))
    
    # Partition the edges into packets.
    packets = []
    current_packet = [edges[0]]
    for i in range(1, len(edges)):
        # If the gap is larger than the threshold, start a new packet.
        if (edges[i][0] - edges[i-1][0]) > packet_gap_threshold:
            packets.append(current_packet)
            current_packet = [edges[i]]
        else:
            current_packet.append(edges[i])
    if current_packet:
        packets.append(current_packet)
    
    # Check if packet_index is valid.
    if packet_index < 0 or packet_index >= len(packets):
        raise ValueError(f"Packet index {packet_index} is out of range. There are only {len(packets)} packets.")
    
    # Select the packet.
    packet_edges = packets[packet_index]
    
    # Determine time window for this packet.
    packet_start = packet_edges[0][0]
    packet_end = packet_edges[-1][0]
    # Use 5% of the packet duration as margin on each side (or a minimum margin if duration is 0)
    margin = max((packet_end - packet_start) * 0.05, 0.000001)
    window_start = packet_start - margin
    window_end = packet_end + margin
    
    # Filter analog data to this time window.
    window_mask = (time_data >= window_start) & (time_data <= window_end)
    time_window = time_data[window_mask]
    voltage_window = voltage[window_mask]
    
    # Get the decoded data for this packet.
    tx_data, rx_data = decoded_packets[packet_index] if packet_index < len(decoded_packets) else ("", "")
    
    # Create the figure with two subplots.
    fig, (ax_waveform, ax_text) = plt.subplots(2, 1, figsize=(14, 8),
                                               gridspec_kw={'height_ratios': [3, 1]},
                                               sharex=True)
    
    # Plot the analog waveform.
    ax_waveform.plot(time_window, voltage_window, label="Analog Signal", color='black')
    ax_waveform.set_ylabel("Voltage")
    ax_waveform.set_title(f"Analog Signal & Edges for Packet {packet_index + 1}")
    
    # Overlay edge markers for this packet.
    for t, level in packet_edges:
        if level.lower() == "low":
            col = 'blue'
        elif level.lower() == "high":
            col = 'red'
        elif level.lower() == "floating":
            col = 'green'
        else:
            col = 'gray'
        ax_waveform.axvline(x=t, color=col, linestyle="--", alpha=0.7)
        ax_waveform.text(t, np.max(voltage_window), level, rotation=90,
                         verticalalignment='bottom', fontsize=8, color=col)
    
    ax_waveform.grid(True)
    ax_waveform.legend()
    
    # Bottom subplot: display the decoded TX/RX binary data.
    ax_text.axis("off")  # Turn off axis lines/ticks.
    text_str = f"Packet {packet_index + 1}:\n  TX: {tx_data}\n  RX: {rx_data}"
    ax_text.text(0.01, 0.5, text_str, fontsize=14, verticalalignment="center",
                 transform=ax_text.transAxes)
    ax_text.set_title("Decoded Binary Data (TX / RX)")
    
    plt.xlabel("Time")
    plt.tight_layout()
    plt.show()

if(0):
    process_rs485_signal('controller-firmware/python/src/sandbox/analog.csv',
                        'controller-firmware/python/src/sandbox/fanuc_rs485_detected_edges.csv',
                        edge_threshold=0.5,   # adjust as needed
                        low_threshold=1.6,      # adjust as needed
                        high_threshold=2.5)     # adjust as needed


# =============================================================================
# Example usage:
#
# decoded_packets = decode_rs485_packets(
#     edges_csv = 'controller-firmware/python/src/sandbox/fanuc_rs485_detected_edges.csv',
#     packet_gap_threshold = 50e-6,  # Adjust this threshold (in seconds) based on your capture
#     bit_interval = 1/2.72727272e6          # Adjust the bit_interval (in seconds) to your protocol's timing
# )

# for idx, (tx, rx) in enumerate(decoded_packets):
#     print(f"Packet {idx}: TX = {tx}, RX = {rx}")
# =============================================================================


# =============================================================================
# Example usage:
#
# Assuming you have:
#   - 'analog_capture.csv' with columns "time", "voltage"
#   - 'detected_edges.csv' with columns "time", "level"
#   - decoded_packets: a list of (tx_data, rx_data) tuples obtained from your decoder
#
decoded_packets = decode_rs485_packets(
    edges_csv = 'controller-firmware/python/src/sandbox/fanuc_rs485_detected_edges.csv',
    packet_gap_threshold = 50e-6,
    bit_interval = 1/2.7272727e6
)

# remove empty packets
decoded_packets = [p for p in decoded_packets if p[0] and p[1]]

# ensure packet is correct length by repeating the last bit as needed
for idx, (tx, rx) in enumerate(decoded_packets):
    if len(rx) == 0:
        continue
    
    l = 101
    if len(rx) < l:
        rx += rx[-1] * (l - len(rx))

    decoded_packets[idx] = (tx, rx)

for idx, (tx, rx) in enumerate(decoded_packets):
    print(f"Packet {idx}: TX = {tx}, RX = {rx}")

    # visualize_single_packet('controller-firmware/python/src/sandbox/analog.csv',
    #                         'controller-firmware/python/src/sandbox/fanuc_rs485_detected_edges.csv',
    #                         decoded_packets,
    #                         packet_index=888,
    #                         packet_gap_threshold=50e-6)
# =============================================================================




