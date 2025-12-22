"""Control panel with buttons to launch system components"""

from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QScrollArea
from PyQt5.QtCore import Qt
import os
from typing import Dict
from .styles import (
    PANEL_BACKGROUND_STYLE,
    LABEL_STYLE_TITLE,
    LABEL_STYLE_SEPARATOR,
    BUTTON_STYLE_STANDARD,
    BUTTON_STYLE_DANGER
)

class ControlPanel(QWidget):
    """Panel with buttons to launch coordinator, agents, simulation, etc"""
    
    def __init__(self, process_manager, is_sim: bool):
        super().__init__()
        self.pm = process_manager
        self.is_sim = is_sim
        self.agents = {}  # agent_id -> index
        self.agent_widgets = {} # agent_id -> widget
        
        self.setStyleSheet(PANEL_BACKGROUND_STYLE)
        self.setup_ui()

    def setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(16, 16, 16, 16)
        main_layout.setSpacing(20)

        # Title
        title = QLabel('Mission Control')
        title.setStyleSheet(LABEL_STYLE_TITLE)
        main_layout.addWidget(title)

        # Scroll Area for controls
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QScrollArea.NoFrame)
        scroll.setStyleSheet("background: transparent;")
        
        content = QWidget()
        content.setStyleSheet("background: transparent;")
        self.layout = QVBoxLayout(content)
        self.layout.setSpacing(15)
        self.layout.setAlignment(Qt.AlignTop)
        
        # --- Global Controls ---
        self.layout.addWidget(self._create_separator("Global Controls"))
        
        global_row1 = QHBoxLayout()
        self.btn_coord = self._create_button("COORDINATOR", self.launch_coordinator)
        self.btn_rviz = self._create_button("RVIZ", self.launch_rviz)
        global_row1.addWidget(self.btn_coord)
        global_row1.addWidget(self.btn_rviz)
        self.layout.addLayout(global_row1)

        global_row2 = QHBoxLayout()
        if self.is_sim:
            self.btn_sim = self._create_button("SIMULATION", self.launch_simulation)
            global_row2.addWidget(self.btn_sim)
        
        self.btn_stop = QPushButton("STOP ALL")
        self.btn_stop.setStyleSheet(BUTTON_STYLE_DANGER)
        self.btn_stop.clicked.connect(self.stop_all)
        global_row2.addWidget(self.btn_stop)
        self.layout.addLayout(global_row2)

        # --- Agent Controls ---
        self.layout.addWidget(self._create_separator("Agent Controls"))
        
        # Container for dynamic agent rows
        self.agent_container = QWidget()
        self.agent_layout = QVBoxLayout(self.agent_container)
        self.agent_layout.setContentsMargins(0, 0, 0, 0)
        self.agent_layout.setSpacing(10)
        self.layout.addWidget(self.agent_container)

        scroll.setWidget(content)
        main_layout.addWidget(scroll)

    def _create_separator(self, text):
        lbl = QLabel(text)
        lbl.setStyleSheet(LABEL_STYLE_SEPARATOR)
        return lbl

    def _create_button(self, text, callback):
        btn = QPushButton(text)
        btn.setStyleSheet(BUTTON_STYLE_STANDARD)
        btn.clicked.connect(callback)
        return btn

    # --- Launch Logic ---
    
    def launch_coordinator(self):
        task = "coordinator-sim-run" if self.is_sim else "coordinator-hardware-run"
        self.pm.start_process("Coordinator", ["pixi", "run", task])

    def launch_rviz(self):
        self.pm.start_process("Rviz", ["pixi", "run", "operator-rviz"])

    def launch_simulation(self):
        if self.is_sim:
            self.pm.start_process("Simulation", ["pixi", "run", "simulation-run"])

    def launch_agent(self, agent_id):
        if self.is_sim:
            self.pm.start_process(agent_id, ["pixi", "run", "agent-sim-run", agent_id])
        else:
            # For hardware, typically we launch the generic agent-hardware-run
            # Note: pixi.toml has agent-hardware-run without ID.
            self.pm.start_process(f"Agent-{agent_id}", ["pixi", "run", "agent-hardware-run"])

    def launch_px4(self, agent_id, index):
        if self.is_sim:
            self.pm.start_process(f"PX4-{index}", ["pixi", "run", "simulation-px4-run", str(index)])

    def stop_all(self):
        self.pm.stop_all()

    # --- Discovery Callbacks ---

    def add_agent(self, agent_id: str):
        if agent_id in self.agents: return
        
        idx = len(self.agents)
        self.agents[agent_id] = idx
        
        row = QWidget()
        row_layout = QHBoxLayout(row)
        row_layout.setContentsMargins(0, 0, 0, 0)
        
        btn_agent = self._create_button(f"LAUNCH {agent_id}", lambda: self.launch_agent(agent_id))
        row_layout.addWidget(btn_agent)
        
        if self.is_sim:
            btn_px4 = self._create_button(f"PX4-{idx}", lambda: self.launch_px4(agent_id, idx))
            row_layout.addWidget(btn_px4)
            
        self.agent_widgets[agent_id] = row
        self.agent_layout.addWidget(row)

    def remove_agent(self, agent_id: str):
        if agent_id in self.agent_widgets:
            widget = self.agent_widgets.pop(agent_id)
            self.agent_layout.removeWidget(widget)
            widget.deleteLater()
            self.agents.pop(agent_id, None)

