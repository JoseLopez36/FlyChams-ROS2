std_msgs/Header header

# Overall system metrics
int32 total_agents                  # Total number of agents in the system
int32 total_targets                 # Total number of targets being tracked
int32 total_clusters                # Total number of active clusters

# Coverage metrics
#float32 area_coverage               # TODO: Percentage of total area covered by agents
#float32 target_coverage             # TODO: Percentage of targets being actively tracked
#float32 cluster_coverage            # TODO: Percentage of clusters being monitored

# System performance
#float32 average_tracking_error      # TODO: Average tracking error across all agents
#float32 system_response_time        # TODO: Average time to respond to new targets
#float32 mission_completion          # TODO: Overall mission completion percentage

# Cluster statistics
#float32 average_cluster_size        # TODO: Average number of targets per cluster
#float32 largest_cluster_size        # TODO: Size of the largest cluster
#float32 cluster_stability           # TODO: Average stability across all clusters

# Target statistics
#float32 average_target_speed        # TODO: Average speed of all targets
#float32 target_dispersion           # TODO: Measure of how spread out targets are
#float32 targets_in_clusters         # TODO: Percentage of targets that are clustered

# Agent statistics
#float32 average_agent_speed         # TODO: Average speed across all agents
#float32 agent_workload_balance      # TODO: Measure of workload distribution (0-1)
#float32 average_battery_level       # TODO: Average battery level across all agents

# Mission metrics
float32 mission_time                 # Total elapsed mission time
#int32 total_waypoints_reached       # TODO: Total waypoints reached by all agents
#float32 overall_efficiency          # TODO: Global measure of system efficiency (0-1)