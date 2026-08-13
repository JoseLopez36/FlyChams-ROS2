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

# Zoom data
float32[] upsilons                   # Generalized zoom factor upsilon (focal for Camera, lambda*xi for Window) (m)
float32[] upsilons_norm              # Normalized zoom factor upsilon (dimensionless, [0, 1])
float32[] focals                     # Focal lengths for Camera (m)
float32[] lambdas                    # Resolution factors for Window (dimensionless)
float32[] xis                        # Geometric correction factors for Window (m)

# Spatial data
geometry_msgs/Vector3[] rotations    # Unit goal roll, pitch and yaw (rad) (only for Camera type)
flychams_api/Crop[] crops            # Unit goal crop or ROI (only for Window type)

# Apparent target sizes
float32[] apparent_target_sizes      # Projected cluster radius for each unit (px)