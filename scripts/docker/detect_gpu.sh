#!/usr/bin/env bash
# Detect the primary GPU vendor and print one of: nvidia | amd | intel | none

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

# Intel: check for i915 kernel module or Intel DRI device
if lsmod 2>/dev/null | grep -q "^i915 "; then
    echo "intel"
    exit 0
fi

if ls /dev/dri/renderD* &>/dev/null; then
    if grep -qi "intel" /sys/class/drm/*/device/vendor 2>/dev/null ||
       lspci 2>/dev/null | grep -qi "intel.*graphics\|intel.*display"; then
        echo "intel"
        exit 0
    fi
fi

echo "none"
exit 0