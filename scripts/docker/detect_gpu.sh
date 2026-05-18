#!/usr/bin/env bash
# Detect the primary GPU vendor and print one of: nvidia | amd

# NVIDIA: check for nvidia-smi or the kernel module
if command -v nvidia-smi &>/dev/null && nvidia-smi &>/dev/null; then
    echo "nvidia"
    exit 0
fi

if lsmod 2>/dev/null | grep -q "^nvidia "; then
    echo "nvidia"
    exit 0
fi

# AMD: check for amdgpu kernel module or /dev/kfd
if lsmod 2>/dev/null | grep -q "^amdgpu "; then
    echo "amd"
    exit 0
fi

if [ -e /dev/kfd ]; then
    echo "amd"
    exit 0
fi

echo "none"
exit 0