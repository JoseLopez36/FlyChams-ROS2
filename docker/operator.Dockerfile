# Operator Dockerfile
FROM flychams-base

# Install foxglove_bridge for Foxglove Studio connectivity
RUN sudo apt-get update && \
    sudo apt-get install -y \
    ros-$ROS_DISTRO-foxglove-bridge \
    ros-$ROS_DISTRO-rosbag2 \
    ros-$ROS_DISTRO-rosbag2-storage-mcap \
    && sudo rm -rf /var/lib/apt/lists/* && \
    sudo apt-get clean