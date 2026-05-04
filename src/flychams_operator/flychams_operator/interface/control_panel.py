"""Control panel with buttons to launch system components"""

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, 
    QScrollArea, QStyle, QFrame, QGraphicsDropShadowEffect,
    QSizePolicy, QStatusBar
)
from PyQt5.QtCore import Qt, QSize, QTimer
from PyQt5.QtGui import QIcon, QPainter, QColor
from typing import Dict

from .styles import (
    PANEL_BACKGROUND_STYLE,
    COLOR_BACKGROUND_SECONDARY,
    COLOR_BACKGROUND_TERTIARY,
    COLOR_BACKGROUND_PRIMARY,
    COLOR_BORDER_PRIMARY,
    COLOR_ACCENT_PRIMARY,
    COLOR_TEXT_PRIMARY,
    COLOR_TEXT_SECONDARY,
    BUTTON_STYLE_STANDARD,
    BUTTON_STYLE_SECONDARY,
    BUTTON_STYLE_DANGER,
    LABEL_STYLE_TITLE
)

class ControlCard(QFrame):
    """A styled container for a category of controls"""
    def __init__(self, title: str, parent=None, scrollable: bool = False):
        super().__init__(parent)
        self.setObjectName("ControlCard")
        
        # Card Styling
        self.setStyleSheet(f"""
            QFrame#ControlCard {{
                background-color: {COLOR_BACKGROUND_SECONDARY};
                border: 1px solid {COLOR_BORDER_PRIMARY};
                border-radius: 12px;
            }}
            QLabel {{
                border: none;
                background: transparent;
            }}
        """)
        
        # Shadow Effect
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(15)
        shadow.setXOffset(0)
        shadow.setYOffset(4)
        shadow.setColor(QColor(0, 0, 0, 60))
        self.setGraphicsEffect(shadow)

        # Main Layout
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setContentsMargins(20, 20, 20, 20)
        self.main_layout.setSpacing(15)

        # Title
        self.title_label = QLabel(title)
        self.title_label.setStyleSheet(f"""
            font-size: 22px;
            font-weight: bold;
            letter-spacing: 1px;
            color: {COLOR_TEXT_SECONDARY};
            border-bottom: 2px solid {COLOR_ACCENT_PRIMARY};
            padding-bottom: 8px;
        """)
        self.main_layout.addWidget(self.title_label)

        # Content Container
        if scrollable:
            self.scroll_area = QScrollArea()
            self.scroll_area.setWidgetResizable(True)
            self.scroll_area.setFrameShape(QScrollArea.NoFrame)
            self.scroll_area.setStyleSheet("background: transparent;")
            
            self.content_widget = QWidget()
            self.content_widget.setStyleSheet("background: transparent;")
            self.content_layout = QVBoxLayout(self.content_widget)
            self.content_layout.setContentsMargins(0, 0, 0, 0)
            self.content_layout.setSpacing(10)
            self.content_layout.setAlignment(Qt.AlignTop)
            
            self.scroll_area.setWidget(self.content_widget)
            self.main_layout.addWidget(self.scroll_area)
        else:
            self.content_layout = QVBoxLayout()
            self.content_layout.setSpacing(12)
            self.content_layout.setAlignment(Qt.AlignTop)
            self.main_layout.addLayout(self.content_layout)
            self.main_layout.addStretch()

    def add_widget(self, widget):
        self.content_layout.addWidget(widget)

    def add_layout(self, layout):
        self.content_layout.addLayout(layout)


class ControlPanel(QWidget):
    """Panel with buttons to launch coordinator, agents, simulation, etc"""
    
    def __init__(self, process_manager, is_sim: bool):
        super().__init__()
        self.pm = process_manager
        self.is_sim = is_sim
        self.agents: Dict[str, int] = {}  # agent_id -> index
        self.agent_widgets: Dict[str, QWidget] = {} # agent_id -> widget
        
        self.setStyleSheet(PANEL_BACKGROUND_STYLE)
        self.setup_ui()

    def setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(30, 30, 30, 30)
        main_layout.setSpacing(25)

        # 1. Header Section
        header_layout = QHBoxLayout()
        title = QLabel('Mission Control')
        title.setStyleSheet(LABEL_STYLE_TITLE)
        header_layout.addWidget(title)
        
        # Status Bar for feedback
        self.status_bar = QStatusBar()
        self.status_bar.setStyleSheet(f"""
            QStatusBar {{
                background-color: {COLOR_BACKGROUND_SECONDARY};
                color: {COLOR_ACCENT_PRIMARY};
                font-size: 16px;
                font-weight: bold;
                border-radius: 6px;
                padding-left: 10px;
            }}
            QStatusBar::item {{
                border: none;
            }}
        """)
        self.status_bar.setSizeGripEnabled(False)
        header_layout.addWidget(self.status_bar, 1)
        
        main_layout.addLayout(header_layout)

        # 2. Cards Section
        cards_layout = QHBoxLayout()
        cards_layout.setSpacing(25)

        # --- Coordinator Card ---
        self.card_coord = ControlCard("Coordinator")
        self.btn_coord = self.create_button(
            "Coordinator", 
            self.launch_coordinator,
            BUTTON_STYLE_STANDARD,
            QStyle.SP_MediaPlay
        )
        self.card_coord.add_widget(self.btn_coord)
        cards_layout.addWidget(self.card_coord, 1)

        # --- Agents Card (Scrollable) ---
        self.card_agents = ControlCard("Agents", scrollable=True)
        cards_layout.addWidget(self.card_agents, 2)

        # --- Simulation Card ---
        if self.is_sim:
            self.card_sim = ControlCard("Simulation")
            self.btn_sim = self.create_button(
                "Sim Controller", 
                self.launch_simulation,
                BUTTON_STYLE_STANDARD,
                QStyle.SP_MediaPlay
            )
            self.btn_ue5 = self.create_button(
                "Unreal Engine 5", 
                self.launch_ue5,
                BUTTON_STYLE_STANDARD,
                QStyle.SP_MediaPlay
            )
            self.btn_cleanup = self.create_button(
                "Cleanup",
                self.cleanup_simulation,
                BUTTON_STYLE_DANGER,
                QStyle.SP_BrowserReload
            )
            self.card_sim.add_widget(self.btn_sim)
            self.card_sim.add_widget(self.btn_ue5)
            self.card_sim.add_widget(self.btn_cleanup)
            cards_layout.addWidget(self.card_sim, 1)

        # --- Operator Card ---
        self.card_op = ControlCard("Operator")
        self.btn_rviz = self.create_button(
            "RViZ", 
            self.launch_rviz,
            BUTTON_STYLE_STANDARD,
            QStyle.SP_MediaPlay
        )
        self.card_op.add_widget(self.btn_rviz)
        cards_layout.addWidget(self.card_op, 1)
        
        main_layout.addLayout(cards_layout)

        # 3. Footer Section
        footer_frame = QFrame()
        footer_frame.setStyleSheet(f"""
            QFrame {{
                background-color: {COLOR_BACKGROUND_SECONDARY};
                border: 1px solid {COLOR_BORDER_PRIMARY};
                border-radius: 12px;
            }}
            QLabel {{
                border: none;
                background: transparent;
            }}
        """)
        footer_layout = QHBoxLayout(footer_frame)
        footer_layout.setContentsMargins(20, 15, 20, 15)
        
        # Emergency Stop button
        self.btn_stop = self.create_button(
            "Emergency Stop",
            self.stop_all,
            BUTTON_STYLE_DANGER,
            QStyle.SP_MediaStop
        )
        footer_layout.addWidget(self.btn_stop)
        main_layout.addWidget(footer_frame)

    def get_white_icon(self, standard_icon, size=QSize(32, 32)):
        """Helper to get a tinted white version of a standard system icon"""
        icon = self.style().standardIcon(standard_icon)
        pixmap = icon.pixmap(size)
        
        painter = QPainter(pixmap)
        painter.setCompositionMode(QPainter.CompositionMode_SourceIn)
        painter.fillRect(pixmap.rect(), QColor(COLOR_TEXT_PRIMARY))
        painter.end()
        
        return QIcon(pixmap)

    def create_button(self, text, callback, style=BUTTON_STYLE_STANDARD, icon_standard=None):
        """Creates a styled button with optional icon"""
        btn = QPushButton(f"  {text}")
        if icon_standard:
            btn.setIcon(self.get_white_icon(icon_standard))
            btn.setIconSize(QSize(32, 32))
            
        btn.setStyleSheet(style)
        btn.setCursor(Qt.PointingHandCursor)
        btn.clicked.connect(callback)
        return btn

    def show_feedback(self, message: str, duration: int = 3000):
        """Show a temporary message in the status bar"""
        self.status_bar.showMessage(message, duration)

    # --- Launch Logic ---
    
    def launch_coordinator(self):
        task = "coordinator-sim-run" if self.is_sim else "coordinator-hardware-run"
        self.pm.start_process("Coordinator", ["pixi", "run", task], ["pixi", "run", "coordinator-stop"])
        self.show_feedback("Launching Coordinator...")

    def launch_rviz(self):
        self.pm.start_process("Rviz", ["pixi", "run", "operator-rviz"], ["pixi", "run", "operator-rviz-stop"])
        self.show_feedback("Launching Rviz...")

    def launch_simulation(self):
        if self.is_sim:
            self.pm.start_process("Simulation", ["pixi", "run", "simulation-run"], ["pixi", "run", "simulation-stop"])
            self.show_feedback("Launching Simulation...")

    def launch_ue5(self):
        if self.is_sim:
            self.pm.start_process("UE5", ["pixi", "run", "simulation-ue5-run"], ["pixi", "run", "simulation-ue5-stop"])
            self.show_feedback("Launching Unreal Engine 5...")

    def launch_agent(self, agent_id):
        if self.is_sim:
            component_name = f"Agent-{agent_id}"
            if self.pm.is_running(component_name):
                self.pm.stop_process(component_name)
                self.show_feedback(f"Stopping Agent {agent_id}...")
            else:
                self.pm.start_process(component_name, ["bash", "tools/agent_setup.sh", "run", agent_id], ["bash", "tools/agent_setup.sh", "stop", agent_id])
                self.show_feedback(f"Launching Agent {agent_id}...")
        else:
            # For hardware, typically we launch the generic agent-hardware-run
            self.pm.start_process(f"Agent-{agent_id}", ["pixi", "run", "agent-hardware-run"], ["pixi", "run", "agent-hardware-stop"])
            self.show_feedback(f"Launching Agent {agent_id}...")

    def launch_px4(self, agent_id, index):
        if self.is_sim:
            self.pm.start_process(f"PX4-{index}", ["pixi", "run", "simulation-px4-run", str(index)], ["pixi", "run", "simulation-px4-stop", str(index)])
            self.show_feedback(f"Launching PX4 for Agent {agent_id}...")

    def cleanup_simulation(self):
        if self.is_sim:
            self.pm.start_process("Cleanup", ["pixi", "run", "simulation-cleanup"])
            self.show_feedback("Cleaning simulation processes...", 5000)

    def stop_all(self):
        cleanup_cmd = ["pixi", "run", "simulation-cleanup"] if self.is_sim else None
        self.pm.stop_all(cleanup_cmd)
        self.show_feedback("Stopping all processes...", 5000)

    # --- Discovery Callbacks ---

    def add_agent(self, agent_id: str):
        if agent_id in self.agents: return
        
        idx = len(self.agents)
        self.agents[agent_id] = idx
        
        # Create a container for the agent row
        row = QWidget()
        row.setStyleSheet(f"""
            background-color: {COLOR_BACKGROUND_TERTIARY};
            border-radius: 6px;
        """)
        row_layout = QHBoxLayout(row)
        row_layout.setContentsMargins(10, 8, 10, 8)
        row_layout.setSpacing(10)
        
        # Agent Name
        lbl_name = QLabel(agent_id)
        lbl_name.setStyleSheet(f"color: {COLOR_TEXT_PRIMARY}; font-size: 18px; font-weight: bold;")
        row_layout.addWidget(lbl_name, 1)
        
        # Launch Button
        btn_agent = QPushButton("Control")
        btn_agent.setStyleSheet(BUTTON_STYLE_STANDARD)
        btn_agent.setCursor(Qt.PointingHandCursor)
        btn_agent.clicked.connect(lambda: self.launch_agent(agent_id))
        row_layout.addWidget(btn_agent)
        
        if self.is_sim:
            btn_px4 = QPushButton("PX4")
            btn_px4.setStyleSheet(BUTTON_STYLE_SECONDARY)
            btn_px4.setCursor(Qt.PointingHandCursor)
            btn_px4.clicked.connect(lambda: self.launch_px4(agent_id, idx))
            row_layout.addWidget(btn_px4)
            
        self.agent_widgets[agent_id] = row
        self.card_agents.add_widget(row)

    def remove_agent(self, agent_id: str):
        if agent_id in self.agent_widgets:
            widget = self.agent_widgets.pop(agent_id)
            self.card_agents.content_layout.removeWidget(widget)
            widget.deleteLater()
            self.agents.pop(agent_id, None)
