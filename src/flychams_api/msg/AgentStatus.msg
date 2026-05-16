std_msgs/Header header
uint8 status

# Additional status flags
bool is_armed
bool is_flying

# Status values
# IDLE: 0    On ground, disarmed, safe
# ACTIVE: 1  Armed, flying, executing mission
# ERROR: 2   Fault condition, requires reset