# Agent Dockerfile
FROM flychams-base

# Setup agent-specific dependencies
RUN sudo apt-get update && \
    sudo apt-get install -y \
    ffmpeg \
    && sudo rm -rf /var/lib/apt/lists/* && \
    sudo apt-get clean

# Setup agent-specific ROS2 Humble Packages
RUN sudo apt-get update && \
    sudo apt-get install -y \
    ros-$ROS_DISTRO-mavros* \
    && sudo rm -rf /var/lib/apt/lists/* && \
    sudo apt-get clean

# Install GeographicLib datasets for mavros
RUN wget https://raw.githubusercontent.com/mavlink/mavros/ros2/mavros/scripts/install_geographiclib_datasets.sh && \
    chmod +x install_geographiclib_datasets.sh && \
    sudo ./install_geographiclib_datasets.sh && \
    rm install_geographiclib_datasets.sh

# Setup Ultralytics
RUN sudo apt-get update && sudo apt-get install -y python3-pip && \
    pip3 install ultralytics lapx && \
    sudo rm -rf /var/lib/apt/lists/*