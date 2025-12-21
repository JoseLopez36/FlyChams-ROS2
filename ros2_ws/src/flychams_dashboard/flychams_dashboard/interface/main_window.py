"""Main window for the operator interface dashboard"""

from PyQt6.QtWidgets import QMainWindow, QWidget, QHBoxLayout, QSplitter, QTabWidget
from PyQt6.QtCore import Qt
from .launch_panel import LaunchPanel
from .map_panel import MapPanel
from .camera_panel import CameraPanel

class MainWindow(QMainWindow):
    """Main window containing all dashboard components"""
    
    def __init__(self, signals):
        super().__init__()
        
        self.signals = signals
        
        # Configure window
        self.setWindowTitle('FlyChams Operator Interface')
        self.setGeometry(100, 100, 1920, 1080)
        
        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Create main layout
        main_layout = QHBoxLayout(central_widget)
        
        # Create splitter for resizable panels
        splitter = QSplitter(Qt.Orientation.Horizontal)
        main_layout.addWidget(splitter)
        
        # Launch panel
        self.launch_panel = LaunchPanel()
        splitter.addWidget(self.launch_panel)
        
        # Tabbed workspace (browser-like tabs)
        self.tabs = QTabWidget()
        self.tabs.setDocumentMode(True)
        self.tabs.setMovable(True)

        # Map panel
        self.map_panel = MapPanel(signals)
        self.tabs.addTab(self.map_panel, 'Map')

        # Cameras panel
        self.camera_panel = CameraPanel()
        self.tabs.addTab(self.camera_panel, 'Cameras')

        splitter.addWidget(self.tabs)
        
        # Set relative sizes (launch panel 1/3, tabs 2/3)
        splitter.setSizes([int(1920 * (1/3)), int(1920 * (2/3))])
        
        # Connect signals for updates
        self.connect_signals()
    
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

