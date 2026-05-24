std_msgs/Header header

# Agent data
geometry_msgs/Point position
geometry_msgs/Point setpoint

# Tracking data
float32[] zoom_factors               # Associated tracking zoom factor for this cluster

# Problem-solving data
float32 position_solve_duration      # Time taken to compute the optimal position (ms)

# Position and movement metrics
float32 distance_traveled            # Total distance covered by the UAV (m)
float32 speed                        # Current speed of the UAV (m/s)

# Goal-related metrics
float32 distance_to_goal             # Current distance to assigned goal (m)