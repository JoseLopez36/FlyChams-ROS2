#!/usr/bin/env python3
"""
Docker container class
"""

import subprocess
from typing import Dict

class DockerContainer:
    """Class that gathers all Docker-related functionalities"""

    def __init__(self, image, name):
        self.image = image
        self.name = name
        self.env: Dict[str, str] = {}

    def set_env(self, env: Dict[str, str]):
        self.env = env

    def setup_auth(self):
        """
        X11 authorization setup for Docker GUI tools
        """
        subprocess.run(["xhost", "+local:docker"])

    def get_env(self):
        return self.env

    def get_run_shell(self, volumes, cmd):
        """Generate shell command string for docker run"""
        shell: list[str] = ["docker", "run"]
        shell.append("--rm")
        shell.append("-it")
        shell += ["--name", self.name]
        shell += ["--network", "host"]
        shell.append("--privileged")

        for k, v in self.env.items():
            shell += ["-e", f"{k}={v}"]
        for host_path, container_path in volumes.items():
            shell += ["-v", f"{host_path}:{container_path}"]

        shell.append(self.image)
        shell += ["bash", "-lc", cmd]
        return shell

    def get_exec_shell(self, cmd):
        """Generate shell command string for docker exec"""
        shell: list[str] = ["docker", "exec"]
        shell.append("-it")
        shell.append(self.name)
        shell += ["bash", "-lc", cmd]
        return shell