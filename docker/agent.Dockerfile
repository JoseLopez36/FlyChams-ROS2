# Agent Dockerfile
FROM flychams-base

# Setup agent-specific dependencies
RUN sudo apt-get update && \
    sudo apt-get install -y \
    ffmpeg \
    && sudo rm -rf /var/lib/apt/lists/* && \
    sudo apt-get clean

# Build and install px4_msgs from source
RUN mkdir -p /home/testuser/px4_msgs_ws/src && \
    git clone --branch release/1.16 https://github.com/PX4/px4_msgs.git \
        /home/testuser/px4_msgs_ws/src/px4_msgs && \
    bash -c "source /opt/ros/humble/setup.bash && \
        cd /home/testuser/px4_msgs_ws && \
        colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release" && \
    echo "source /home/testuser/px4_msgs_ws/install/setup.bash" >> /home/testuser/.bashrc

# Setup Ultralytics
# RUN sudo apt-get update && sudo apt-get install -y python3-pip && \
#     pip3 install ultralytics lapx && \
#     sudo rm -rf /var/lib/apt/lists/*