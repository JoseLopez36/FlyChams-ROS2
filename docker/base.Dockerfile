# Base Dockerfile - common ROS2 Humble foundation for all FlyChams services
FROM ros:humble-ros-base

# Setup base ROS2 Humble Packages
RUN apt-get update && \
    apt-get install -y \
    ros-$ROS_DISTRO-tf2-sensor-msgs \
    ros-$ROS_DISTRO-tf2-geometry-msgs \
    ros-$ROS_DISTRO-vision-opencv \
    ros-$ROS_DISTRO-geographic-msgs \
    ros-$ROS_DISTRO-compressed-image-transport \
    libyaml-cpp-dev \
    ros-$ROS_DISTRO-pcl-ros \
    ros-$ROS_DISTRO-pcl-conversions \
    libgeographic-dev \
    && rm -rf /var/lib/apt/lists/* && \
    apt-get clean

# Create a non-root user with sudo access
ARG USER_NAME=testuser
ENV USER_NAME=${USER_NAME}
RUN adduser --disabled-password --gecos '' $USER_NAME \
    && adduser $USER_NAME sudo \
    && echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Ensure the user owns their home directory
RUN chown -R $USER_NAME:$USER_NAME /home/$USER_NAME

# Automatically source ROS2 environment for the new user
RUN echo "source /opt/ros/$ROS_DISTRO/setup.bash" >> /home/$USER_NAME/.bashrc

# Switch to the non-root user
USER $USER_NAME
WORKDIR /home/${USER_NAME}