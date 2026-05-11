# Simulation Dockerfile
FROM flychams-base

# Clone FlyChams-Cosys-AirSim (flychams branch)
RUN git clone --branch flychams --depth 1 \
        https://github.com/JoseLopez36/FlyChams-Cosys-AirSim.git \
        /home/${USER_NAME}/FlyChams-Cosys-AirSim

# Build AirSim C++ libraries (required by airsim_wrapper)
RUN cd /home/${USER_NAME}/FlyChams-Cosys-AirSim && \
    ./setup.sh && \
    ./build.sh

# Build airsim_interfaces and airsim_wrapper
RUN . /opt/ros/humble/setup.sh && \
    colcon build \
        --base-paths /home/${USER_NAME}/FlyChams-Cosys-AirSim/ros2/src \
        --build-base /home/${USER_NAME}/FlyChams-Cosys-AirSim/ros2/build \
        --install-base /home/${USER_NAME}/FlyChams-Cosys-AirSim/ros2/install \
        --cmake-args -DCMAKE_BUILD_TYPE=Release && \
    echo ". /home/${USER_NAME}/FlyChams-Cosys-AirSim/ros2/install/setup.bash" >> /home/${USER_NAME}/.bashrc