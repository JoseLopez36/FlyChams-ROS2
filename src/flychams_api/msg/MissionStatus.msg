std_msgs/Header header
uint8 status

# Additional status flags
bool fleet_ready
float32 mission_time
string[] active_agents

# Status values
# READY: 0    Fleet can accept mission start
# ACTIVE: 1   Mission is running
# PAUSED: 2   Mission is paused, agents hovering
# ABORTED: 3  Mission aborted, agents landing