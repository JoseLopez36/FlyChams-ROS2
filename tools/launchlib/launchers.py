#!/usr/bin/env python3
"""
Container launcher classes
"""

import shlex

from .types import Environment, AgentSSH
from .docker import DockerContainer

class ContainerLauncher:
    """Base class for docker-based launchers"""

    def __init__(self, env: Environment, container: DockerContainer, mount_docker_sock: bool = False):
        self.env = env
        self.docker = container

        self.mount_docker_sock = mount_docker_sock

        # Set docker environment
        self.docker.set_env(self.get_base_docker_env())

    def setup(self, cmd):
        # Get base volumes
        base_volumes = self.get_base_volumes()

        # Get run shell command
        shell = self.docker.get_run_shell(base_volumes, cmd)

        # Convert shell command to string
        shell_str = " ".join(shlex.quote(str(arg)) for arg in shell)

        return shell_str

    def run(self, cmd):
        # Get exec shell command
        shell = self.docker.get_exec_shell(cmd)

        # Convert shell command to string
        shell_str = " ".join(shlex.quote(str(arg)) for arg in shell)

        return shell_str

    def get_base_docker_env(self):
        username = self.env.docker_user_name
        return {
            "USER_NAME": username,
            "DISPLAY": self.env.display,
            "QT_X11_NO_MITSHM": "1",
            "ROS_DOMAIN_ID": self.env.ros_domain_id,
            "FASTDDS_BUILTIN_TRANSPORTS": "UDPv4",
            "FLYCHAMS_PATH": f"/home/{username}/FlyChams-ROS2",
            "PX4_PATH": f"/home/{username}/PX4-Autopilot",
            "AIRSIM_PATH": f"/home/{username}/FlyChams-Cosys-AirSim"
        }

    def get_base_volumes(self):
        username = self.env.docker_user_name
        volumes = {
            "/tmp/.X11-unix": "/tmp/.X11-unix",
            self.env.flychams_ros2_path: f"/home/{username}/FlyChams-ROS2",
            self.env.flychams_px4_path: f"/home/{username}/PX4-Autopilot",
            self.env.flychams_airsim_path: f"/home/{username}/FlyChams-Cosys-AirSim"
        }
        if self.mount_docker_sock:
            volumes["/var/run/docker.sock"] = "/var/run/docker.sock"
        return volumes

class AgentContainerLauncher():
    """Base class for docker-based agent launchers"""

    def __init__(self, agent_id, env: Environment, container: DockerContainer):
        self.agent_id = agent_id

        self.env = env
        self.docker = container

        # Set docker environment
        self.docker.set_env(self.get_base_docker_env())

    def setup(self, cmd):
        # Get base volumes
        base_volumes = self.get_base_volumes()

        # Get run shell command
        shell = self.docker.get_run_shell(base_volumes, cmd)

        # Convert shell command to string
        shell_str = " ".join(shlex.quote(str(arg)) for arg in shell)

        return shell_str

    def run(self, cmd):
        # Get exec shell command
        shell = self.docker.get_exec_shell(cmd)

        # Convert shell command to string
        shell_str = " ".join(shlex.quote(str(arg)) for arg in shell)

        return shell_str

    def get_base_docker_env(self):
        username = self.env.docker_user_name
        return {
            "USER_NAME": username,
            "DISPLAY": self.env.display,
            "QT_X11_NO_MITSHM": "1",
            "ROS_DOMAIN_ID": self.env.ros_domain_id,
            "FASTDDS_BUILTIN_TRANSPORTS": "UDPv4",
            "FLYCHAMS_PATH": f"/home/{username}/FlyChams-ROS2",
            "PX4_PATH": f"/home/{username}/PX4-Autopilot",
            "AIRSIM_PATH": f"/home/{username}/FlyChams-Cosys-AirSim",
            "AGENT_ID": self.agent_id
        }

    def get_base_volumes(self):
        username = self.env.docker_user_name
        return {
            "/tmp/.X11-unix": "/tmp/.X11-unix",
            self.env.flychams_ros2_path: f"/home/{username}/FlyChams-ROS2",
            self.env.flychams_px4_path: f"/home/{username}/PX4-Autopilot",
            self.env.flychams_airsim_path: f"/home/{username}/FlyChams-Cosys-AirSim"
        }

class AgentRemoteLauncher():
    """Base class for remote agent launchers"""

    def __init__(self, agent_id, ssh: AgentSSH):
        self.agent_id = agent_id

        self.ssh = ssh

    def setup(self, cmd):
        username = self.ssh.user

        # Get run shell command
        shell = f"ssh {username}@{self.ssh.hostname} '{cmd}'"

        return shell

    def run(self, cmd):
        username = self.ssh.user

        # Get exec shell command
        shell = f"ssh {username}@{self.ssh.hostname} '{cmd}'"

        return shell

class PX4ContainerLauncher():
    """Class for PX4 SITL container launcher"""

    def __init__(self, agent_index, env: Environment):
        self.agent_index = agent_index

        self.env = env

    def setup(self, repo_path):
        px4_path = self.env.flychams_px4_path

        # Build PX4 SITL command
        px4_cmd = (
            f"PX4_SIM_HOSTNAME=172.17.0.1 PX4_SIM_MODEL=iris "
            f"{px4_path}/build/px4_sitl_default/bin/px4 "
            f"-i {self.agent_index} "
            f"-d {px4_path}/ROMFS/px4fmu_common "
            f"-s etc/init.d-posix/rcS"
        )

        # Get shell command
        return f"{repo_path}/tools/shell/docker_run_px4.sh '{px4_path}' 'flychams-PX4-{self.agent_index}' '{px4_cmd}'"