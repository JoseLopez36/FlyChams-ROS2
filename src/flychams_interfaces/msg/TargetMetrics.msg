std_msgs/Header header

# Target data
geometry_msgs/Point position

# Position and movement metrics
float32 distance_traveled        # Total distance covered by target (m)
float32 speed                    # Current speed of target (m/s)

# Estimation metrics
#float32 estimated_x             # TODO: Estimated x position (m)
#float32 estimated_y             # TODO: Estimated y position (m)

# Mission metrics
float32 time_elapsed             # Time elapsed since mission start (s)

# Performance metrics
float32 average_speed            # Average speed during mission (m/s)