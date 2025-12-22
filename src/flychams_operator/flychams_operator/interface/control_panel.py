"""Control panel with buttons to launch system components"""

from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QScrollArea, QStyle
from PyQt5.QtCore import Qt, QSize
from PyQt5.QtGui import QIcon, QPainter, QPixmap, QColor
import os
from typing import Dict
from .styles import (
    PANEL_BACKGROUND_STYLE,
    COLOR_BACKGROUND_SECONDARY,
    LABEL_STYLE_TITLE,
    LABEL_STYLE_SEPARATOR,
    BUTTON_STYLE_STANDARD,
    BUTTON_STYLE_DANGER,
    COLOR_TEXT_PRIMARY
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
        main_layout.setContentsMargins(24, 24, 24, 24)
        main_layout.setSpacing(30)

        # 1. Header Section
        header_layout = QHBoxLayout()
        title = QLabel('Mission Control')
        title.setStyleSheet(LABEL_STYLE_TITLE)
        header_layout.addWidget(title)
        main_layout.addLayout(header_layout)

        # 2. Categories Section (Horizontal)
        categories_layout = QHBoxLayout()
        categories_layout.setSpacing(20)

        # --- Coordinator Column ---
        coord_col, coord_layout = self.create_category_column("Coordinator")
        self.btn_coord = self.create_button(
            "Launch Coordinator", 
            self.launch_coordinator,
            self.get_white_icon(QStyle.SP_MediaPlay)
        )
        coord_layout.addWidget(self.btn_coord)
        categories_layout.addWidget(coord_col, 1)

        # --- Agents Column ---
        agents_col, self.agent_layout = self.create_category_column("Agents")
        categories_layout.addWidget(agents_col, 2)

        # --- Simulation Column ---
        if self.is_sim:
            sim_col, sim_layout = self.create_category_column("Simulation")
            self.btn_sim = self.create_button(
                "Launch Simulation Controller", 
                self.launch_simulation,
                self.get_white_icon(QStyle.SP_MediaPlay)
            )
            self.btn_ue5 = self.create_button(
                "Launch Unreal Engine 5", 
                self.launch_ue5,
                self.get_white_icon(QStyle.SP_MediaPlay)
            )
            sim_layout.addWidget(self.btn_sim)
            sim_layout.addWidget(self.btn_ue5)
            categories_layout.addWidget(sim_col, 1)

        # --- Operator Column ---
        op_col, op_layout = self.create_category_column("Operator")
        self.btn_rviz = self.create_button(
            "Launch RViZ", 
            self.launch_rviz,
            self.get_white_icon(QStyle.SP_MediaPlay)
        )
        op_layout.addWidget(self.btn_rviz)
        categories_layout.addWidget(op_col, 1)
        
        main_layout.addLayout(categories_layout)

        # 3. Footer Section
        footer_layout = QHBoxLayout()

        # Stop All Processes button
        self.btn_stop = QPushButton("Stop All Processes")
        self.btn_stop.setIcon(self.get_white_icon(QStyle.SP_MediaStop, size=QSize(64, 64)))
        self.btn_stop.setIconSize(QSize(64, 64))
        self.btn_stop.setStyleSheet(BUTTON_STYLE_DANGER)
        self.btn_stop.setMinimumWidth(400)
        self.btn_stop.setMinimumHeight(70)
        self.btn_stop.clicked.connect(self.stop_all)
        footer_layout.addWidget(self.btn_stop)
        main_layout.addLayout(footer_layout)

    def get_white_icon(self, standard_icon, size=QSize(32, 32)):
        """Helper to get a tinted white version of a standard system icon"""
        icon = self.style().standardIcon(standard_icon)
        pixmap = icon.pixmap(size)
        
        painter = QPainter(pixmap)
        painter.setCompositionMode(QPainter.CompositionMode_SourceIn)
        painter.fillRect(pixmap.rect(), QColor(COLOR_TEXT_PRIMARY))
        painter.end()
        
        return QIcon(pixmap)

    def create_category_column(self, title_text):
        """Helper to create a styled column with a scrollable container for widgets"""
        container = QWidget()
        container.setStyleSheet(f"""
            QWidget {{
                background-color: {COLOR_BACKGROUND_SECONDARY};
                border: 1px solid #3d3d3d;
                border-radius: 8px;
            }}
            QLabel {{
                border: none;
                background-color: transparent;
            }}
            QPushButton {{
                border: 1px solid #3d3d3d;
            }}
        """)
        
        main_layout = QVBoxLayout(container)
        main_layout.setContentsMargins(15, 15, 15, 15)
        main_layout.setSpacing(15)
        
        header = QLabel(title_text)
        header.setStyleSheet(LABEL_STYLE_SEPARATOR)
        header.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(header)

        # Scroll Area for internal content
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QScrollArea.NoFrame)
        scroll.setStyleSheet("background: transparent;")
        
        content_widget = QWidget()
        content_widget.setStyleSheet("background: transparent;")
        content_layout = QVBoxLayout(content_widget)
        content_layout.setContentsMargins(0, 0, 0, 0)
        content_layout.setSpacing(10)
        content_layout.setAlignment(Qt.AlignTop)
        
        scroll.setWidget(content_widget)
        main_layout.addWidget(scroll)
        
        return container, content_layout

    def create_separator(self, text):
        lbl = QLabel(text)
        lbl.setStyleSheet(LABEL_STYLE_SEPARATOR)
        return lbl

    def create_button(self, text, callback, icon=None):
        btn = QPushButton(f"  {text}")
        if icon:
            btn.setIcon(icon)
            btn.setIconSize(QSize(24, 24))
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

    def launch_ue5(self):
        if self.is_sim:
            self.pm.start_process("UE5", ["pixi", "run", "simulation-ue5-run"])

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
        
        btn_agent = self.create_button(
            f"Launch {agent_id}", 
            lambda: self.launch_agent(agent_id),
            self.get_white_icon(QStyle.SP_MediaPlay)
        )
        row_layout.addWidget(btn_agent)
        
        if self.is_sim:
            btn_px4 = self.create_button(
                f"Launch PX4-{idx}", 
                lambda: self.launch_px4(agent_id, idx),
                self.get_white_icon(QStyle.SP_MediaPlay)
            )
            row_layout.addWidget(btn_px4)
            
        self.agent_widgets[agent_id] = row
        self.agent_layout.addWidget(row)

    def remove_agent(self, agent_id: str):
        if agent_id in self.agent_widgets:
            widget = self.agent_widgets.pop(agent_id)
            self.agent_layout.removeWidget(widget)
            widget.deleteLater()
            self.agents.pop(agent_id, None)

