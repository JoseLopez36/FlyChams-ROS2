"""Main window for the operator interface dashboard"""

from PyQt6.QtWidgets import QMainWindow, QWidget, QHBoxLayout, QSplitter, QTabWidget
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QPalette, QColor
from .launch_panel import LaunchPanel
from .map_panel import MapPanel
from .camera_panel import CameraPanel
from .styles import (
    SPLITTER_STYLE,
    TAB_WIDGET_STYLE,
    COLOR_BACKGROUND_PRIMARY,
    COLOR_BACKGROUND_SECONDARY
)

class MainWindow(QMainWindow):
    """Main window containing all dashboard components"""
    
    def __init__(self, signals):
        super().__init__()
        
        self.signals = signals
        
        # Apply dark mode theme
        self.apply_dark_theme()
        
        # Configure window
        self.setWindowTitle('FlyChams Operator Interface')
        self.setGeometry(100, 100, 1920, 1080)
        
        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Create main layout
        main_layout = QHBoxLayout(central_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        
        # Create splitter for resizable panels
        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.setStyleSheet(SPLITTER_STYLE)
        main_layout.addWidget(splitter)
        
        # Launch panel
        self.launch_panel = LaunchPanel()
        splitter.addWidget(self.launch_panel)
        
        # Tabbed workspace (browser-like tabs)
        self.tabs = QTabWidget()
        self.tabs.setDocumentMode(True)
        self.tabs.setMovable(True)
        self.tabs.setStyleSheet(TAB_WIDGET_STYLE)

        # Map panel
        self.map_panel = MapPanel(signals)
        self.tabs.addTab(self.map_panel, 'Map')

        # Cameras panel
        self.camera_panel = CameraPanel()
        self.tabs.addTab(self.camera_panel, 'Cameras')

        splitter.addWidget(self.tabs)
        
        # Set relative sizes (launch panel smaller: 1/4, tabs 3/4)
        splitter.setSizes([int(1920 * (1/4)), int(1920 * (3/4))])
        
        # Connect signals for updates
        self.connect_signals()
    
    def apply_dark_theme(self):
        """Apply a comprehensive dark theme to the application"""
        palette = QPalette()
        
        # Convert hex colors to RGB tuples for QColor
        bg_primary_rgb = tuple(int(COLOR_BACKGROUND_PRIMARY[i:i+2], 16) for i in (1, 3, 5))
        bg_secondary_rgb = tuple(int(COLOR_BACKGROUND_SECONDARY[i:i+2], 16) for i in (1, 3, 5))
        
        # Window colors
        palette.setColor(QPalette.ColorRole.Window, QColor(*bg_primary_rgb))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(255, 255, 255))
        
        # Base colors
        palette.setColor(QPalette.ColorRole.Base, QColor(*bg_primary_rgb))
        palette.setColor(QPalette.ColorRole.AlternateBase, QColor(*bg_secondary_rgb))
        
        # Text colors
        palette.setColor(QPalette.ColorRole.Text, QColor(255, 255, 255))
        palette.setColor(QPalette.ColorRole.BrightText, QColor(255, 255, 255))
        
        # Button colors
        palette.setColor(QPalette.ColorRole.Button, QColor(*bg_secondary_rgb))
        palette.setColor(QPalette.ColorRole.ButtonText, QColor(255, 255, 255))
        
        # Highlight colors
        palette.setColor(QPalette.ColorRole.Highlight, QColor(74, 158, 255))
        palette.setColor(QPalette.ColorRole.HighlightedText, QColor(255, 255, 255))
        
        # Disabled colors
        palette.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, QColor(128, 128, 128))
        palette.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, QColor(128, 128, 128))
        palette.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, QColor(128, 128, 128))
        
        self.setPalette(palette)
        
        # Additional global stylesheet
        self.setStyleSheet(f"""
            QMainWindow {{
                background-color: {COLOR_BACKGROUND_PRIMARY};
            }}
            QWidget {{
                background-color: {COLOR_BACKGROUND_PRIMARY};
                color: #ffffff;
            }}
            QLabel {{
                color: #ffffff;
            }}
            QScrollBar:vertical {{
                background-color: {COLOR_BACKGROUND_SECONDARY};
                width: 12px;
                border: none;
            }}
            QScrollBar::handle:vertical {{
                background-color: #4d4d4d;
                min-height: 20px;
                border-radius: 6px;
                margin: 2px;
            }}
            QScrollBar::handle:vertical:hover {{
                background-color: #5d5d5d;
            }}
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
                height: 0px;
            }}
            QScrollBar:horizontal {{
                background-color: {COLOR_BACKGROUND_SECONDARY};
                height: 12px;
                border: none;
            }}
            QScrollBar::handle:horizontal {{
                background-color: #4d4d4d;
                min-width: 20px;
                border-radius: 6px;
                margin: 2px;
            }}
            QScrollBar::handle:horizontal:hover {{
                background-color: #5d5d5d;
            }}
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {{
                width: 0px;
            }}
        """)
    
    def connect_signals(self):
        """Connect Qt signals to GUI update methods"""
        # Agent signals
        self.signals.agent_added.connect(self.add_agent)
        self.signals.agent_removed.connect(self.remove_agent)
        self.signals.agent_position_updated.connect(self.update_agent_position)
        self.signals.agent_setpoint_updated.connect(self.update_agent_setpoint)
        
        # Target signals
        self.signals.target_added.connect(self.add_target)
        self.signals.target_removed.connect(self.remove_target)
        self.signals.target_position_updated.connect(self.update_target_position)
        
        # Cluster signals
        self.signals.cluster_added.connect(self.add_cluster)
        self.signals.cluster_removed.connect(self.remove_cluster)
        self.signals.cluster_geometry_updated.connect(self.update_cluster_geometry)
    
    # ================================ Signal callbacks ================================
    def add_agent(self, agent_id: str):
        self.launch_panel.add_agent(agent_id)
        self.map_panel.add_agent(agent_id)
        self.camera_panel.add_agent(agent_id)
    
    def remove_agent(self, agent_id: str):
        self.launch_panel.remove_agent(agent_id)
        self.map_panel.remove_agent(agent_id)
        self.camera_panel.remove_agent(agent_id)
    
    def update_agent_position(self, agent_id: str, x: float, y: float, z: float):
        self.map_panel.update_agent_position(agent_id, x, y, z)
    
    def update_agent_setpoint(self, agent_id: str, x: float, y: float, z: float):
        self.map_panel.update_agent_setpoint(agent_id, x, y, z)

    def add_target(self, target_id: str):
        self.map_panel.add_target(target_id)
    
    def remove_target(self, target_id: str):
        self.map_panel.remove_target(target_id)
    
    def update_target_position(self, target_id: str, x: float, y: float, z: float):
        self.map_panel.update_target_position(target_id, x, y, z)
    
    def add_cluster(self, cluster_id: str):
        self.map_panel.add_cluster(cluster_id)
    
    def remove_cluster(self, cluster_id: str):
        self.map_panel.remove_cluster(cluster_id)

    def update_cluster_geometry(self, cluster_id: str, center_x: float, center_y: float, center_z: float, radius: float):
        self.map_panel.update_cluster_geometry(cluster_id, center_x, center_y, center_z, radius)

