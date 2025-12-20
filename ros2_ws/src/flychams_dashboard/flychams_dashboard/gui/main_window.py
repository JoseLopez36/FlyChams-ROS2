"""Main window for the operator interface dashboard."""

from PyQt6.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QSplitter
from PyQt6.QtCore import Qt
from .mission_launch_panel import MissionLaunchPanel
from .map_view import MapView
from .camera_view import CameraView


class MainWindow(QMainWindow):
    """Main window containing all dashboard components."""
    
    def __init__(self, operator_interface, signals):
        super().__init__()
        
        self.operator_interface = operator_interface
        self.signals = signals
        
        self.setWindowTitle('FlyChams Operator Interface')
        self.setGeometry(100, 100, 1400, 900)
        
        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Create main layout
        main_layout = QHBoxLayout(central_widget)
        
        # Create splitter for resizable panels
        splitter = QSplitter(Qt.Orientation.Horizontal)
        main_layout.addWidget(splitter)
        
        # Left panel: Mission launch controls
        launch_panel = MissionLaunchPanel()
        splitter.addWidget(launch_panel)
        
        # Right panel: Split into map and camera views
        right_splitter = QSplitter(Qt.Orientation.Vertical)
        
        # Map view (top)
        self.map_view = MapView(operator_interface, signals)
        right_splitter.addWidget(self.map_view)
        
        # Camera view (bottom)
        self.camera_view = CameraView()
        right_splitter.addWidget(self.camera_view)
        
        # Set relative sizes (map gets more space)
        right_splitter.setSizes([600, 300])
        splitter.addWidget(right_splitter)
        
        # Set relative sizes (launch panel smaller)
        splitter.setSizes([300, 1100])
        
        # Connect signals for updates
        self._connect_signals()
    
    def _connect_signals(self):
        """Connect Qt signals to GUI update methods."""
        # Agent signals
        self.signals.agent_added.connect(self.map_view.add_agent)
        self.signals.agent_removed.connect(self.map_view.remove_agent)
        self.signals.agent_position_updated.connect(self.map_view.update_agent_position)
        self.signals.agent_setpoint_updated.connect(self.map_view.update_agent_setpoint)
        
        # Target signals
        self.signals.target_added.connect(self.map_view.add_target)
        self.signals.target_removed.connect(self.map_view.remove_target)
        self.signals.target_position_updated.connect(self.map_view.update_target_position)
        
        # Cluster signals
        self.signals.cluster_added.connect(self.map_view.add_cluster)
        self.signals.cluster_removed.connect(self.map_view.remove_cluster)
        self.signals.cluster_geometry_updated.connect(self.map_view.update_cluster_geometry)
    
    def closeEvent(self, event):
        """Handle window close event."""
        # Cleanup if needed
        event.accept()

