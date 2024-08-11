import numpy as np
import matplotlib.pyplot as plt

encoderCountReal = 0.0
encoderCountInteger = 0

ENCODER_COUNT = 2**16

filterData = [0, 0]

PRESCALER = 1 * 65536
INPUT_ROUNDING = 16
OUTPUT_ROUNDING = 16
A1 = int(-0.04515176221313341 *65536)
A2 = int(-0.9404614587008322 *65536)

B0 = int(0.003596694771508549 *65536)
B1 = int(0.007193389543017098 *65536)
B2 = int(0.003596694771508549 *65536)

times = []
filteredPos = []
realEncoder = []
integerEncoder = []

for i in range(4000):

    # update encoder
    encoderCountReal += .001
    encoderCountInteger = round(encoderCountReal, 0)
    if(encoderCountInteger > ENCODER_COUNT-1):
        encoderCountInteger -= ENCODER_COUNT
        encoderCountReal -= ENCODER_COUNT
    elif(encoderCountInteger < 0):
        encoderCountInteger += ENCODER_COUNT
        encoderCountReal += ENCODER_COUNT

    if(encoderCountInteger > 0):
        print("true")

    if(i == 200):
        encoderCountReal += ENCODER_COUNT -50

    temp = int(encoderCountInteger * PRESCALER)
    temp -= int(filterData[0] * A1)
    temp -= int(filterData[1] * A2)

    dn = round(temp / pow(2, INPUT_ROUNDING), 0)

    
    temp2 = int(dn * B0)
    temp2 += int(filterData[0] * B1)
    temp2 += int(filterData[1] * B2)
    

    yn = round(temp2 / pow(2, OUTPUT_ROUNDING), 0)

    filterData.insert(0, dn)
    filterData.pop(-1)

    print(filterData)

    times.append(i)
    filteredPos.append(yn)
    realEncoder.append(encoderCountReal)
    integerEncoder.append(encoderCountInteger)


plt.plot(times, filteredPos)
plt.plot(times, realEncoder)
plt.plot(times, integerEncoder)
plt.show()


# import numpy as np

# class BiquadFilter:
#     def __init__(self, b0, b1, b2, a1, a2):
#         # Initialize coefficients
#         self.b0, self.b1, self.b2 = b0, b1, b2
#         self.a1, self.a2 = a1, a2
        
#         # Initialize state variables
#         self.x1, self.x2 = 0.0, 0.0
#         self.y1, self.y2 = 0.0, 0.0

#     def process(self, x):
#         # Direct Form II filter implementation
#         y = self.b0 * x + self.b1 * self.x1 + self.b2 * self.x2 - self.a1 * self.y1 - self.a2 * self.y2
        
#         # Update state
#         self.x2 = self.x1
#         self.x1 = x
#         self.y2 = self.y1
#         self.y1 = y
        
#         return y

# # Example usage
# if __name__ == "__main__":
#     # Define filter coefficients (example values)
#     b0, b1, b2 = 5.074331248486237e-9, 1.0148662496972475e-8, 5.074331248486237e-9
#     a1, a2 = -1.999798506779029, 0.9997985270763541
    
#     # Create a BiquadFilter object
#     filter = BiquadFilter(b0, b1, b2, a1, a2)
    
#     # Example input signal (simple sinusoidal signal)
#     fs = 44100  # Sampling frequency
#     t = np.linspace(0, 1, fs, endpoint=False)  # Time vector
#     input_signal = np.sin(2 * np.pi * 50 * t)  # 5 Hz sine wave

#     # Process the signal through the filter
#     output_signal = np.array([filter.process(x) for x in input_signal])
    
#     # You can use matplotlib to visualize the input and output signals
#     import matplotlib.pyplot as plt
#     plt.figure()
#     plt.subplot(2, 1, 1)
#     plt.title('Input Signal')
#     plt.plot(t, input_signal)
#     plt.subplot(2, 1, 2)
#     plt.title('Output Signal')
#     plt.plot(t, output_signal)
#     plt.show()

