std_msgs/Header header

# Overall fleet metrics
int32 total_agents                   # Total number of agents in the fleet

# Assignment data
int32 assignment_swap_count          # Total number of cluster-to-resource reassignments over the mission

# Problem-solving data
float32 assignment_solve_duration    # Time taken to compute the optimal assignment (ms)