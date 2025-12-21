"""Camera view widget for displaying UDP/RTP video streams"""

from PyQt6.QtWidgets import QWidget, QVBoxLayout, QLabel, QTabWidget, QStackedWidget
from PyQt6.QtCore import Qt, QTimer, QUrl
from PyQt6.QtMultimedia import QMediaPlayer
from PyQt6.QtMultimediaWidgets import QVideoWidget
from typing import Optional
from .styles import (
    LABEL_STYLE_CONNECTING,
    LABEL_STYLE_TITLE_MEDIUM,
    LABEL_STYLE_PLACEHOLDER,
    TAB_WIDGET_STYLE
)

class CameraFeed(QWidget):
    """Widget for a single camera feed using QtMultimedia"""
    
    def __init__(self, port: int = 5000):
        super().__init__()
        
        self.port = port
        self.media_player: Optional[QMediaPlayer] = None
        
        # Create stacked widget to switch between video and status label
        self.stacked_widget = QStackedWidget(self)
        
        # Status label for connection state
        self.status_label = QLabel('Connecting...')
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_label.setStyleSheet(LABEL_STYLE_CONNECTING)
        
        # Video widget for QtMultimedia
        self.video_widget = QVideoWidget()
        self.video_widget.setStyleSheet("background-color: black;")
        
        # Add widgets to stack
        self.stacked_widget.addWidget(self.status_label)  # Index 0
        self.stacked_widget.addWidget(self.video_widget)  # Index 1
        self.stacked_widget.setCurrentIndex(0)  # Show status label initially
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.stacked_widget)
        
        # Try to start stream after widget is shown
        QTimer.singleShot(100, self.connect_stream)
    
    def connect_stream(self):
        """Connect to the video stream using QtMultimedia"""
        try:
            # Create media player
            self.media_player = QMediaPlayer(self)
            self.media_player.setVideoOutput(self.video_widget)
            
            # Connect to error signal to handle connection issues
            self.media_player.errorOccurred.connect(self._handle_error)
            self.media_player.mediaStatusChanged.connect(self._handle_media_status_changed)
            
            # Set up RTP stream URL
            stream_url = QUrl(f"udp://127.0.0.1:{self.port}")
            
            # Set media source
            self.media_player.setSource(stream_url)
            
            # Start playback
            self.media_player.play()
            
            # Switch to video widget after a short delay
            QTimer.singleShot(1000, lambda: self.stacked_widget.setCurrentIndex(1))
                
        except Exception as e:
            self.status_label.setText(f'Error connecting to port {self.port}:\n{str(e)}')
            self.stacked_widget.setCurrentIndex(0)
    
    def _handle_error(self, error, error_string):
        """Handle media player errors"""
        error_msg = error_string if error_string else f"Media player error: {error}"
        self.status_label.setText(f'Error connecting to port {self.port}:\n{error_msg}')
        self.stacked_widget.setCurrentIndex(0)
    
    def _handle_media_status_changed(self, status):
        """Handle media status changes"""
        if status == QMediaPlayer.MediaStatus.LoadedMedia:
            # Media loaded successfully, switch to video widget
            self.stacked_widget.setCurrentIndex(1)
        elif status == QMediaPlayer.MediaStatus.InvalidMedia:
            # Invalid media, show error
            self.status_label.setText(f'Invalid media on port {self.port}')
            self.stacked_widget.setCurrentIndex(0)
    
    def closeEvent(self, event):
        """Clean up when widget is closed"""
        # Stop and cleanup media player
        if self.media_player:
            self.media_player.stop()
            self.media_player.setSource(QUrl())
            self.media_player = None
        
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
        placeholder = QLabel('No camera feeds configured\n\n'
                          'To add a feed, configure UDP/RTP stream')
        placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
        placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
        self.tab_widget.addTab(placeholder, 'No Feeds')

    # ================================ Signal callbacks ================================
    def add_agent(self, agent_id: str, port: int = 5000):
        """Add a camera feed for an agent
        """
        # Remove placeholder if present
        if self.tab_widget.count() == 1:
            widget = self.tab_widget.widget(0)
            if isinstance(widget, QLabel) and 'No camera feeds' in widget.text():
                self.tab_widget.removeTab(0)
        
        # Create feed widget with specified port
        feed_widget = CameraFeed(port=port)
        self.tab_widget.addTab(feed_widget, agent_id)

    def remove_agent(self, agent_id: str):
        """Remove a camera feed for an agent"""
        for i in range(self.tab_widget.count()):
            if self.tab_widget.tabText(i) == agent_id:
                widget = self.tab_widget.widget(i)
                self.tab_widget.removeTab(i)
                if widget:
                    widget.close()
                break
        
        # Add placeholder if no feeds remain
        if self.tab_widget.count() == 0:
            placeholder = QLabel('No camera feeds configured')
            placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
            placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
            self.tab_widget.addTab(placeholder, 'No Feeds')
