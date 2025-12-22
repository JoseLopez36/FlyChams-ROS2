"""Camera view widget for displaying UDP/RTP video streams"""

import logging
from PyQt6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QLabel, QTabWidget, QStackedWidget, QSizePolicy, QSpacerItem
from PyQt6.QtCore import Qt, QTimer, QUrl, QSize, QEvent
from PyQt6.QtMultimedia import QMediaPlayer
from PyQt6.QtMultimediaWidgets import QVideoWidget
from typing import Optional
from .styles import (
    LABEL_STYLE_CONNECTING,
    LABEL_STYLE_TITLE_MEDIUM,
    LABEL_STYLE_PLACEHOLDER,
    TAB_WIDGET_STYLE
)

logger = logging.getLogger(__name__)

class CameraFeed(QWidget):
    """Widget for a single camera feed using QtMultimedia"""
    
    def __init__(self, stream_url: Optional[str] = None):
        super().__init__()
        
        self.stream_url = stream_url
        self.media_player: Optional[QMediaPlayer] = None

        if stream_url:
            logger.info(f"Creating camera feed for stream: {stream_url}")
        else:
            logger.debug("Creating placeholder camera feed (no stream URL)")
        
        # Create stacked widget to switch between video and status label
        self.stacked_widget = QStackedWidget(self)
        
        # Status label for connection state
        self.status_label = QLabel('No feed' if stream_url is None else 'Connecting...')
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
        if stream_url:
            QTimer.singleShot(100, self.connect_stream)
    
    def connect_stream(self):
        """Connect to the video stream using QtMultimedia"""
        logger.info(f"Connecting to stream: {self.stream_url}")
        try:
            # Create media player
            self.media_player = QMediaPlayer(self)
            self.media_player.setVideoOutput(self.video_widget)
            
            # Connect to error signal to handle connection issues
            self.media_player.errorOccurred.connect(self.handle_error)
            self.media_player.mediaStatusChanged.connect(self.handle_media_status_changed)
            
            # Set up RTP stream URL
            stream_url = QUrl(self.stream_url)
            
            # Set media source
            self.media_player.setSource(stream_url)
            
            # Start playback
            self.media_player.play()
            
            # Switch to video widget after a short delay
            QTimer.singleShot(1000, lambda: self.stacked_widget.setCurrentIndex(1))
                
        except Exception as e:
            logger.error(f"Error connecting to stream {self.stream_url}: {e}")
            self.status_label.setText(f'Error connecting to stream {self.stream_url}:\n{str(e)}')
            self.stacked_widget.setCurrentIndex(0)
    
    # ================================ Callback handling ================================
    def handle_error(self, error, error_string):
        """Handle media player errors"""
        error_msg = error_string if error_string else f"Media player error: {error}"
        logger.error(f"Media player error for stream {self.stream_url}: {error_msg}")
        self.status_label.setText(f'Error connecting to stream {self.stream_url}:\n{error_msg}')
        self.stacked_widget.setCurrentIndex(0)
    
    def handle_media_status_changed(self, status):
        """Handle media status changes"""
        if status == QMediaPlayer.MediaStatus.LoadedMedia:
            # Media loaded successfully, switch to video widget
            logger.info(f"Media loaded successfully for stream: {self.stream_url}")
            self.stacked_widget.setCurrentIndex(1)
        elif status == QMediaPlayer.MediaStatus.InvalidMedia:
            # Invalid media, show error
            logger.warning(f"Invalid media for stream: {self.stream_url}")
            self.status_label.setText(f'Invalid media on stream {self.stream_url}')
            self.stacked_widget.setCurrentIndex(0)

    # ================================ Qt methods ================================
    def closeEvent(self, event):
        """Clean up when widget is closed"""
        logger.debug(f"Closing camera feed for stream: {self.stream_url}")
        # Stop and cleanup media player
        if self.media_player:
            self.media_player.stop()
            self.media_player.setSource(QUrl())
            self.media_player = None
        
        event.accept()

class AgentCameraComposition(QWidget):
    """Widget displaying camera feeds for a single agent: camera 0 (50% vertical) + cameras 1-4 (12.5% each)"""
    
    def __init__(self, agent_id: str, stream_urls: list):
        super().__init__()
        
        self.agent_id = agent_id
        self.camera_feeds = []
        self.fixed_widgets = []  # Store fixed aspect ratio wrappers
        
        logger.info(f"Creating camera composition for agent '{agent_id}' with {len(stream_urls)} stream(s)")
        
        # Create camera 0 feed (takes 2/4 = 50% of vertical space)
        if len(stream_urls) > 0:
            camera_0_feed = CameraFeed(stream_urls[0])
        else:
            camera_0_feed = CameraFeed(None)
        self.camera_feeds.append(camera_0_feed)
        
        # Create cameras 1-4 feeds (each takes 1/4 of remaining 50% = 12.5% each)
        for i in range(4):
            camera_idx = i + 1  # Cameras 1-4
            if camera_idx < len(stream_urls):
                feed = CameraFeed(stream_urls[camera_idx])
            else:
                feed = CameraFeed(None)  # Placeholder for non-existent camera
            self.camera_feeds.append(feed)
        
        # Create main horizontal layout
        main_layout = QHBoxLayout(self)
        main_layout.setSpacing(10)
        
        # Add left spacer to center content horizontally
        left_spacer = QSpacerItem(0, 0, QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Minimum)
        main_layout.addItem(left_spacer)
        
        # Add camera 0 feed to main layout
        main_layout.addWidget(self.camera_feeds[0])

        # Create vertical layout for camera feeds 1 to 4
        camera_1_to_4_feeds_layout = QVBoxLayout()
        camera_1_to_4_feeds_layout.setSpacing(10)
        
        # Add camera 1 to 4 feeds to layout
        for i in range(1, 5):
            camera_1_to_4_feeds_layout.addWidget(self.camera_feeds[i])

        # Add camera 1 to 4 feeds layout to main layout
        main_layout.addLayout(camera_1_to_4_feeds_layout)
        
        # Add right spacer to center content horizontally
        right_spacer = QSpacerItem(0, 0, QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Minimum)
        main_layout.addItem(right_spacer)
    
    def closeEvent(self, event):
        """Clean up when widget is closed"""
        logger.debug(f"Closing camera composition for agent '{self.agent_id}'")
        # Close all camera feeds
        for feed in self.camera_feeds:
            feed.close()
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
                          'To add a feed, configure a stream')
        placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
        placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
        self.tab_widget.addTab(placeholder, 'No Feeds')

    # ================================ Signal callbacks ================================
    def add_agent(self, agent_id: str, stream_urls: list):
        """Add camera feeds for an agent (one tab per agent with composition layout)"""
        logger.info(f"Adding agent '{agent_id}' with {len(stream_urls)} stream(s)")
        
        # Remove placeholder if present
        if self.tab_widget.count() == 1:
            widget = self.tab_widget.widget(0)
            if isinstance(widget, QLabel) and 'No camera feeds' in widget.text():
                self.tab_widget.removeTab(0)
        
        # Check if agent tab already exists (remove it first)
        for i in range(self.tab_widget.count()):
            if self.tab_widget.tabText(i) == agent_id:
                logger.debug(f"Replacing existing tab for agent '{agent_id}'")
                widget = self.tab_widget.widget(i)
                self.tab_widget.removeTab(i)
                if widget:
                    widget.close()
                break
        
        # Create composition widget for this agent (1 large + 4 small feeds)
        composition_widget = AgentCameraComposition(agent_id, stream_urls)
        self.tab_widget.addTab(composition_widget, agent_id)

    def remove_agent(self, agent_id: str):
        """Remove camera feeds for an agent"""
        logger.info(f"Removing agent '{agent_id}'")
        
        for i in range(self.tab_widget.count()):
            if self.tab_widget.tabText(i) == agent_id:
                widget = self.tab_widget.widget(i)
                self.tab_widget.removeTab(i)
                if widget:
                    widget.close()
                break
        
        # Add placeholder if no feeds remain
        if self.tab_widget.count() == 0:
            logger.debug("No agents remaining, adding placeholder")
            placeholder = QLabel('No camera feeds configured\n\n'
                              'To add a feed, configure UDP/RTP stream')
            placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
            placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
            self.tab_widget.addTab(placeholder, 'No Feeds')
