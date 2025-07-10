"""
Visualizer for a NURBS Curve with Variable Weights and Zero Endpoint Velocity/Acceleration

This script creates a degree-4 NURBS curve in 2D with variable weights.
The first three control points and the last three control points are repeated 
(i.e. identical after dehomogenization) to force both the velocity and acceleration 
to be zero at u=0 and u=1. The interior control points have variable weights.

The script then samples the curve and its first three derivatives 
(velocity, acceleration, and jerk) to verify continuity and the endpoint conditions.
"""

import numpy as np
import matplotlib.pyplot as plt

# Import the NURBS curve class and knot vector generator from geomdl
from geomdl import NURBS, knotvector
import cv2

# ----------------------------------------------
# 1. Define control points (with variable weights)
# ----------------------------------------------
#
# In a NURBS curve, the actual (Cartesian) point is computed by dehomogenizing the
# homogeneous control point [x, y, w] as [x/w, y/w]. To force the derivative and 
# acceleration to vanish at the endpoints, we set the first three and last three control
# points (after dehomogenization) to be identical.
#
# Here we define a set of control points in homogeneous coordinates.
#   - The first three are identical (to enforce zero velocity and acceleration at u=0).
#   - The interior points have variable weights.
#   - The last three are identical (to enforce zero velocity and acceleration at u=1).
ctrlptsw = [
    [0.0, 0.0, 1.0],  # P0
    #[0.0, 0.0, 1.0],  # P1 (repeated)
    #[0.0, 0.0, 1.0],  # P2 (repeated)
    [2.0, 3.0, 1.5],  # P3 (interior; weight 1.5)
    [2.0, 3.0, 1.2],  # P4 (interior; weight 1.2)
    [1.0*3, 0.0, 3.0],  # P5 (interior; weight 1.0)
    [8.0, 0.0, 0.8],  # P6
    [18.0, 0.0, 0.8],  # P6
    #[8.0, 0.0, 0.8],  # P7 (repeated)
    #[8.0, 0.0, 0.8]   # P8 (repeated)
]

def calc_time_scaling(vel, acc, jerk, max_vel, max_acc, max_jerk):
    time_scaling = 1.0
    if max_vel > 0:
        time_scaling = min(time_scaling, max_vel / vel)
    if max_acc > 0:
        time_scaling = min(time_scaling, np.power(max_acc / acc, 1/2))
    if max_jerk > 0:
        time_scaling = min(time_scaling, np.power(max_jerk / jerk, 1/3))
    return time_scaling

# ----------------------------------------------
# 2. Create and configure the NURBS curve
# ----------------------------------------------
curve = NURBS.Curve()        # Create a NURBS curve instance
curve.degree = 4             # Degree 4 yields a C³ continuous curve (continuous jerk)
curve.ctrlptsw = ctrlptsw    # Set the homogeneous control points
# Generate an open knot vector (this will automatically clamp the endpoints)
curve.knotvector = knotvector.generate(curve.degree, len(ctrlptsw))

# ----------------------------------------------
# 3. Sample the curve and its derivatives
# ----------------------------------------------
num_samples = 1000
u_vals = np.linspace(0, 1, num_samples)

positions = []     # Curve positions
velocities = []    # First derivative
accelerations = [] # Second derivative
jerks = []         # Third derivative

def compute_from_times(times: list, fixed_time_step: float):
    """
    Compute the derivatives of the curve if the input times were played back at a fixed time step.
    """

    positions = []
    velocities = []
    accelerations = []
    jerks = []
    previous_pos = curve.derivatives(times[0], order=0)[0]
    previous_vel = [0.0, 0.0]
    previous_acc = [0.0, 0.0]

    for time in times:
        pos = curve.derivatives(min(time, 1), order=0)[0]
        positions.append(pos)   # positions do not have any scaling effects
        vel = [pos[0] - previous_pos[0], pos[1] - previous_pos[1]]
        vel[0] /= fixed_time_step
        vel[1] /= fixed_time_step
        acc = [vel[0] - previous_vel[0], vel[1] - previous_vel[1]]
        acc[0] /= fixed_time_step
        acc[1] /= fixed_time_step
        jerk = [acc[0] - previous_acc[0], acc[1] - previous_acc[1]]
        jerk[0] /= fixed_time_step
        jerk[1] /= fixed_time_step
        velocities.append(vel)
        accelerations.append(acc)
        jerks.append(jerk)
        previous_pos = pos
        previous_vel = vel
        previous_acc = acc

    return positions, velocities, accelerations, jerks


# for u in u_vals:
#     derivs = curve.derivatives(u, order=3)
#     positions.append(derivs[0])
#     velocities.append(derivs[1])
#     accelerations.append(derivs[2])
#     jerks.append(derivs[3])


target_vel = 200
target_acc = 5000
target_jerk = 0

time = 0
base_time = .0001
scaling = 1.0
prev_scaling = 1.0

tol = 0.001
times = [0.0]
prev_vel = [0.0, 0.0]
prev_acc = [0.0, 0.0]

vel_run_complete = False
acc_run_complete = False
jerk_run_complete = False

while time < 1:

    print(f"Current time: {time:.4f}, Scaling: {scaling:.4f}")

    #scaling = 1.0

    run_complete = False

    cycles = 0

    while 1:
        derivs = curve.derivatives(min(time+(base_time*scaling), 1.0), order=3)
        vel = np.linalg.norm(derivs[1])
        acc = [derivs[1][0]*scaling - prev_vel[0], derivs[1][1]*scaling - prev_vel[1]]
        acc = [acc[0] / (base_time), acc[1] / (base_time)]
        

        jerk = [acc[0] - prev_acc[0], acc[1] - prev_acc[1]]
        jerk = [jerk[0] / (base_time), jerk[1] / (base_time)]
        
        acc_norm = np.linalg.norm(acc)
        jerk_norm = np.linalg.norm(jerk)

        vel *= scaling
        #acc *= scaling*scaling
        #jerk *= scaling*scaling*scaling

        # tol_reached = abs(vel-target_vel) <= tol or target_vel == 0
        # tol_reached &= abs(acc_norm-target_acc) <= tol or target_acc == 0 or acc_norm < target_acc
        # tol_reached &= abs(jerk_norm-target_jerk) <= tol or target_jerk == 0

        if target_vel == 0:
            vel_scaling = 0
        else:
            vel_scaling = vel / target_vel

        if target_acc == 0:
            acc_scaling = 0
        else:
            acc_scaling = np.power(acc_norm / target_acc, 1/3)
        
        if target_jerk == 0:
            jerk_scaling = 0
        else:
            jerk_scaling = np.power(jerk_norm / target_jerk, 1/3)

        scaling /= max(vel_scaling, acc_scaling, jerk_scaling)

        if 1:
            time += base_time*scaling
            positions.append(derivs[0])
            velocities.append([derivs[1][0] * scaling, derivs[1][1] * scaling])
            #accelerations.append([derivs[2][0] * scaling*scaling, derivs[2][1] * scaling*scaling])
            accelerations.append(acc)
            #jerks.append([derivs[3][0] * scaling*scaling*scaling, derivs[3][1] * scaling*scaling*scaling])
            jerks.append(jerk)
            vel_run_complete = False
            acc_run_complete = False
            jerk_run_complete = False
            break

        cycles += 1

    prev_vel = velocities[-1]
    prev_acc = accelerations[-1]

    times.append(time)

        


# first step through the curve at constant velocity (each step is the same length)
# time = 0
# step_size = 0.001    # distance between each point
# step_guess = 0.0001
# previous_point = curve.derivatives(0, order=0)[0]
# times = []
# while time < 1:

#     for i in range(10):
#         derivs = curve.derivatives(min(time+step_guess, 1.0), order=0)
#         step = (derivs[0][0] - previous_point[0])**2 + (derivs[0][1] - previous_point[1])**2
#         step = np.sqrt(step)

#         if abs(step - step_size) < step_size * 0.001:    # step is close to the desired step size
#             break

#         step_guess *= step_size / step

#     previous_point = derivs[0]

#     time += step_guess
#     times.append(time)

# positions, velocities, accelerations, jerks = compute_from_times(times, .001)

positions = np.array(positions)
velocities = np.array(velocities)
accelerations = np.array(accelerations)
jerks = np.array(jerks)

animation_positions = positions * 100
# Prepare an OpenCV animation window to visualize the motion using the positions array.
# Normalize positions to fit within the window.
padding = 50
min_x, max_x = np.min(animation_positions[:, 0]), np.max(animation_positions[:, 0])
min_y, max_y = np.min(animation_positions[:, 1]), np.max(animation_positions[:, 1])
width = int(max_x - min_x + 2 * padding)
height = int(max_y - min_y + 2 * padding)

# Scale positions to canvas coordinates (with y flipped, as OpenCV’s origin is top-left).
scaled_positions = []
for x, y in animation_positions:
    sx = int((x - min_x) / (max_x - min_x) * (width - 2 * padding) + padding)
    sy = int((max_y - y) / (max_y - min_y) * (height - 2 * padding) + padding)
    scaled_positions.append((sx, sy))

# Create a blank white canvas.
canvas = np.ones((height, width, 3), dtype=np.uint8) * 255

# Animate the movement by drawing the traveled path.
if 0:
    #for i, pt in enumerate(scaled_positions):
    for index in range(0, len(scaled_positions), 10):
        pt = scaled_positions[index]
        i = index
        frame = canvas.copy()
        if i > 0:
            # Draw the path up to the current point.
            for j in range(1, i + 1):
                cv2.line(frame, scaled_positions[j - 1], scaled_positions[j], (0, 0, 255), 2)
        # Mark the current position.
        cv2.circle(frame, pt, 5, (255, 0, 0), -1)
        cv2.imshow("Motion Animation", frame)
        if cv2.waitKey(20) == 27:  # Exit on pressing ESC key
            break

    cv2.waitKey(0)
    cv2.destroyAllWindows()


# Compute magnitudes of the derivatives (for plotting)
vel_mags = np.linalg.norm(velocities, axis=1)
acc_mags = np.linalg.norm(accelerations, axis=1)
jerk_mags = np.linalg.norm(jerks, axis=1)


# ----------------------------------------------
# 4. Visualization
# ----------------------------------------------
fig, axs = plt.subplots(5, 1, figsize=(10, 16))

# 4a. Plot the NURBS curve and its control polygon.
# Convert homogeneous control points to Cartesian by dehomogenizing.
ctrlpts_cart = []
for pt in ctrlptsw:
    # Each point is [x, y, w]. Compute [x/w, y/w].
    ctrlpts_cart.append([pt[0] / pt[2], pt[1] / pt[2]])
ctrlpts_cart = np.array(ctrlpts_cart)

axs[0].plot(positions[:, 0], positions[:, 1], 'b-', linewidth=2, label="NURBS Curve")
axs[0].plot(ctrlpts_cart[:, 0], ctrlpts_cart[:, 1], 'ro--', label="Control Polygon")
axs[0].set_title("NURBS Curve with Variable Weights\n(Zero Velocity & Acceleration at Endpoints)")
axs[0].set_xlabel("X")
axs[0].set_ylabel("Y")
axs[0].legend()
axs[0].grid(True)
axs[0].axis('equal')

u_vals = np.linspace(0, 1, len(velocities))

# 4b. Plot the velocity magnitude versus the parameter u.
axs[1].plot(u_vals, vel_mags, 'g-', linewidth=2)    # combined velocity
#axs[1].plot(u_vals, velocities[:, 0], 'r-', linewidth=1, label="X Velocity")
#axs[1].plot(u_vals, velocities[:, 1], 'b-', linewidth=1, label="Y Velocity")
axs[1].set_title("Velocity Magnitude vs. Parameter (u)")
axs[1].set_xlabel("u")
axs[1].set_ylabel("||Velocity||")
axs[1].grid(True)
axs[1].set_ylim([-target_vel*1.5, target_vel*1.5])

# 4c. Plot the acceleration magnitude versus the parameter u.
axs[2].plot(u_vals, acc_mags, 'm-', linewidth=2)
#axs[2].plot(u_vals, accelerations[:, 0], 'r-', linewidth=1, label="X Acceleration")
#axs[2].plot(u_vals, accelerations[:, 1], 'b-', linewidth=1, label="Y Acceleration")
axs[2].set_title("Acceleration Magnitude vs. Parameter (u)")
axs[2].set_xlabel("u")
axs[2].set_ylabel("||Acceleration||")
axs[2].grid(True)
axs[2].set_ylim([0, np.median(acc_mags)*2])

# 4d. Plot the jerk magnitude versus the parameter u.
axs[3].plot(u_vals, jerk_mags, 'c-', linewidth=2)
axs[3].set_title("Jerk Magnitude vs. Parameter (u)")
axs[3].set_xlabel("u")
axs[3].set_ylabel("||Jerk||")
axs[3].grid(True)
axs[3].set_ylim([0, np.median(jerk_mags)*2])

# 4e. Plot the time
times = np.array(times)
times = np.diff(times) / base_time  # compute the time between each point

calced_acc = np.diff(velocities, axis=0) / base_time
calced_acc = np.linalg.norm(calced_acc, axis=1)
#times = np.diff(times)  # compute the time between each point
axs[4].plot(times, 'c-', linewidth=2)
axs[4].set_title("Time vs. Index")
axs[4].set_xlabel("Index")
axs[4].set_ylabel("Time")
axs[4].grid(True)

plt.tight_layout()
plt.show()
