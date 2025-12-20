"""Mission launch panel with buttons to launch system components."""

from PyQt6.QtWidgets import QWidget, QVBoxLayout, QPushButton, QLabel, QGroupBox, QTextEdit
from PyQt6.QtCore import Qt
import subprocess
import os
from pathlib import Path


class MissionLaunchPanel(QWidget):
    """Panel with buttons to launch coordinator, agents, simulation, etc."""
    
    def __init__(self):
        super().__init__()
        
        self.setup_ui()
        
        # Find tools directory
        # Assuming we're in ros2_ws/src/flychams_dashboard, go up to project root
        current_file = Path(__file__).resolve()
        self.tools_dir = current_file.parent.parent.parent.parent.parent / 'tools'
    
    def setup_ui(self):
        """Set up the UI components."""
        layout = QVBoxLayout(self)
        
        # Title
        title = QLabel('Mission Launch')
        title.setStyleSheet('font-size: 16px; font-weight: bold;')
        layout.addWidget(title)
        
        # Coordinator group
        coordinator_group = QGroupBox('Coordinator')
        coordinator_layout = QVBoxLayout()
        
        self.launch_coordinator_btn = QPushButton('Launch Coordinator (Sim)')
        self.launch_coordinator_btn.clicked.connect(lambda: self.launch_coordinator('--sim'))
        coordinator_layout.addWidget(self.launch_coordinator_btn)
        
        self.launch_coordinator_hw_btn = QPushButton('Launch Coordinator (Hardware)')
        self.launch_coordinator_hw_btn.clicked.connect(lambda: self.launch_coordinator('--hardware'))
        coordinator_layout.addWidget(self.launch_coordinator_hw_btn)
        
        coordinator_group.setLayout(coordinator_layout)
        layout.addWidget(coordinator_group)
        
        # Agents group
        agents_group = QGroupBox('Agents')
        agents_layout = QVBoxLayout()
        
        self.launch_agent_btn = QPushButton('Launch Agent (Sim)')
        self.launch_agent_btn.clicked.connect(lambda: self.launch_agent('--sim'))
        agents_layout.addWidget(self.launch_agent_btn)
        
        self.launch_agent_hw_btn = QPushButton('Launch Agent (Hardware)')
        self.launch_agent_hw_btn.clicked.connect(lambda: self.launch_agent('--hardware'))
        agents_layout.addWidget(self.launch_agent_hw_btn)
        
        agents_group.setLayout(agents_layout)
        layout.addWidget(agents_group)
        
        # Simulation group
        simulation_group = QGroupBox('Simulation')
        simulation_layout = QVBoxLayout()
        
        self.launch_simulation_btn = QPushButton('Launch Simulation')
        self.launch_simulation_btn.clicked.connect(self.launch_simulation)
        simulation_layout.addWidget(self.launch_simulation_btn)
        
        simulation_group.setLayout(simulation_layout)
        layout.addWidget(simulation_group)
        
        # Status log
        log_group = QGroupBox('Launch Log')
        log_layout = QVBoxLayout()
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(200)
        log_layout.addWidget(self.log_text)
        
        log_group.setLayout(log_layout)
        layout.addWidget(log_group)
        
        # Spacer
        layout.addStretch()
    
    def log(self, message: str):
        """Add a message to the log."""
        self.log_text.append(message)
        # Auto-scroll to bottom
        scrollbar = self.log_text.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
    
    def launch_coordinator(self, mode: str):
        """Launch the coordinator."""
        script_path = self.tools_dir / 'launch_coordinator.py'
        if not script_path.exists():
            self.log(f'ERROR: Script not found: {script_path}')
            return
        
        self.log(f'Launching coordinator ({mode})...')
        try:
            # Launch in background
            process = subprocess.Popen(
                ['python3', str(script_path), mode],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(self.tools_dir.parent)
            )
            self.log(f'Coordinator launched (PID: {process.pid})')
        except Exception as e:
            self.log(f'ERROR launching coordinator: {e}')
    
    def launch_agent(self, mode: str):
        """Launch an agent (will prompt for agent ID)."""
        # For now, just log - could add a dialog to get agent ID
        self.log(f'Agent launch requested ({mode})')
        self.log('NOTE: Agent ID selection dialog not yet implemented')
        self.log('Use: python3 tools/launch_agent.py --agent-id <ID> {mode}')
    
    def launch_simulation(self):
        """Launch the simulation."""
        script_path = self.tools_dir / 'launch_simulation.py'
        if not script_path.exists():
            self.log(f'ERROR: Script not found: {script_path}')
            return
        
        self.log('Launching simulation...')
        try:
            # Launch in background
            process = subprocess.Popen(
                ['python3', str(script_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(self.tools_dir.parent)
            )
            self.log(f'Simulation launched (PID: {process.pid})')
        except Exception as e:
            self.log(f'ERROR launching simulation: {e}')

