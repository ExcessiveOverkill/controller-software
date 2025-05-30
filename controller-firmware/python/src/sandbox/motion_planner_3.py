import cv2
import numpy as np
import math

time_step = 0.1
max_velocity = 800
max_acceleration = 40
max_jerk = 10


points = np.array([
    [100, 500],
    [900, 500]
    ], dtype=np.float32)


def line_2d_distance_to_time(start_point: np.array, end_point: np.array, distance: float) -> np.array:
    speed = 1.0
    
    line_length = np.linalg.norm(end_point - start_point)
    time = distance / speed
    time = time * (distance / line_length)
    return (distance / speed), line_length

def interpolate_line(start_point: np.array, end_point: np.array, time: float) -> np.array:
    return (1 - time) * start_point + time * end_point


line_length = line_2d_distance_to_time(points[0], points[1], 0.0)[1]

acc_time = 0
vel_time = 0


# check length to reach max velocity while accelerating
time_to_max_velocity = max_velocity / max_acceleration
distance_to_max_velocity = 0.5 * max_acceleration * (time_to_max_velocity ** 2)

# handle case where max velocity is reached before half the line length
if distance_to_max_velocity <= line_length/2:
    acc_time = time_to_max_velocity
    vel_time = (line_length - 2 * distance_to_max_velocity) / max_velocity
else:
    # handle case where max velocity is reached after half the line length
    
    # find the time to reach half the line length while accelerating
    time_to_half_length = math.sqrt((line_length/2) / (0.5 * max_acceleration))
    acc_time = time_to_half_length


distance = 0.0
n = 0

linear_points = []
time = 0.0
u = 0.0
while 1:
    if time < acc_time:
        distance = 0.5 * max_acceleration * (time ** 2)
        u = distance / line_length
    elif time < acc_time + vel_time:
        distance = 0.5 * max_acceleration * (acc_time ** 2)
        distance = max_velocity * (time - acc_time)
        u = distance / line_length
    else:
        distance = line_length - 0.5 * max_acceleration * ((acc_time - (time - acc_time - vel_time)) ** 2)
        u = distance / line_length
    
    pt = interpolate_line(points[0], points[1], u)
    linear_points.append(pt)

    time += time_step

    if time > acc_time + vel_time + acc_time:
        break


animation_positions = np.array(linear_points, dtype=np.int32)
# Prepare an OpenCV animation window to visualize the motion using the positions array.
# Normalize positions to fit within the window.

width = 1000
height = 1000

# Create a blank white canvas.
canvas = np.ones((height, width, 3), dtype=np.uint8) * 255

# Animate the movement by drawing the traveled path.
if 1:
    #for i, pt in enumerate(animation_positions):
    for index in range(0, len(animation_positions), 1):
        pt = animation_positions[index]
        i = index
        frame = canvas.copy()
        if i > 0:
            # Draw the path up to the current point.
            for j in range(1, i + 1):
                cv2.line(frame, animation_positions[j - 1], animation_positions[j], (0, 0, 255), 2)
        # Mark the current position.
        cv2.circle(frame, pt, 5, (255, 0, 0), -1)
        cv2.imshow("Motion Animation", frame)
        if cv2.waitKey(20) == 27:  # Exit on pressing ESC key
            break

    cv2.waitKey(0)
    cv2.destroyAllWindows()
