"""Top-down map view for visualizing agents, targets, and clusters"""

from PyQt6.QtWidgets import QWidget
from PyQt6.QtCore import Qt, QPointF
from PyQt6.QtGui import QPainter, QPen, QBrush, QColor, QFont
from typing import Dict
from geometry_msgs.msg import Point


class MapPanel(QWidget):
    """Widget for displaying top-down 2D map visualization"""
    
    def __init__(self, signals):
        super().__init__()
        
        self.signals = signals
        
        # Data storage
        self.agents: Dict[str, Dict] = {}  # agent_id -> {position, setpoint}
        self.targets: Dict[str, Dict] = {}  # target_id -> {position}
        self.clusters: Dict[str, Dict] = {}  # cluster_id -> {center, radius}
        
        # View settings
        self.scale = 50.0  # pixels per meter
        self.offset_x = 0.0
        self.offset_y = 0.0
        
        self.setMinimumSize(400, 400)
        self.setStyleSheet('background-color: #1e1e1e;')
    
    def paintEvent(self, event):
        """Paint the map visualization"""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Get widget dimensions
        width = self.width()
        height = self.height()
        
        # Center point
        center_x = width / 2.0 + self.offset_x
        center_y = height / 2.0 + self.offset_y
        
        # Draw grid
        self._draw_grid(painter, width, height, center_x, center_y)
        
        # Draw clusters first (background)
        for cluster_id, cluster_data in self.clusters.items():
            if cluster_data.get('has_geometry', False):
                self._draw_cluster(painter, cluster_id, cluster_data, center_x, center_y)
        
        # Draw targets
        for target_id, target_data in self.targets.items():
            if target_data.get('has_position', False):
                self._draw_target(painter, target_id, target_data, center_x, center_y)
        
        # Draw agents and setpoints
        for agent_id, agent_data in self.agents.items():
            # Draw setpoint line if both position and setpoint exist
            if agent_data.get('has_position', False) and agent_data.get('has_setpoint', False):
                self._draw_setpoint_line(painter, agent_data, center_x, center_y)
            
            # Draw agent
            if agent_data.get('has_position', False):
                self._draw_agent(painter, agent_id, agent_data, center_x, center_y)

        # If there's no data yet, show a subtle placeholder label.
        if not self._has_any_drawable_data():
            self._draw_placeholder(painter, width, height)

    def _has_any_drawable_data(self) -> bool:
        """Return True if there is any data worth drawing (positions/geometry)."""
        for agent_data in self.agents.values():
            if agent_data.get('has_position', False) or agent_data.get('has_setpoint', False):
                return True
        for target_data in self.targets.values():
            if target_data.get('has_position', False):
                return True
        for cluster_data in self.clusters.values():
            if cluster_data.get('has_geometry', False):
                return True
        return False

    def _draw_placeholder(self, painter: QPainter, width: int, height: int) -> None:
        """Draw centered placeholder text when the map is empty."""
        painter.save()
        font = QFont()
        font.setPointSize(12)
        painter.setFont(font)
        painter.setPen(QPen(QColor(160, 160, 160)))
        painter.drawText(0, 0, width, height, int(Qt.AlignmentFlag.AlignCenter), "Map view (empty)")
        painter.restore()
    
    def _draw_grid(self, painter: QPainter, width: int, height: int, center_x: float, center_y: float):
        """Draw a grid background"""
        pen = QPen(QColor(60, 60, 60), 1)
        painter.setPen(pen)
        
        # Vertical lines
        grid_spacing = 50.0  # pixels
        for x in range(0, width + int(grid_spacing), int(grid_spacing)):
            painter.drawLine(x, 0, x, height)
        
        # Horizontal lines
        for y in range(0, height + int(grid_spacing), int(grid_spacing)):
            painter.drawLine(0, y, width, y)
        
        # Center crosshair
        pen = QPen(QColor(100, 100, 100), 2)
        painter.setPen(pen)
        painter.drawLine(int(center_x - 10), int(center_y), int(center_x + 10), int(center_y))
        painter.drawLine(int(center_x), int(center_y - 10), int(center_x), int(center_y + 10))
    
    def _draw_agent(self, painter: QPainter, agent_id: str, agent_data: Dict, center_x: float, center_y: float):
        """Draw an agent as a circle with ID label"""
        pos = agent_data.get('position')
        if not pos:
            return
        
        # Convert to screen coordinates
        x = center_x + pos.x * self.scale
        y = center_y - pos.y * self.scale  # Flip Y axis
        
        # Draw agent circle
        pen = QPen(QColor(0, 150, 255), 2)
        brush = QBrush(QColor(0, 150, 255, 100))
        painter.setPen(pen)
        painter.setBrush(brush)
        radius = 8
        painter.drawEllipse(int(x - radius), int(y - radius), radius * 2, radius * 2)
        
        # Draw agent ID label
        font = QFont()
        font.setPointSize(8)
        painter.setFont(font)
        pen = QPen(QColor(255, 255, 255))
        painter.setPen(pen)
        painter.drawText(int(x + radius + 2), int(y), agent_id)
    
    def _draw_setpoint_line(self, painter: QPainter, agent_data: Dict, center_x: float, center_y: float):
        """Draw a dashed line from agent position to setpoint"""
        pos = agent_data.get('position')
        setpoint = agent_data.get('setpoint')
        if not pos or not setpoint:
            return
        
        # Convert to screen coordinates
        x1 = center_x + pos.x * self.scale
        y1 = center_y - pos.y * self.scale
        x2 = center_x + setpoint.x * self.scale
        y2 = center_y - setpoint.y * self.scale
        
        # Draw dashed line
        pen = QPen(QColor(255, 200, 0), 1, Qt.PenStyle.DashLine)
        painter.setPen(pen)
        painter.drawLine(int(x1), int(y1), int(x2), int(y2))
        
        # Draw setpoint marker
        pen = QPen(QColor(255, 200, 0), 2)
        brush = QBrush(QColor(255, 200, 0, 150))
        painter.setPen(pen)
        painter.setBrush(brush)
        radius = 5
        painter.drawEllipse(int(x2 - radius), int(y2 - radius), radius * 2, radius * 2)
    
    def _draw_target(self, painter: QPainter, target_id: str, target_data: Dict, center_x: float, center_y: float):
        """Draw a target as a triangle marker"""
        pos = target_data.get('position')
        if not pos:
            return
        
        # Convert to screen coordinates
        x = center_x + pos.x * self.scale
        y = center_y - pos.y * self.scale
        
        # Draw target triangle
        pen = QPen(QColor(255, 0, 0), 2)
        brush = QBrush(QColor(255, 0, 0, 150))
        painter.setPen(pen)
        painter.setBrush(brush)
        size = 10
        points = [
            QPointF(x, y - size),
            QPointF(x - size, y + size),
            QPointF(x + size, y + size)
        ]
        painter.drawPolygon(points)
        
        # Draw target ID label
        font = QFont()
        font.setPointSize(8)
        painter.setFont(font)
        pen = QPen(QColor(255, 255, 255))
        painter.setPen(pen)
        painter.drawText(int(x + size + 2), int(y), target_id)
    
    def _draw_cluster(self, painter: QPainter, cluster_id: str, cluster_data: Dict, center_x: float, center_y: float):
        """Draw a cluster as a circle with radius"""
        center = cluster_data.get('center')
        radius = cluster_data.get('radius', 0.0)
        if not center or radius <= 0:
            return
        
        # Convert to screen coordinates
        x = center_x + center.x * self.scale
        y = center_y - center.y * self.scale
        screen_radius = radius * self.scale
        
        # Draw cluster circle
        pen = QPen(QColor(150, 0, 255), 2, Qt.PenStyle.DashLine)
        brush = QBrush(QColor(150, 0, 255, 50))
        painter.setPen(pen)
        painter.setBrush(brush)
        painter.drawEllipse(int(x - screen_radius), int(y - screen_radius), 
                          int(screen_radius * 2), int(screen_radius * 2))
        
        # Draw cluster center
        pen = QPen(QColor(150, 0, 255), 2)
        brush = QBrush(QColor(150, 0, 255))
        painter.setPen(pen)
        painter.setBrush(brush)
        center_size = 4
        painter.drawEllipse(int(x - center_size), int(y - center_size), 
                          center_size * 2, center_size * 2)
    
    def add_agent(self, agent_id: str):
        """Add an agent to the visualization"""
        self.agents[agent_id] = {
            'has_position': False,
            'has_setpoint': False,
            'position': None,
            'setpoint': None
        }
        self.update()
    
    def remove_agent(self, agent_id: str):
        """Remove an agent from the visualization"""
        if agent_id in self.agents:
            del self.agents[agent_id]
        self.update()
    
    def update_agent_position(self, agent_id: str, x: float, y: float, z: float):
        """Update agent position"""
        if agent_id not in self.agents:
            self.add_agent(agent_id)
        
        point = Point()
        point.x = x
        point.y = y
        point.z = z
        self.agents[agent_id]['position'] = point
        self.agents[agent_id]['has_position'] = True
        self.update()
    
    def update_agent_setpoint(self, agent_id: str, x: float, y: float, z: float):
        """Update agent setpoint"""
        if agent_id not in self.agents:
            self.add_agent(agent_id)
        
        point = Point()
        point.x = x
        point.y = y
        point.z = z
        self.agents[agent_id]['setpoint'] = point
        self.agents[agent_id]['has_setpoint'] = True
        self.update()
    
    def add_target(self, target_id: str):
        """Add a target to the visualization"""
        self.targets[target_id] = {
            'has_position': False,
            'position': None
        }
        self.update()
    
    def remove_target(self, target_id: str):
        """Remove a target from the visualization"""
        if target_id in self.targets:
            del self.targets[target_id]
        self.update()
    
    def update_target_position(self, target_id: str, x: float, y: float, z: float):
        """Update target position"""
        if target_id not in self.targets:
            self.add_target(target_id)
        
        point = Point()
        point.x = x
        point.y = y
        point.z = z
        self.targets[target_id]['position'] = point
        self.targets[target_id]['has_position'] = True
        self.update()
    
    def add_cluster(self, cluster_id: str):
        """Add a cluster to the visualization"""
        self.clusters[cluster_id] = {
            'has_geometry': False,
            'center': None,
            'radius': 0.0
        }
        self.update()
    
    def remove_cluster(self, cluster_id: str):
        """Remove a cluster from the visualization"""
        if cluster_id in self.clusters:
            del self.clusters[cluster_id]
        self.update()
    
    def update_cluster_geometry(self, cluster_id: str, center_x: float, center_y: float, center_z: float, radius: float):
        """Update cluster geometry"""
        if cluster_id not in self.clusters:
            self.add_cluster(cluster_id)
        
        center = Point()
        center.x = center_x
        center.y = center_y
        center.z = center_z
        
        self.clusters[cluster_id]['center'] = center
        self.clusters[cluster_id]['radius'] = radius
        self.clusters[cluster_id]['has_geometry'] = True
        self.update()

