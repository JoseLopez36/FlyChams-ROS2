# Agent Dockerfile
FROM flychams-base

# GPU vendor: nvidia | amd
ARG GPU_VENDOR=nvidia
ENV GPU_VENDOR=${GPU_VENDOR}

# Setup Python dependencies
RUN sudo apt-get update && sudo apt-get install -y \
    python3-pip python3-dev python3-numpy \
    && sudo rm -rf /var/lib/apt/lists/* && sudo apt-get clean

# Setup common GStreamer dependencies
RUN sudo apt-get update && sudo apt-get install -y \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    && sudo rm -rf /var/lib/apt/lists/* && sudo apt-get clean

# Setup GPU-specific GStreamer dependencies
RUN if [ "$GPU_VENDOR" = "nvidia" ]; then \
        sudo apt-get update && sudo apt-get install -y \
            gstreamer1.0-plugins-bad \
        && sudo rm -rf /var/lib/apt/lists/* && sudo apt-get clean; \
    elif [ "$GPU_VENDOR" = "amd" ]; then \
        sudo apt-get update && sudo apt-get install -y \
            gstreamer1.0-vaapi \
            libva-dev libva-drm2 libva-x11-2 \
            mesa-va-drivers \
        && sudo rm -rf /var/lib/apt/lists/* && sudo apt-get clean; \
    fi

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

# Setup px4_msgs
RUN mkdir -p /home/${USER_NAME}/px4_msgs_ws/src && \
    git clone --branch release/1.16 https://github.com/PX4/px4_msgs.git \
        /home/${USER_NAME}/px4_msgs_ws/src/px4_msgs && \
    bash -c "source /opt/ros/humble/setup.bash && \
        cd /home/${USER_NAME}/px4_msgs_ws && \
        colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release" && \
    echo "source /home/${USER_NAME}/px4_msgs_ws/install/setup.bash" >> /home/${USER_NAME}/.bashrc