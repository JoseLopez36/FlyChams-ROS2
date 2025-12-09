#!/usr/bin/env python3
import os
import sys
import subprocess
import argparse
from pathlib import Path

def load_env_file(env_file):
    with open(env_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            if '=' in line:
                key, value = line.split('=', 1)
                value = os.path.expandvars(value)
                os.environ[key] = value

def setup_x11_auth():
    """Setup X11 authorization for Docker container access"""
    display = os.environ.get('DISPLAY', ':0')
    
    # Check if DISPLAY is set and X11 socket exists
    if not display:
        print("[WARNING] DISPLAY environment variable not set")
        return None
    
    x11_socket = '/tmp/.X11-unix/X0'
    if ':' in display:
        display_num = display.split(':')[1].split('.')[0]
        x11_socket = f'/tmp/.X11-unix/X{display_num}'
    
    if not os.path.exists(x11_socket):
        print(f"[WARNING] X11 socket not found at {x11_socket}")
        print("[WARNING] Make sure X11 is running and accessible")
    
    # Check if xhost is available and set up local access
    try:
        # Try to add local Docker access (this allows local connections)
        result = subprocess.run(['xhost', '+local:docker'], capture_output=True, text=True, check=False)
        if result.returncode == 0:
            print("[INFO] X11 access configured for Docker")
        else:
            print("[WARNING] Could not configure xhost automatically")
            print("[INFO] You may need to run manually: xhost +local:docker")
    except FileNotFoundError:
        print("[WARNING] xhost not found. X11 forwarding may not work.")
        print("[INFO] Install x11-xserver-utils: sudo apt install x11-xserver-utils")
    except Exception as e:
        print(f"[WARNING] Could not configure xhost: {e}")
    
    # Try to get XAUTHORITY file path
    xauth_file = os.environ.get('XAUTHORITY')
    if not xauth_file:
        # Try common XAUTHORITY locations
        home = os.path.expanduser('~')
        xauth_file = os.path.join(home, '.Xauthority')
        if not os.path.exists(xauth_file):
            xauth_file = None
    
    return xauth_file

def main():
    # Get arguments
    parser = argparse.ArgumentParser(description='Run rviz instance')
    parser.add_argument('--plotjuggler', action='store_true', help='Run PlotJuggler instead of RViz')
    args = parser.parse_args()

    # Get script directory
    script_dir = Path(__file__).resolve().parent

    # Get config paths
    root_dir = script_dir.parent.parent
    config_env_path = root_dir / 'docker' / 'config.env'
    
    # Load environment variables
    if config_env_path.exists():
        load_env_file(config_env_path)
    
    user_name = os.environ.get('USER_NAME')
    flychams_ros2_path = os.environ.get('FLYCHAMS_ROS2_PATH')
    flychams_airsim_path = os.environ.get('FLYCHAMS_AIRSIM_PATH')
    flychams_px4_path = os.environ.get('FLYCHAMS_PX4_PATH')

    # Setup X11 authorization
    xauth_file = setup_x11_auth()
    
    # Get plotjuggler argument
    plotjuggler_arg = "--plotjuggler" if args.plotjuggler else ""
    
    # Create command
    if args.plotjuggler:
        cmd = f"/home/{user_name}/FlyChams-ROS2/tools/run/run_plotjuggler.sh"
    else:
        cmd = f"/home/{user_name}/FlyChams-ROS2/tools/run/run_rviz.sh"
    
    # Get display variable
    display = os.environ.get('DISPLAY', ':0')
    
    docker_cmd = [
        'docker', 'run', '--rm', '-it',
        '--name', 'flychams-visualization',
        '--network', 'host',
        '--privileged',
        '-e', f"DISPLAY={display}",
        '-e', 'QT_X11_NO_MITSHM=1',
        '-e', f"FLYCHAMS_PATH=/home/{user_name}/FlyChams-ROS2",
        '-e', f"AIRSIM_PATH=/home/{user_name}/FlyChams-Cosys-AirSim",
        '-e', f"PX4_PATH=/home/{user_name}/PX4-Autopilot",
        '-v', '/tmp/.X11-unix:/tmp/.X11-unix:rw',
        '-v', f"{flychams_ros2_path}:/home/{user_name}/FlyChams-ROS2",
        '-v', f"{flychams_airsim_path}:/home/{user_name}/FlyChams-Cosys-AirSim",
        '-v', f"{flychams_px4_path}:/home/{user_name}/PX4-Autopilot",
    ]
    
    # Add XAUTHORITY volume if available
    if xauth_file and os.path.exists(xauth_file):
        docker_cmd.extend(['-v', f"{xauth_file}:{xauth_file}:ro"])
        docker_cmd.extend(['-e', f"XAUTHORITY={xauth_file}"])
    
    docker_cmd.extend([
        'flychams-ros2:latest',
        'bash', '-c',
        f"source /opt/ros/iron/setup.bash && source /home/{user_name}/FlyChams-ROS2/ros2_ws/install/setup.bash && {cmd}"
    ])
    
    try:
        subprocess.run(docker_cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"\n[ERROR] Failed to launch visualization tool. Exit code: {e.returncode}")
        print("\nTroubleshooting tips:")
        print("1. Make sure X11 is running: echo $DISPLAY")
        print("2. Allow X11 access for Docker: xhost +local:docker")
        print("3. If using SSH, enable X11 forwarding: ssh -X user@host")
        sys.exit(e.returncode)

if __name__ == '__main__':
    main()

