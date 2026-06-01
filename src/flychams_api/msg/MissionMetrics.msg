std_msgs/Header header

# Overall system metrics
int32 total_agents                   # Total number of agents in the system
int32 total_targets                  # Total number of targets being tracked
int32 total_clusters                 # Total number of active clusters

# General metrics
float32 time                         # Total elapsed mission time (s)

# Performance metrics
# CPU usage
float32 cpu_instant                  # Instant CPU usage (%)
float32 cpu_mean                     # Mean CPU usage over mission (%)
float32 cpu_max                      # Peak CPU usage over mission (%)
# GPU usage
float32 gpu_instant                  # Instant GPU utilisation (%)
float32 gpu_mean                     # Mean GPU utilisation over mission (%)
float32 gpu_max                      # Peak GPU utilisation over mission (%)
# RAM usage
float32 ram_instant                  # Instant RAM used (GB)
float32 ram_mean                     # Mean RAM used over mission (GB)
float32 ram_max                      # Peak RAM used over mission (GB)
# VRAM usage
float32 vram_instant                 # Instant VRAM used (GB)
float32 vram_mean                    # Mean VRAM used over mission (GB)
float32 vram_max                     # Peak VRAM used over mission (GB)