# Agent Dockerfile
FROM flychams-base

# Setup agent-specific dependencies
RUN sudo apt-get update && \
    sudo apt-get install -y \
    libgstreamer1.0-0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-x \
    gstreamer1.0-alsa \
    gstreamer1.0-gl \
    gstreamer1.0-gtk3 \
    gstreamer1.0-qt5 \
    gstreamer1.0-pulseaudio \
    libgstreamer-plugins-base1.0-dev \
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