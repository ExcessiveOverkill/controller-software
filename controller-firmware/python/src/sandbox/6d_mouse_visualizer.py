import serial
import struct
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits.mplot3d import Axes3D

GEAR_MOTOR_COUNT_PER_REV = 57.0/13.0 * 1000.0 * 4.0

# —— USER CONFIG ——
PORT            = 'COM11'
BAUDRATE        = 115200
COUNTS_PER_REV  = [GEAR_MOTOR_COUNT_PER_REV * 14, GEAR_MOTOR_COUNT_PER_REV * 11.56, GEAR_MOTOR_COUNT_PER_REV * 11.56, 2048, -2048, 2048]
HOME_POSITIONS   = [0, 0, 0, 0, 0, 0] # home positions for each joint in radians

DH_PARAMS = [                    # (alpha, a,    d,     theta_offset)
    (    0,   0.0,  0.3,       0),
    (-np.pi/2,0.0,  0.0, -np.pi/2),
    (    0,   0.210,  0.0,       0),
    (-np.pi/2,0.0,  0.210,       0),
    ( np.pi/2, 0.0, 0.0,       0),
    (-np.pi/2, 0.0, 0.1,       0),
]
HEADER = b'\xff'*16
# —————————————————

ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)

def read_packet():
    """Block until we see 16 0xFF bytes (header), then read 25 bytes."""
    
    # clear buffer
    ser.reset_input_buffer()
    buf = bytearray()
    while True:
        b = ser.read(1)
        if not b:
            return None
        buf += b
        if buf.endswith(HEADER):
            break
        # keep buf small
        if len(buf) > len(HEADER):
            buf = buf[-len(HEADER):]
    data = ser.read(4*6+1)
    if len(data) < 4*6+1:
        return None
    return struct.unpack('<6i', data[0:-1]), data[-1]

def dh_transform(alpha, a, d, theta):
    ca, sa = np.cos(alpha), np.sin(alpha)
    ct, st = np.cos(theta), np.sin(theta)
    return np.array([
        [  ct,    -st,    0,   a],
        [st*ca, ct*ca, -sa, -d*sa],
        [st*sa, ct*sa,  ca,  d*ca],
        [   0,      0,    0,    1],
    ])

def forward_kinematics(joints):
    """Return array of joint positions [[x,y,z],…] for base→flange→pen."""

    # correct for linkage setup
    joints[2] = joints[2] - joints[1]   # the angle between links is the difference of the two encoders

    T = np.eye(4)
    pts = [T[:3,3].copy()]
    for θ, (α, a, d, off) in zip(joints, DH_PARAMS):
        T = T @ dh_transform(α, a, d, θ + off)
        pts.append(T[:3,3].copy())
    return np.array(pts)

# Set up figure
fig = plt.figure()
ax  = fig.add_subplot(111, projection='3d')
line, = ax.plot([], [], [], 'o-', lw=2)
text_annotations = [
    ax.text2D(0.02, 0.95 - i*0.05, '', transform=ax.transAxes)
    for i in range(6)
]

ax.set_xlim(-1,1); ax.set_ylim(-1,1); ax.set_zlim(0,1.5)
ax.set_xlabel('X'); ax.set_ylabel('Y'); ax.set_zlabel('Z')
ax.set_title('Real-time 6-DOF Encoder Visualization')

def init():
    line.set_data([], [])
    line.set_3d_properties([])
    for txt in text_annotations:
        txt.set_text('')
    return (line, *text_annotations)

def update(frame):
    pkt, btn = read_packet()
    if pkt is None:
        return (line, *text_annotations)

    counts = np.array(pkt)
    print(counts, btn)
    # convert counts → radians
    angles = (counts / np.array(COUNTS_PER_REV)) * 2*np.pi

    # forward kinematics
    pts = forward_kinematics(angles)
    xs, ys, zs = pts.T
    line.set_data(xs, ys)
    line.set_3d_properties(zs)

    # update text
    for i, txt in enumerate(text_annotations):
        txt.set_text(f'J{i+1}: {counts[i]}')

    return (line, *text_annotations)

ani = FuncAnimation(fig, update, init_func=init,
                    interval=50, blit=True)
plt.show()
