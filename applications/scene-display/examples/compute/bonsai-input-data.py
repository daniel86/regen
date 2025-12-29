# Generate a sequence of input data samples for Bonsai volume rendering.
# Each input is (1) ray origin, a point on sphere of radius 1 centered at (0,0,0),
# and (2) ray direction, a unit vector pointing into the sphere.
import numpy as np

# Assuming we only need a small network for modeling the function
# we might get away with 500k - 1M training samples.
NUM_SAMPLES = 500000
RADIUS = 1.0
ray_origins = []
ray_directions = []

def compute_sphere_point(radius):
	theta = np.arccos(1 - 2 * np.random.rand())  # polar angle
	phi = 2 * np.pi * np.random.rand()           # azimuthal angle
	x = radius * np.sin(theta) * np.cos(phi)
	y = radius * np.sin(theta) * np.sin(phi)
	z = radius * np.cos(theta)
	return np.array([x, y, z])

for _ in range(NUM_SAMPLES):
	# Generate a random point on the sphere surface for ray origin
	ray_origin = compute_sphere_point(RADIUS)
	ray_origins.append(ray_origin)

	# Generate a second random point on the sphere surface for ray direction
	ray_direction = compute_sphere_point(RADIUS) - ray_origin
	ray_direction /= np.linalg.norm(ray_direction)  # Normalize to unit vector
	ray_directions.append(ray_direction)

ray_origins = np.array(ray_origins, dtype=np.float32)
ray_directions = np.array(ray_directions, dtype=np.float32)

ray_origins.tofile("rayOrigin.bin")
ray_directions.tofile("rayDirection.bin")
