# Agent Dockerfile
FROM flychams-gpu

# GPU vendor forwarded from flychams-gpu
ARG GPU_VENDOR=nvidia
ENV GPU_VENDOR=${GPU_VENDOR}

# Setup PyTorch and Ultralytics
# RUN if [ "$GPU_VENDOR" = "nvidia" ]; then \
#         pip3 install --no-cache-dir ultralytics lapx; \
#     elif [ "$GPU_VENDOR" = "amd" ]; then \
#         pip3 install --no-cache-dir \
#             torch torchvision \
#             --index-url https://download.pytorch.org/whl/rocm6.2 && \
#         pip3 install --no-cache-dir ultralytics lapx; \
#     else \
#         pip3 install --no-cache-dir ultralytics lapx; \
#     fi

# Setup PX4 dependencies
RUN mkdir -p /home/${USER_NAME}/px4_msgs_ws/src && \
    git clone --branch release/1.16 https://github.com/PX4/px4_msgs.git \
        /home/${USER_NAME}/px4_msgs_ws/src/px4_msgs && \
    git clone --branch main https://github.com/PX4/px4_ros_com.git \
        /home/${USER_NAME}/px4_msgs_ws/src/px4_ros_com && \
    bash -c "source /opt/ros/humble/setup.bash && \
        cd /home/${USER_NAME}/px4_msgs_ws && \
        colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release" && \
    echo "source /home/${USER_NAME}/px4_msgs_ws/install/setup.bash" >> /home/${USER_NAME}/.bashrc