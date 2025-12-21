"""Mission launch panel with buttons to launch system components"""

from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import QWidget, QVBoxLayout, QPlainTextEdit, QPushButton, QLabel, QGroupBox, QTabWidget
from PyQt6.QtCore import QTimer
import subprocess
from pathlib import Path
import os
import shlex
import threading
import queue

class Terminal(QWidget):
    """Read-only terminal-like text area with an append API"""

    def __init__(self, placeholder_text: str = ""):
        super().__init__()

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        self._output = QPlainTextEdit()
        self._output.setReadOnly(True)

        # Terminal-ish styling
        font = QFont("Monospace")
        font.setStyleHint(QFont.StyleHint.Monospace)
        self._output.setFont(font)
        self._output.setStyleSheet(
            "QPlainTextEdit { background-color: #222222; color: #ffffff; }"
        )

        if placeholder_text:
            self._output.setPlainText(placeholder_text)

        layout.addWidget(self._output)

    def clear(self) -> None:
        self._output.clear()

    def append_line(self, text: str) -> None:
        """Append a single line and auto-scroll."""
        self._output.appendPlainText(text)
        scrollbar = self._output.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

class LaunchPanel(QWidget):
    """Panel with buttons to launch coordinator, agents, simulation, etc"""
    
    def __init__(self):
        super().__init__()
        
        # Dictionary to track terminals for each component
        self.terminals = {}
        
        # Queue for thread-safe logging from background threads
        self.log_queue = queue.Queue()
        
        # Timer to process log queue
        self.log_timer = QTimer()
        self.log_timer.timeout.connect(self.process_log_queue)
        self.log_timer.start(100)  # Check every 100ms
        
        self.setup_ui()

        # Get FlyChams path environment variable
        flychams_path = os.getenv('FLYCHAMS_PATH')
        if not flychams_path:
            raise ValueError('FLYCHAMS_PATH environment variable is not set')
        
        # Set tools directory
        self.tools_dir = Path(flychams_path) / 'tools'
    
    def setup_ui(self):
        """Set up the UI components"""
        layout = QVBoxLayout(self)
        
        # Title
        title = QLabel('Mission Launch')
        title.setStyleSheet('font-size: 16px; font-weight: bold;')
        layout.addWidget(title)
        
        self.launch_coordinator_btn = QPushButton('Launch Coordinator')
        self.launch_coordinator_btn.clicked.connect(self.launch_coordinator)
        layout.addWidget(self.launch_coordinator_btn)
         
        self.launch_agents_btn = QPushButton('Launch Agents')
        self.launch_agents_btn.clicked.connect(self.launch_agents)
        layout.addWidget(self.launch_agents_btn)

        self.launch_dashboard_btn = QPushButton('Launch Dashboard')
        self.launch_dashboard_btn.clicked.connect(self.launch_dashboard)
        layout.addWidget(self.launch_dashboard_btn)

        self.launch_simulation_btn = QPushButton('Launch Simulation')
        self.launch_simulation_btn.clicked.connect(self.launch_simulation)
        layout.addWidget(self.launch_simulation_btn)

        self.launch_px4_btn = QPushButton('Launch PX4')
        self.launch_px4_btn.clicked.connect(self.launch_px4)
        layout.addWidget(self.launch_px4_btn)      

        self.stop_btn = QPushButton('STOP')
        self.stop_btn.clicked.connect(self.stop)
        layout.addWidget(self.stop_btn)
        
        # Create tabbed terminal widget
        self.terminal_tabs = QTabWidget()
        layout.addWidget(self.terminal_tabs)
        
        # Create initial terminal tab
        initial_terminal = Terminal(placeholder_text="Press Launch Coordinator to start the system")
        self.terminals["General"] = initial_terminal
        self.terminal_tabs.addTab(initial_terminal, "General")
    
    def get_or_create_terminal(self, component_name: str) -> Terminal:
        """Get or create a terminal tab for a component"""
        if component_name not in self.terminals:
            # Create new terminal for this component
            terminal = Terminal()
            self.terminals[component_name] = terminal
            # Add tab with component name
            self.terminal_tabs.addTab(terminal, component_name)
            # Switch to the new tab
            self.terminal_tabs.setCurrentWidget(terminal)
        return self.terminals[component_name]
    
    def process_log_queue(self):
        """Process queued log messages from background threads"""
        while not self.log_queue.empty():
            try:
                component_name, message = self.log_queue.get_nowait()
                terminal = self.get_or_create_terminal(component_name)
                terminal.append_line(message)
            except queue.Empty:
                break
    
    def log(self, message: str, component_name: str = None):
        """Add a message to the log for a specific component"""
        if component_name is None:
            # If no component specified, log to first tab or create a default one
            if not self.terminals:
                component_name = "General"
            else:
                # Use the currently active tab
                current_index = self.terminal_tabs.currentIndex()
                if current_index >= 0:
                    component_name = self.terminal_tabs.tabText(current_index)
                else:
                    component_name = "General"
        
        terminal = self.get_or_create_terminal(component_name)
        terminal.append_line(message)
    
    def read_process_output(self, process: subprocess.Popen, component_name: str):
        """Read stdout and stderr from a process and append to terminal"""
        def read_stream(stream, is_stderr=False):
            try:
                for line in iter(stream.readline, b''):
                    if line:
                        line_str = line.decode('utf-8', errors='replace').rstrip()
                        prefix = "[STDERR] " if is_stderr else ""
                        self.log_queue.put((component_name, f"{prefix}{line_str}"))
            except Exception as e:
                self.log_queue.put((component_name, f"ERROR reading stream: {e}"))
            finally:
                stream.close()
        
        # Start threads to read stdout and stderr
        stdout_thread = threading.Thread(target=read_stream, args=(process.stdout, False), daemon=True)
        stderr_thread = threading.Thread(target=read_stream, args=(process.stderr, True), daemon=True)
        stdout_thread.start()
        stderr_thread.start()
    
    def launch_coordinator(self):
        """Launch the coordinator"""
        component_name = "Coordinator"
        script_path = self.tools_dir / 'launch_coordinator.py'
        if not script_path.exists():
            self.log(f'ERROR: Script not found: {script_path}', component_name)
            return
        
        cmd = [
            "python3",
            "-u",
            str(self.tools_dir / "launch_coordinator.py"),
            "--sim"
        ]

        self.log(f'Launching coordinator...', component_name)
        try:
            # Launch in background
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(self.tools_dir.parent)
            )
            self.log(f'Coordinator launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR launching coordinator: {e}', component_name)
    
    def launch_agents(self):
        """Launch every registered agent"""
        component_name = "Agents"
        self.log(f'Launching agents...', component_name)
        # TODO: Implement
        self.log(f'Agents launch not yet implemented', component_name)

    def launch_dashboard(self):
        """Launch the dashboard"""
        component_name = "Dashboard"
        script_path = self.tools_dir / 'launch_dashboard.py'
        if not script_path.exists():
            self.log(f'ERROR: Script not found: {script_path}', component_name)
            return
        
        self.log(f'Launching dashboard...', component_name)
        try:
            # Launch in background
            process = subprocess.Popen(
                ['python3', str(script_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(self.tools_dir.parent)
            )
            self.log(f'Dashboard launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR launching dashboard: {e}', component_name)
    
    def launch_simulation(self):
        """Launch the simulation"""
        component_name = "Simulation"
        script_path = self.tools_dir / 'launch_simulation.py'
        if not script_path.exists():
            self.log(f'ERROR: Script not found: {script_path}', component_name)
            return
        
        self.log(f'Launching simulation...', component_name)
        try:
            # Launch in background
            process = subprocess.Popen(
                ['python3', str(script_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(self.tools_dir.parent)
            )
            self.log(f'Simulation launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR launching simulation: {e}', component_name)

    def launch_px4(self):
        """Launch PX4 for every registered agent"""
        component_name = "PX4"
        self.log(f'Launching PX4...', component_name)
        # TODO: Implement
        self.log(f'PX4 launch not yet implemented', component_name)

    def stop(self):
        """Stop all FlyChams components"""
        component_name = "STOP"
        script_path = self.tools_dir / 'stop.py'
        if not script_path.exists():
            self.log(f'ERROR: Script not found: {script_path}', component_name)
            return
        
        self.log(f'Stopping FlyChams...', component_name)
        try:
            # Stop in background
            process = subprocess.Popen(
                ['python3', str(script_path), '--sim'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(self.tools_dir.parent)
            )
            self.log(f'Stop command launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR stopping FlyChams: {e}', component_name)