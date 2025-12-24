"""Monitoring panel: central camera stream + tracking views. Supports both multi-camera and multi-window tracking approaches"""

import logging
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QTabWidget, QLabel,
    QMenu, QAction, QToolButton, QStackedWidget
)
from PyQt5.QtCore import Qt, QUrl, pyqtSignal, QTimer
from PyQt5.QtMultimedia import QMediaPlayer, QMediaContent
from PyQt5.QtMultimediaWidgets import QVideoWidget
from typing import Dict, Optional
from .styles import (
    LABEL_STYLE_TITLE_MEDIUM,
    LABEL_STYLE_PLACEHOLDER,
    TAB_WIDGET_STYLE,
    LABEL_STYLE_CONNECTING
)

logger = logging.getLogger(__name__)

class Feed(QWidget):
    """Feed widget with header, menu and status handling"""

    def __init__(self, stream_url: Optional[str] = None, label_text: Optional[str] = None):
        super().__init__()
        
        self.stream_url = stream_url
        self.label_text = label_text or ""
        self.player: Optional[QMediaPlayer] = None

        # Main layout for the feed (header + video/status)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        # Header bar
        header_widget = QWidget()
        header_widget.setStyleSheet("background-color: #2d2d2d; border-top-left-radius: 4px; border-top-right-radius: 4px;")
        header_layout = QHBoxLayout(header_widget)
        header_layout.setContentsMargins(8, 4, 8, 4)
        
        self.title_label = QLabel(self.label_text.upper())
        self.title_label.setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold; border: none;")
        header_layout.addWidget(self.title_label)
        
        # Menu
        self.menu = QMenu(self)
        self.menu.setStyleSheet("""
            QMenu {
                background-color: #2d2d2d;
                color: white;
                border: 1px solid #444;
            }
            QMenu::item {
                padding: 4px 20px;
            }
            QMenu::item:selected {
                background-color: #3d3d3d;
            }
        """)
        
        make_main_action = QAction("Make main feed", self)
        self.menu.addAction(make_main_action)

        # Menu button
        self.menu_button = QToolButton()
        self.menu_button.setText("⋮")
        self.menu_button.setMenu(self.menu)
        self.menu_button.setPopupMode(QToolButton.InstantPopup)
        self.menu_button.setStyleSheet("""
            QToolButton {
                color: #ffffff; 
                font-size: 18px; 
                border: none; 
                background: transparent;
                font-weight: bold;
                padding: 0px;
                margin: 0px;
                min-width: 24px;
                max-width: 24px;
                min-height: 24px;
                max-height: 24px;
            }
            QToolButton:hover {
                background-color: rgba(255, 255, 255, 0.1);
                border-radius: 4px;
            }
            QToolButton::menu-indicator {
                image: none;
            }
        """)
        
        header_layout.addStretch()
        header_layout.addWidget(self.menu_button)
        layout.addWidget(header_widget)
        
        # Stacked widget for status/video
        self.stacked_widget = QStackedWidget()
        
        self.status_label = QLabel('No feed' if stream_url is None else 'Connecting...')
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet(LABEL_STYLE_CONNECTING)
        
        self.video_widget = QVideoWidget()
        self.video_widget.setStyleSheet("background-color: black;")
        
        self.stacked_widget.addWidget(self.status_label)  # Index 0
        self.stacked_widget.addWidget(self.video_widget)  # Index 1
        self.stacked_widget.setCurrentIndex(0)
        
        layout.addWidget(self.stacked_widget)
        
        if stream_url:
            QTimer.singleShot(100, self.connect_stream)

    def connect_stream(self):
        """Connect to the video stream"""
        if not self.stream_url:
            return
            
        logger.info(f"Connecting to stream: {self.stream_url}")
        try:
            self.player = QMediaPlayer(self)
            self.player.setVideoOutput(self.video_widget)
            self.player.error.connect(self._handle_error)
            self.player.mediaStatusChanged.connect(self._handle_media_status_changed)
            
            self.player.setMedia(QMediaContent(QUrl(self.stream_url)))
            self.player.play()
        except Exception as e:
            logger.error(f"Error connecting to stream {self.stream_url}: {e}")
            self.status_label.setText(f'Error:\n{str(e)}')
            self.stacked_widget.setCurrentIndex(0)

    def _handle_error(self):
        err_msg = self.player.errorString()
        logger.error(f"MediaPlayer Error for {self.stream_url}: {err_msg}")
        self.status_label.setText(f"Error: {err_msg}")
        self.stacked_widget.setCurrentIndex(0)

    def _handle_media_status_changed(self, status):
        if status == QMediaPlayer.LoadedMedia or status == QMediaPlayer.BufferedMedia:
            self.stacked_widget.setCurrentIndex(1)
        elif status == QMediaPlayer.InvalidMedia:
            self.status_label.setText(f'Invalid media')
            self.stacked_widget.setCurrentIndex(0)

    def set_label(self, label: str):
        self.title_label.setText(label.upper())

    def close(self):
        if self.player:
            self.player.stop()
            self.player.setMedia(QMediaContent())
            self.player = None

class CentralFeed(Feed):
    """Central feed for a single agent"""

    def __init__(self, stream_url: Optional[str] = None, label_text: Optional[str] = None):
        super().__init__(stream_url, label_text)

class TrackingFeed(Feed):
    """Tracking feed for a single agent"""
    
    def __init__(self, stream_url: Optional[str] = None, label_text: Optional[str] = None):
        super().__init__(stream_url, label_text)

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
        self.central_feed = CentralFeed(central_url, f"{agent_id} - {labels[0]}")
        main_layout.addWidget(self.central_feed, stretch=3)
        
        # 2. Tracking feeds grid (2x2 layout)
        grid_layout = QGridLayout()
        grid_layout.setSpacing(10)
        
        self.tracking_feeds = []
        for i in range(1, 5):
            url = stream_urls[i] if i < len(stream_urls) else None
            feed = TrackingFeed(url, labels[i])
            self.tracking_feeds.append(feed)
            
            # Arrange in 2x2 grid
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