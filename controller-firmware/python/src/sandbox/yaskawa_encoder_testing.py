import pyvisa
import numpy as np

# Connect to the instrument (update the resource string with your DG1022's IP)
rm = pyvisa.ResourceManager()
rm.list_resources()
awg = rm.open_resource('TCPIP0::192.168.1.208::INSTR')


sample_rate = 40e6  # 40 MSa/s

data = "0011001100110011001100110011001101001010101010110010101010110010101010110010101010110011001010101010110"

invert = False

bit_time = 100e-9

tx_points = ""
enable_points = ""

if invert:
    data = data.replace("0", "2")
    data = data.replace("1", "0")
    data = data.replace("2", "1")

for i in range(0, len(data)):
    if data[i] == '1':
        for j in range(0, int(sample_rate * bit_time)):
            tx_points = f"{tx_points}1.0,"
    else:
        for j in range(0, int(sample_rate * bit_time)):
            tx_points = f"{tx_points}-1.0,"

    for j in range(0, int(sample_rate * bit_time)):
        enable_points = f"{enable_points}1.0,"

for j in range(0, int(sample_rate * bit_time) * 4000):
    tx_points = f"{tx_points}-1.0,"
    enable_points = f"{enable_points}-1.0,"

#tx_waveform = np.array(points)

# Convert numpy arrays to comma-separated strings (the format expected by the DG1022)
#tx_str = ','.join(map(str, tx_waveform))
#tx_en_str = ','.join(map(str, tx_en_waveform))

awg.write("OUTP1 OFF")
awg.write("OUTP2 OFF")

awg.write("SOUR1:FUNC USER")
awg.write("SOUR1:VOLT:UNIT VPP")
awg.write("SOUR1:VOLT:HIGH 3.3")
awg.write("SOUR1:VOLT:LOW 0")
# awg.write("SOUR1:FREQ 10000")
awg.write("SOUR1:APPL:ARB")

awg.write("SOUR2:FUNC USER")
awg.write("SOUR2:VOLT:UNIT VPP")
awg.write("SOUR2:VOLT:HIGH 3.3")
awg.write("SOUR2:VOLT:LOW 0")
# awg.write("SOUR2:FREQ 10000")
awg.write("SOUR2:APPL:ARB")


# --- Configure Channel 1 for TX ---
awg.write(f"SOUR1:DATA VOLATILE,{tx_points}")
awg.write(f"SOUR2:DATA VOLATILE,{enable_points}")

# awg.write("DATA:DAC VOLATILE,8192,16383,8192,0")
awg.write("SOUR1:FUNC:USER VOLATILE")
awg.write("SOUR2:FUNC:USER VOLATILE")

# Set both channels to burst mode triggered by the same source
# awg.write("SOUR1:BURSt:STAT ON")
# awg.write("SOUR1:BURSt:MODE TRIG")
# awg.write("SOUR1:BURSt:SOUR INT")
# awg.write("SOUR2:BURSt:STAT ON")
# awg.write("SOUR2:BURSt:MODE TRIG")
# awg.write("SOUR2:BURSt:SOUR INT")

# --- Enable outputs ---
awg.write("OUTP1 ON")
awg.write("OUTP2 ON")

awg.write("TRIG:IMM")


# Optionally, configure triggering/synchronization if needed:
# awg.write("TRIG:SOUR IMM")  # Immediate trigger (or configure external trigger as required)

#awg.write("SOUR2:PHAS:ALIGN")  # Align the phases of both channels



