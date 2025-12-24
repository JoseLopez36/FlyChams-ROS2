"""Main window for the operator interface operator"""

from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QStackedWidget, 
    QToolBar, QPushButton, QLabel, QDockWidget, QListWidget, QListWidgetItem,
    QApplication
)
from PyQt5.QtCore import Qt, QSize, QPropertyAnimation, QEasingCurve, QParallelAnimationGroup
from PyQt5.QtGui import QPalette, QColor, QIcon
from .control_panel import ControlPanel
from .logs_panel import LogsPanel
from .map_panel import MapPanel
from .monitoring_panel import MonitoringPanel
from .process_manager import ProcessManager
from .styles import (
    COLOR_BACKGROUND_PRIMARY,
    COLOR_BACKGROUND_SECONDARY,
    NAV_DRAWER_STYLE,
    TOOLBAR_STYLE,
    HAMBURGER_BUTTON_STYLE
)

class MainWindow(QMainWindow):
    """Main window containing all operator components with hamburger navigation"""
    
    def __init__(self, signals, is_sim: bool):
        super().__init__()
        
        self.signals = signals
        self.is_sim = is_sim
        
        # Initialize Process Manager
        self.process_manager = ProcessManager()
        
        # Apply dark mode theme
        self.apply_dark_theme()
        
        # Configure window
        self.setWindowTitle('FLYCHAMS OPERATOR INTERFACE')
        
        # Get screen geometry and set 16:9 aspect ratio based on full height
        screen = QApplication.primaryScreen()
        if screen:
            screen_geometry = screen.availableGeometry()
            display_height = screen_geometry.height()
            target_width = int(display_height * 16 / 9)
            self.resize(target_width, display_height)
        else:
            self.resize(1920, 1080)
            
        self.setMinimumSize(1200, 800)
        
        # Create UI components
        self.setup_ui()
        
        # Connect signals for updates
        self.connect_signals()
    
    def setup_ui(self):
        # 1. Toolbar with Hamburger
        self.toolbar = QToolBar()
        self.toolbar.setMovable(False)
        self.toolbar.setStyleSheet(TOOLBAR_STYLE)
        self.addToolBar(Qt.TopToolBarArea, self.toolbar)
        
        self.hamburger_btn = QPushButton("☰")
        self.hamburger_btn.setStyleSheet(HAMBURGER_BUTTON_STYLE)
        self.hamburger_btn.clicked.connect(self.toggle_nav_drawer)
        self.toolbar.addWidget(self.hamburger_btn)
        
        title_lbl = QLabel("FLYCHAMS OPERATOR INTERFACE")
        title_lbl.setObjectName("ToolbarTitle")
        self.toolbar.addWidget(title_lbl)
        
        # 2. Nav Drawer (DockWidget)
        self.nav_drawer = QDockWidget("Navigation", self)
        self.nav_drawer.setFeatures(QDockWidget.NoDockWidgetFeatures)
        self.nav_drawer.setTitleBarWidget(QWidget()) # Hide title bar
        
        self.nav_list = QListWidget()
        self.nav_list.setStyleSheet(NAV_DRAWER_STYLE)
        self.nav_list.setMinimumWidth(0)
        self.nav_list.setMaximumWidth(300)
        
        # Setup animation
        self.animation = QPropertyAnimation(self.nav_drawer, b"maximumWidth")
        self.animation.setDuration(150)
        self.animation.setEasingCurve(QEasingCurve.InOutQuart)
        
        self.min_animation = QPropertyAnimation(self.nav_drawer, b"minimumWidth")
        self.min_animation.setDuration(150)
        self.min_animation.setEasingCurve(QEasingCurve.InOutQuart)

        self.anim_group = QParallelAnimationGroup()
        self.anim_group.addAnimation(self.animation)
        self.anim_group.addAnimation(self.min_animation)
        
        # Add nav items
        self.control_panel = ControlPanel(self.process_manager, self.is_sim)
        self.logs_panel = LogsPanel(self.process_manager)
        self.map_panel = MapPanel(self.signals)
        self.monitoring_panel = MonitoringPanel()

        self.pages = [
            ("Mission Control", self.control_panel),
            ("System Logs", self.logs_panel),
            ("Real-Time Map", self.map_panel),
            ("Monitoring Feeds", self.monitoring_panel)
        ]
        
        for name, widget in self.pages:
            item = QListWidgetItem(name)
            self.nav_list.addItem(item)
            
        self.nav_drawer.setWidget(self.nav_list)
        self.addDockWidget(Qt.LeftDockWidgetArea, self.nav_drawer)
        
        # 3. Central Stacked Widget
        self.central_stack = QStackedWidget()
        for name, widget in self.pages:
            self.central_stack.addWidget(widget)
            
        self.setCentralWidget(self.central_stack)
        
        # Start at Control page
        self.nav_list.setCurrentRow(0)

        # Menu initial state
        self.nav_drawer.setMinimumWidth(0)
        self.nav_drawer.setMaximumWidth(0)
        self.nav_list.currentRowChanged.connect(self.switch_page)

    def toggle_nav_drawer(self):
        is_expanded = self.nav_drawer.maximumWidth() > 0
        
        if is_expanded:
            # Close: animate from 200 to 0
            self.animation.setStartValue(300)
            self.animation.setEndValue(0)
            self.min_animation.setStartValue(300)
            self.min_animation.setEndValue(0)
        else:
            # Open: animate from 0 to 200
            self.animation.setStartValue(0)
            self.animation.setEndValue(300)
            self.min_animation.setStartValue(0)
            self.min_animation.setEndValue(300)
            
        self.anim_group.start()

    def switch_page(self, index):
        self.central_stack.setCurrentIndex(index)

        # Hide the nav drawer
        self.toggle_nav_drawer()

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
        palette.setColor(QPalette.ColorRole.Highlight, QColor(0, 122, 122))  # Lighter Prussian Green
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
    def add_agent(self, agent_id: str, stream_urls: list):
        self.control_panel.add_agent(agent_id)
        self.map_panel.add_agent(agent_id)
        self.monitoring_panel.add_agent(agent_id, stream_urls)
    
    def remove_agent(self, agent_id: str):
        self.control_panel.remove_agent(agent_id)
        self.map_panel.remove_agent(agent_id)
        self.monitoring_panel.remove_agent(agent_id)
    
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

