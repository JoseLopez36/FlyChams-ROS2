"""Monitoring panel: central camera stream + tracking views. Supports both multi-camera and multi-window tracking approaches"""

import logging
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QTabWidget, QLabel
from PyQt5.QtCore import Qt
from PyQt5.QtMultimedia import QMediaPlayer, QMediaContent
from PyQt5.QtMultimediaWidgets import QVideoWidget
from PyQt5.QtCore import QUrl
from typing import Dict
from .styles import (
    LABEL_STYLE_TITLE_MEDIUM,
    LABEL_STYLE_PLACEHOLDER,
    TAB_WIDGET_STYLE
)

logger = logging.getLogger(__name__)

class Feed(QWidget):
    def __init__(self, stream_url: str = None):
        super().__init__()

        # Create the video widget (output)
        self.video_widget = QVideoWidget()
        self.video_widget.setStyleSheet("background-color: black;")
        
        # Add layout to show the video widget and a label
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setContentsMargins(2, 2, 2, 2)
        
        self.label = QLabel("No Stream")
        self.label.setStyleSheet(LABEL_STYLE_TITLE_MEDIUM)
        self.label.setAlignment(Qt.AlignCenter)
        self.main_layout.addWidget(self.label)
        
        self.main_layout.addWidget(self.video_widget)
        self.main_layout.setStretch(1, 1) # Video widget takes most space

        # Create the media player
        self.player = QMediaPlayer(None, QMediaPlayer.VideoSurface)
        self.player.setVideoOutput(self.video_widget)
        
        # Connect error handling
        self.player.error.connect(self._handle_error)

        if stream_url:
            # Set source and play
            logger.info(f"Setting source for feed to {stream_url}")
            self.player.setMedia(QMediaContent(QUrl(stream_url)))
            self.player.play()
        else:
            logger.debug("Feed created without stream URL")

    def _handle_error(self):
        err_msg = self.player.errorString()
        logger.error(f"MediaPlayer Error: {err_msg}")
        self.label.setText(f"Error: {err_msg}")

    def set_label(self, label: str):
        self.label.setText(label)

    def close(self):
        self.player.stop()
        self.player.setMedia(QMediaContent())

class CentralFeed(Feed):
    """Central feed for a single agent"""
    def __init__(self, stream_url: str = None):
        super().__init__(stream_url)

class TrackingFeed(Feed):
    """Tracking feed for a single agent"""
    def __init__(self, stream_url: str = None):
        super().__init__(stream_url)

class AgentCameraComposition(QWidget):
    """
    Widget displaying camera feeds for a single agent:
    - 1 large central feed
    - 2x2 grid of tracking feeds
    """
    
    def __init__(self, agent_id: str, stream_urls: list):
        super().__init__()
        self.agent_id = agent_id
        
        logger.info(f"Creating camera composition for agent '{agent_id}' with {len(stream_urls)} stream(s)")
        
        # Labels for the feeds
        labels = [
            "CENTRAL FEED",
            "TRACKING FEED 1",
            "TRACKING FEED 2",
            "TRACKING FEED 3",
            "TRACKING FEED 4"
        ]

        # Main layout
        main_layout = QHBoxLayout(self)
        main_layout.setSpacing(10)

        # 1. Central feed
        central_url = stream_urls[0] if len(stream_urls) > 0 else None
        self.central_feed = CentralFeed(central_url)
        self.central_feed.set_label(f"{agent_id} - {labels[0]}")
        main_layout.addWidget(self.central_feed, stretch=3)
        
        # 2. Tracking feeds grid (2x2 layout)
        grid_layout = QGridLayout()
        grid_layout.setSpacing(10)
        
        self.tracking_feeds = []
        for i in range(1, 5):
            url = stream_urls[i] if i < len(stream_urls) else None
            feed = TrackingFeed(url)
            feed.set_label(labels[i])
            self.tracking_feeds.append(feed)
            
            # Arrange in 2x2 grid: (1,1)->(0,0), (1,2)->(0,1), (1,3)->(1,0), (1,4)->(1,1)
            row = (i - 1) // 2
            col = (i - 1) % 2
            grid_layout.addWidget(feed, row, col)
        
        # Ensure all feeds in the grid have the same size
        grid_layout.setRowStretch(0, 1)
        grid_layout.setRowStretch(1, 1)
        grid_layout.setColumnStretch(0, 1)
        grid_layout.setColumnStretch(1, 1)
        
        # Add the grid layout to main layout with a smaller stretch
        main_layout.addLayout(grid_layout, stretch=2)

    # ================================ Qt methods ================================
    def closeEvent(self, event):
        """Clean up when widget is closed"""
        logger.debug(f"Closing camera composition for agent '{self.agent_id}'")
        # Close all feeds
        self.central_feed.close()
        for feed in self.tracking_feeds:
            feed.close()
        event.accept()

class MonitoringPanel(QWidget):
    """Widget for displaying multiple camera feeds"""
    
    def __init__(self):
        super().__init__()

        self.agent_widgets: Dict[str, AgentCameraComposition] = {}
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)
        
        # Title
        title = QLabel('Monitoring Feeds')
        title.setStyleSheet(LABEL_STYLE_TITLE_MEDIUM)
        layout.addWidget(title)
        
        # Tab widget for multiple feeds
        self.tab_widget = QTabWidget()
        self.tab_widget.setStyleSheet(TAB_WIDGET_STYLE)
        layout.addWidget(self.tab_widget)
        
        # Placeholder message
        placeholder = QLabel('No camera feeds configured\n\n'
                          'To add a feed, configure a stream')
        placeholder.setAlignment(Qt.AlignCenter)
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
        self.agent_widgets[agent_id] = composition_widget
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
        self.agent_widgets.pop(agent_id, None)
        
        # Add placeholder if no feeds remain
        if self.tab_widget.count() == 0:
            logger.debug("No agents remaining, adding placeholder")
            placeholder = QLabel('No camera feeds configured\n\n'
                              'To add a feed, configure UDP/RTP stream')
            placeholder.setAlignment(Qt.AlignCenter)
            placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
            self.tab_widget.addTab(placeholder, 'No Feeds')