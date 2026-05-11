# Simulation Dockerfile
FROM flychams-base

# Clone FlyChams-Cosys-AirSim (flychams branch)
RUN git clone --branch flychams --depth 1 \
        https://github.com/JoseLopez36/FlyChams-Cosys-AirSim.git \
        /home/${USER_NAME}/airsim && \
    mkdir -p /home/${USER_NAME}/ros2_ws/src && \
    cp -r /home/${USER_NAME}/airsim/ros2/src/airsim_interfaces /home/${USER_NAME}/ros2_ws/src/airsim_interfaces && \
    cp -r /home/${USER_NAME}/airsim/ros2/src/airsim_wrapper   /home/${USER_NAME}/ros2_ws/src/airsim_wrapper

# Build airsim_interfaces and airsim_wrapper
RUN . /opt/ros/humble/setup.sh && \
    colcon build \
        --base-paths /home/${USER_NAME}/ros2_ws/src \
        --build-base /home/${USER_NAME}/ros2_ws/build \
        --install-base /home/${USER_NAME}/ros2_ws/install \
        --cmake-args -DCMAKE_BUILD_TYPE=Release && \
    echo ". /home/${USER_NAME}/ros2_ws/install/setup.bash" >> /home/${USER_NAME}/.bashrc