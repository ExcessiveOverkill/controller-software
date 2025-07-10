import cv2
import numpy as np
import math

tolerance = 20.0

def average_position(points: np.array, weights: np.array) -> np.array:
    return np.sum(points * weights[:, None], axis=0) / np.sum(weights)

def spline(t, control_points: np.array):
    
    degree = 3 # number of control points active per time 1

    segment = math.floor(t)

    # first find the center point between the outer control points
    center = (control_points[segment] + control_points[segment + 2]) / 2

    # find distance from center to middle control point
    distance = np.linalg.norm(control_points[segment + 1] - center)

    # find center weights to ensure middle tolerance is met
    center_weight = 2.0 * (distance/tolerance - 1.0)
    # outer control points have weight 1.0




# -------------------------------
# Configuration / Input Variables
# -------------------------------

# Define the control points (x, y) on a 500x500 canvas.
control_points = np.array([
    [100, 300],
    [200, 100],
    [300, 400],
    [400, 200]
], dtype=np.float32)

# Define a weight for each point. A higher weight pulls the curve closer to that point.
weights = np.array([1, 5, 1, 1], dtype=np.float32)

# Number of samples along the curve for drawing
num_samples = 1000

# Canvas dimensions
width, height = 500, 500

# -------------------------------
# Compute the Interpolated Curve
# -------------------------------

curve = []  # Will hold the (x,y) points of the curve

if len(control_points) < 3:
    # If only two points, do simple linear interpolation.
    for i in range(num_samples):
        t = i / (num_samples - 1)
        pt = (1 - t) * control_points[0] + t * control_points[1]
        curve.append(pt)
else:
    # For three or more control points, use the weighted Bézier curve.
    for i in range(num_samples):
        t = i / (num_samples - 1)
        pt = rational_bezier(t, control_points, weights)
        curve.append(pt)

curve = np.array(curve, dtype=np.float32)

# -------------------------------
# Animation Loop with OpenCV
# -------------------------------

for i in range(1, len(curve)):
    # Create a blank black image.
    img = np.ones((height, width, 3), dtype=np.uint8)
    img *= 100  # Set all pixels to gray
    
    # Draw the control points as small green circles.
    for pt in control_points:
        cv2.circle(img, (int(pt[0]), int(pt[1])), 5, (0, 255, 0), -1)
    
    # Draw lines connecting the control points (the control polygon) in yellow.
    for j in range(1, len(control_points)):
        pt1 = tuple(control_points[j - 1].astype(int))
        pt2 = tuple(control_points[j].astype(int))
        cv2.line(img, pt1, pt2, (0, 255, 255), 1)
    
    # Draw the portion of the computed curve up to the current sample in blue.
    for j in range(1, i):
        pt1 = tuple(curve[j - 1].astype(int))
        pt2 = tuple(curve[j].astype(int))
        cv2.line(img, pt1, pt2, (255, 0, 0), 2)
    
    # Display the image.
    cv2.imshow("Weighted Bézier Curve Animation", img)
    
    # Wait for 10 ms; exit early if the ESC key (27) is pressed.
    if cv2.waitKey(10) & 0xFF == 27:
        break

cv2.destroyAllWindows()
