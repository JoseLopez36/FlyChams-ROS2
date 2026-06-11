# GPU Dockerfile - GStreamer + GPU acceleration
FROM flychams-base

# GPU vendor: nvidia | amd | jetson
ARG GPU_VENDOR=nvidia
ENV GPU_VENDOR=${GPU_VENDOR}

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
    elif [ "$GPU_VENDOR" = "jetson" ]; then \
        :; \
    fi