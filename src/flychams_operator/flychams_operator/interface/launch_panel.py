"""Mission launch panel with buttons to launch system components"""

from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QPlainTextEdit, QPushButton, QLabel, QTabWidget, QHBoxLayout
from PyQt5.QtCore import QTimer
import subprocess
from pathlib import Path
import os
import threading
import queue
import re
from typing import Dict
from .styles import (
    TERMINAL_STYLE,
    PANEL_BACKGROUND_STYLE,
    LABEL_STYLE_TITLE,
    LABEL_STYLE_SEPARATOR,
    BUTTON_STYLE_STANDARD,
    BUTTON_STYLE_DANGER,
    TAB_WIDGET_STYLE_COMPACT
)

def strip_ansi_codes(text: str) -> str:
    """Remove ANSI escape codes from text"""
    # Pattern to match ANSI escape sequences: ESC[ followed by numbers/semicolons and ending with m
    ansi_escape = re.compile(r'\x1b\[[0-9;]*m')
    return ansi_escape.sub('', text)

class Terminal(QWidget):
    """Read-only terminal-like text area with an append API"""

    def __init__(self, placeholder_text: str = ""):
        super().__init__()

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self._output = QPlainTextEdit()
        self._output.setReadOnly(True)

        # Terminal-ish styling with improved dark theme
        font = QFont("Monospace")
        font.setStyleHint(QFont.StyleHint.Monospace)
        font.setPointSize(10)
        self._output.setFont(font)
        self._output.setStyleSheet(TERMINAL_STYLE)

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
    
    def __init__(self, is_sim: bool):
        super().__init__()
        
        # Set dark background for the panel
        self.setStyleSheet(f"""
            QWidget {{
                {PANEL_BACKGROUND_STYLE}
            }}
        """)
        
        # Dictionary to track terminals for each component
        self.terminals = {}
        
        # Track if "No Process" tab exists
        self.has_no_process_tab = False
        
        # Queue for thread-safe logging from background threads
        self.log_queue = queue.Queue()
        
        # Timer to process log queue
        self.log_timer = QTimer()
        self.log_timer.timeout.connect(self.process_log_queue)
        self.log_timer.start(100)  # Check every 100ms
        
        # Track agents and their indices
        self.agents: Dict[str, int] = {}  # agent_id -> agent_index
        self.agent_buttons: Dict[str, Dict[str, QPushButton]] = {}  # agent_id -> {'agent': button, 'px4': button}
        self.next_agent_index = 0
        
        # Simulation mode flag (default to True for simulation)
        self.is_sim = is_sim
        
        self.setup_ui()

        # Get FlyChams path environment variable
        flychams_path = os.getenv('FLYCHAMS_ROS2_PATH')
        if not flychams_path:
            raise ValueError('FLYCHAMS_ROS2_PATH environment variable is not set')
    
    def setup_ui(self):
        """Set up the UI components"""
        layout = QVBoxLayout(self)
        layout.setSpacing(12)
        layout.setContentsMargins(16, 16, 16, 16)
        
        # Title with improved styling
        title = QLabel('Mission Launch')
        title.setStyleSheet(LABEL_STYLE_TITLE)
        layout.addWidget(title)
        
        # Separator label for agent-specific buttons
        self.global_separator_label = QLabel('Global Controls')
        self.global_separator_label.setStyleSheet(LABEL_STYLE_SEPARATOR)
        layout.addWidget(self.global_separator_label)

        # First row: Coordinator and Dashboard (2 buttons)
        row1 = QWidget()
        row1_layout = QHBoxLayout(row1)
        row1_layout.setContentsMargins(0, 0, 0, 0)
        row1_layout.setSpacing(8)
        
        self.launch_coordinator_btn = QPushButton('LAUNCH COORDINATOR')
        self.launch_coordinator_btn.setStyleSheet(BUTTON_STYLE_STANDARD)
        self.launch_coordinator_btn.clicked.connect(self.launch_coordinator)
        row1_layout.addWidget(self.launch_coordinator_btn)
        
        layout.addWidget(row1)

        # Second row: Simulation and Stop (2 buttons)
        row2 = QWidget()
        row2_layout = QHBoxLayout(row2)
        row2_layout.setContentsMargins(0, 0, 0, 0)
        row2_layout.setSpacing(8)
        
        self.launch_simulation_btn = QPushButton('LAUNCH SIMULATION')
        self.launch_simulation_btn.setStyleSheet(BUTTON_STYLE_STANDARD)
        self.launch_simulation_btn.clicked.connect(self.launch_simulation)
        row2_layout.addWidget(self.launch_simulation_btn)

        self.stop_btn = QPushButton('STOP')
        self.stop_btn.setStyleSheet(BUTTON_STYLE_DANGER)
        self.stop_btn.clicked.connect(self.stop)
        row2_layout.addWidget(self.stop_btn)
        
        layout.addWidget(row2)

        # Separator label for agent-specific buttons
        self.agent_separator_label = QLabel('Agent Controls')
        self.agent_separator_label.setStyleSheet(LABEL_STYLE_SEPARATOR)
        layout.addWidget(self.agent_separator_label)
        
        # Container for agent buttons (will be populated dynamically)
        self.agent_buttons_container = QWidget()
        self.agent_buttons_layout = QVBoxLayout(self.agent_buttons_container)
        self.agent_buttons_layout.setContentsMargins(0, 0, 0, 8)
        self.agent_buttons_layout.setSpacing(8)
        layout.addWidget(self.agent_buttons_container)
        
        # Create tabbed terminal widget with improved styling
        self.terminal_tabs = QTabWidget()
        self.terminal_tabs.setStyleSheet(TAB_WIDGET_STYLE_COMPACT)
        layout.addWidget(self.terminal_tabs)
        
        # Create initial terminal tab
        initial_terminal = Terminal(placeholder_text="Press a button to start the system")
        self.terminals["No Process"] = initial_terminal
        self.terminal_tabs.addTab(initial_terminal, "No Process")
        self.has_no_process_tab = True
    
    # ================================ Terminal methods ================================
    def get_or_create_terminal(self, component_name: str) -> Terminal:
        """Get or create a terminal tab for a component"""
        # Remove "No Process" tab if this is the first real process
        if self.has_no_process_tab and component_name != "No Process":
            # Find and remove the "No Process" tab
            for i in range(self.terminal_tabs.count()):
                if self.terminal_tabs.tabText(i) == "No Process":
                    self.terminal_tabs.removeTab(i)
                    if "No Process" in self.terminals:
                        del self.terminals["No Process"]
                    self.has_no_process_tab = False
                    break
        
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
        if component_name is not None:
            terminal = self.get_or_create_terminal(component_name)
            terminal.append_line(message)
    
    def read_process_output(self, process: subprocess.Popen, component_name: str):
        """Read stdout and stderr from a process and append to terminal"""
        def read_stream(stream, is_stderr=False):
            try:
                for line in iter(stream.readline, b''):
                    if line:
                        line_str = line.decode('utf-8', errors='replace').rstrip()
                        # Strip ANSI escape codes
                        line_str = strip_ansi_codes(line_str)
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
    
    # ================================ Launch methods ================================
    def launch_coordinator(self):
        """Launch the coordinator using pixi task"""
        component_name = "Coordinator"
        
        # Choose task based on is_sim parameter
        task_name = "coordinator-sim-run" if self.is_sim else "coordinator-hardware-run"
        cmd = ["pixi", "run", task_name]

        mode_str = "simulation" if self.is_sim else "hardware"
        self.log(f'Launching coordinator ({mode_str} mode)...', component_name)
        try:
            # Launch in background
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            self.log(f'Coordinator launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR launching coordinator: {e}', component_name)

    def launch_agent(self, agent_id: str):
        """Launch a specific agent by ID using pixi task"""
        component_name = agent_id
        
        self.log(f'Launching agent {agent_id}...', component_name)
        try:
            # Launch in background using pixi task with agent ID as argument
            process = subprocess.Popen(
                ['pixi', 'run', 'agent-sim-run', agent_id],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            self.log(f'Agent {agent_id} launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR launching agent {agent_id}: {e}', component_name)
    
    def launch_simulation(self):
        """Launch the simulation using pixi task"""
        component_name = "Simulation"
        
        self.log(f'Launching simulation...', component_name)
        try:
            # Launch in background using pixi task
            process = subprocess.Popen(
                ['pixi', 'run', 'simulation-run'],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            self.log(f'Simulation launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR launching simulation: {e}', component_name)

    def launch_px4(self, agent_id: str, agent_index: int):
        """Launch PX4 for a specific agent using pixi task"""
        component_name = f"PX4-{agent_index}"
        
        self.log(f'Launching PX4 for agent {agent_id} (index: {agent_index})...', component_name)
        try:
            # Launch in background using pixi task with agent index as argument
            process = subprocess.Popen(
                ['pixi', 'run', 'simulation-px4-run', str(agent_index)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            self.log(f'PX4 for agent {agent_id} launched (PID: {process.pid})', component_name)
            # Start reading output
            self.read_process_output(process, component_name)
        except Exception as e:
            self.log(f'ERROR launching PX4 for agent {agent_id}: {e}', component_name)

    def stop(self):
        """Stop all FlyChams components"""
        component_name = "STOP"
        
        self.log(f'Stopping FlyChams components...', component_name)
        try:
            # Stop agents using pixi task (requires agent ID, but we'll try to stop all known agents)
            # Note: This is a simplified stop - it only stops agents that were launched
            # For a complete stop, you may need to manually stop each component
            for agent_id in list(self.agents.keys()):
                try:
                    stop_process = subprocess.Popen(
                        ['pixi', 'run', 'agent-sim-stop', agent_id],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE
                    )
                    self.log(f'Stop command for agent {agent_id} launched (PID: {stop_process.pid})', component_name)
                    self.read_process_output(stop_process, component_name)
                except Exception as e:
                    self.log(f'ERROR stopping agent {agent_id}: {e}', component_name)
            
            self.log(f'Stop commands launched for {len(self.agents)} agents', component_name)
            self.log(f'Note: Coordinator, Simulation, and PX4 processes may need to be stopped manually (Ctrl+C)', component_name)
        except Exception as e:
            self.log(f'ERROR stopping FlyChams: {e}', component_name)

    # ================================ Signal callbacks ================================
    def add_agent(self, agent_id: str):
        """Handle agent addition - create buttons for this agent"""
        # Assign agent index
        agent_index = self.next_agent_index
        self.agents[agent_id] = agent_index
        self.next_agent_index += 1
        
        # Create buttons for this agent with improved styling
        agent_row = QWidget()
        agent_row_layout = QHBoxLayout(agent_row)
        agent_row_layout.setContentsMargins(0, 0, 0, 0)
        agent_row_layout.setSpacing(8)
        
        # Agent launch button with homogeneous styling
        agent_btn = QPushButton(f'LAUNCH {agent_id}')
        agent_btn.setStyleSheet(BUTTON_STYLE_STANDARD)
        agent_btn.clicked.connect(lambda checked, aid=agent_id: self.launch_agent(aid))
        agent_row_layout.addWidget(agent_btn)
        
        # PX4 launch button for this agent with homogeneous styling
        px4_btn = QPushButton(f'PX4-{agent_index}')
        px4_btn.setStyleSheet(BUTTON_STYLE_STANDARD)
        def make_px4_handler(aid: str):
            def handler(checked):
                idx = self.agents.get(aid)
                if idx is not None:
                    self.launch_px4(aid, idx)
            return handler
        px4_btn.clicked.connect(make_px4_handler(agent_id))
        agent_row_layout.addWidget(px4_btn)
        
        # Store buttons
        self.agent_buttons[agent_id] = {
            'agent': agent_btn,
            'px4': px4_btn,
            'widget': agent_row
        }
        
        # Add to layout
        self.agent_buttons_layout.addWidget(agent_row)
    
    def remove_agent(self, agent_id: str):
        """Handle agent removal - remove buttons for this agent"""
        if agent_id in self.agent_buttons:
            # Remove widget from layout
            widget = self.agent_buttons[agent_id]['widget']
            self.agent_buttons_layout.removeWidget(widget)
            widget.deleteLater()
            
            # Remove from tracking
            del self.agent_buttons[agent_id]
            if agent_id in self.agents:
                del self.agents[agent_id]