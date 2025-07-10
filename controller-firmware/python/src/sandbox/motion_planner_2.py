import cv2
import numpy as np
import math

# -------------------------------
# Configuration / Input Variables
# -------------------------------

# Define the control points (x, y) on a 500x500 canvas.
points = np.array([
    [100, 300],
    [100, 100],
    [300, 300],
    [400, 200],
    [500, 300],
    [600, 200],
    [700, 300],
    [800, 200],
    [900, 300],
    [900, 100]
    
], dtype=np.float32)

tolerance = 20
velocity = 50
acceleration = 100


# Canvas dimensions
width, height = 1000, 1000

# -------------------------------
# Compute the Interpolated Curve
# -------------------------------

curve = []  # Will hold the (x,y) points of the curve

curve.append(points[0])

circles = []

min_radius = (velocity ** 2) / acceleration

for i in range(1, len(points) - 1):

    v1 = points[i - 1] - points[i]
    v2 = points[i + 1] - points[i]
    v1 = v1 / np.linalg.norm(v1)
    v2 = v2 / np.linalg.norm(v2)
    angle = math.acos(np.dot(v1, v2))

    # find the radius required to meet the tolerance
    radius = (tolerance * math.sin(angle / 2)) / (1 - math.sin(angle / 2))

    #radius = min(radius, min_radius)    # Ensure the radius is at least the minimum required to meet the velocity and acceleration constraints
    #radius = min_radius

    # find the center of the circle

    # resulting acceleration vector from v1 to v2
    a = v1 + v2

    # normalize the acceleration vector
    a = a / np.linalg.norm(a)

    # extend the acceleration vector to the center of the circle
    center = points[i] + a * (radius/math.sin(angle/2))

    circles.append((center, radius))
    



curve = np.array(curve, dtype=np.float32)

# -------------------------------
# Animation Loop with OpenCV
# -------------------------------

for i in range(len(curve)):
    # Create a blank black image.
    img = np.ones((height, width, 3), dtype=np.uint8)
    img *= 100  # Set all pixels to gray
    
    # Draw the points as small green circles.
    for pt in points:
        cv2.circle(img, (int(pt[0]), int(pt[1])), 5, (0, 255, 0), -1)
        cv2.circle(img, (int(pt[0]), int(pt[1])), tolerance, (0, 255, 0), 1)    # Draw a green circle around the point to indicate the max tolerance.

    # Draw the circles
    for center, radius in circles:
        cv2.circle(img, (int(center[0]), int(center[1])), int(radius), (0, 0, 255), 1)
    
    # Draw lines connecting the control points (the control polygon) in yellow.
    for j in range(1, len(points)):
        pt1 = tuple(points[j - 1].astype(int))
        pt2 = tuple(points[j].astype(int))
        cv2.line(img, pt1, pt2, (0, 255, 255), 1)
    
    # Draw the portion of the computed curve up to the current sample in blue.
    for j in range(1, i):
        pt1 = tuple(curve[j - 1].astype(int))
        pt2 = tuple(curve[j].astype(int))
        cv2.line(img, pt1, pt2, (255, 0, 0), 2)
    
    # Display the image.
    cv2.imshow("Curve Animation", img)
    
    # Wait for 10 ms; exit early if the ESC key (27) is pressed.
    if cv2.waitKey() & 0xFF == 27:
        break

cv2.destroyAllWindows()
