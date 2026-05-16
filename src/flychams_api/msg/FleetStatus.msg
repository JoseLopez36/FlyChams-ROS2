std_msgs/Header header
uint8 status

# Additional status flags
bool all_agents_idle
bool all_agents_active
bool any_agent_error
string[] agent_ids
uint8[] agent_statuses

# Fleet state values
# IDLE: 0    All agents in IDLE
# ACTIVE: 1  All agents in ACTIVE
# MIXED: 2   Agents in mixed states
# ERROR: 3   Any agent in ERROR