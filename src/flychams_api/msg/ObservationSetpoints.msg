std_msgs/Header header

# Number of units
int32 n_o                            # Number of observation units
int32 n_t                            # Number of tracking units
int32 n_c                            # Number of cameras
int32 n_w                            # Number of windows

# Unit identification
string[] ids                         # Unit IDs
uint8[] types                        # Unit types (0: None, 1: Camera, 2: Window)
uint8[] roles                        # Unit roles (0: None, 1: Central, 2: Tracking)

# Unit data
float32[] zoom_factors               # Unit zoom factor
float32[] generalized_zoom_factors   # Generalized zoom factor for each unit (0=min, 1=max)
geometry_msgs/Vector3[] rotations    # Unit goal roll, pitch and yaw (rad) (only for Camera type)
flychams_api/Crop[] crops            # Unit goal crop or ROI (only for Window type)
float32[] apparent_target_sizes      # Projected cluster radius for each unit (px)