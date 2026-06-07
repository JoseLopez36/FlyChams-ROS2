std_msgs/Header header

# Agent data
geometry_msgs/Point position
geometry_msgs/Point setpoint

# Tracking data
float32[] upsilons_norm              # Normalized zoom factor upsilon for each unit (dimensionless, [0, 1])
float32[] apparent_target_sizes      # Projected cluster radius for each unit (px)

# Problem-solving data
float32 position_solve_duration      # Time taken to compute the optimal position (ms)

# Position and movement metrics
float32 distance_traveled            # Total distance covered by the UAV (m)
float32 speed                        # Current speed of the UAV (m/s)

# Goal-related metrics
float32 distance_to_goal             # Current distance to assigned goal (m)