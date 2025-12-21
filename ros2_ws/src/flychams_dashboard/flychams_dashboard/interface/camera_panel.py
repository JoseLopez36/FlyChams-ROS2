"""Camera view widget for displaying UDP/RTP video streams"""

from PyQt6.QtWidgets import QWidget, QVBoxLayout, QLabel, QTabWidget
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QImage, QPixmap
import cv2
from typing import Optional
from .styles import (
    LABEL_STYLE_CONNECTING,
    LABEL_STYLE_TITLE_MEDIUM,
    LABEL_STYLE_PLACEHOLDER,
    TAB_WIDGET_STYLE
)


class CameraFeedWidget(QWidget):
    """Widget for a single camera feed"""
    
    def __init__(self, stream_url: str):
        super().__init__()
        
        self.stream_url = stream_url
        self.cap: Optional[cv2.VideoCapture] = None
        self.label = QLabel('Connecting...')
        self.label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.label.setStyleSheet(LABEL_STYLE_CONNECTING)
        
        layout = QVBoxLayout(self)
        layout.addWidget(self.label)
        
        # Timer for updating frames
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_frame)
        self.timer.start(33)  # ~30 FPS
        
        # Try to open stream
        self.connect_stream()
    
    def connect_stream(self):
        """Connect to the video stream"""
        try:
            self.cap = cv2.VideoCapture(self.stream_url)
            if not self.cap.isOpened():
                self.label.setText(f'Failed to open stream:\n{self.stream_url}')
                self.cap = None
        except Exception as e:
            self.label.setText(f'Error connecting:\n{str(e)}')
            self.cap = None
    
    def update_frame(self):
        """Update the displayed frame"""
        if self.cap is None or not self.cap.isOpened():
            return
        
        ret, frame = self.cap.read()
        if not ret:
            return
        
        # Convert BGR to RGB
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        
        # Resize to fit widget
        height, width, channel = frame_rgb.shape
        bytes_per_line = 3 * width
        q_image = QImage(frame_rgb.data, width, height, bytes_per_line, QImage.Format.Format_RGB888)
        
        # Scale to fit label while maintaining aspect ratio
        pixmap = QPixmap.fromImage(q_image)
        scaled_pixmap = pixmap.scaled(
            self.label.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self.label.setPixmap(scaled_pixmap)
    
    def closeEvent(self, event):
        """Clean up when widget is closed"""
        if self.cap:
            self.cap.release()
        self.timer.stop()
        event.accept()


class CameraPanel(QWidget):
    """Widget for displaying multiple camera feeds"""
    
    def __init__(self):
        super().__init__()
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)
        
        # Title with improved styling
        title = QLabel('Camera Feeds')
        title.setStyleSheet(LABEL_STYLE_TITLE_MEDIUM)
        layout.addWidget(title)
        
        # Tab widget for multiple feeds with improved styling
        self.tab_widget = QTabWidget()
        self.tab_widget.setStyleSheet(TAB_WIDGET_STYLE)
        layout.addWidget(self.tab_widget)
        
        # Placeholder message with improved styling
        placeholder = QLabel('No camera feeds configured.\n\n'
                          'To add a feed, configure UDP/RTP stream URLs.')
        placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
        placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
        self.tab_widget.addTab(placeholder, 'No Feeds')
    
    def add_camera_feed(self, feed_id: str, stream_url: str):
        """Add a camera feed"""
        # Remove placeholder if present
        if self.tab_widget.count() == 1:
            widget = self.tab_widget.widget(0)
            if isinstance(widget, QLabel) and 'No camera feeds' in widget.text():
                self.tab_widget.removeTab(0)
        
        # Create feed widget
        feed_widget = CameraFeedWidget(stream_url)
        self.tab_widget.addTab(feed_widget, feed_id)
    
    def remove_camera_feed(self, feed_id: str):
        """Remove a camera feed"""
        for i in range(self.tab_widget.count()):
            if self.tab_widget.tabText(i) == feed_id:
                widget = self.tab_widget.widget(i)
                self.tab_widget.removeTab(i)
                if widget:
                    widget.close()
                break
        
        # Add placeholder if no feeds remain
        if self.tab_widget.count() == 0:
            placeholder = QLabel('No camera feeds configured.')
            placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
            placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
            self.tab_widget.addTab(placeholder, 'No Feeds')

    def add_agent(self, agent_id: str):
        """Add a camera feed for an agent"""
        # TODO: Implement
        pass

    def remove_agent(self, agent_id: str):
        """Remove a camera feed for an agent"""
        # TODO: Implement
        pass
