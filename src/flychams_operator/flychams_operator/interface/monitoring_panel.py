"""Monitoring panel: central camera stream + tracking views. Supports both multi-camera and multi-window tracking approaches"""

import logging
from typing import Dict, Optional, List

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QTabWidget, QLabel,
    QMenu, QAction, QToolButton, QStackedWidget, QFrame, QStyle, QSizePolicy
)
from PyQt5.QtCore import Qt, QUrl, QTimer
from PyQt5.QtMultimedia import QMediaPlayer, QMediaContent
from PyQt5.QtMultimediaWidgets import QVideoWidget
from .styles import (
    LABEL_STYLE_TITLE_MEDIUM,
    LABEL_STYLE_PLACEHOLDER,
    TAB_WIDGET_STYLE,
    LABEL_STYLE_CONNECTING
)

# Configure logger
logger = logging.getLogger(__name__)

# --- Styling Constants ---
STYLE_PANEL_BG = "background-color: #1e1e1e;"
STYLE_FEED_CONTAINER = """
    QFrame {
        background-color: #2d2d2d;
        border: 1px solid #3d3d3d;
        border-radius: 4px;
    }
"""
STYLE_FEED_HEADER = """
    background-color: #333333;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
    border-bottom: 1px solid #3d3d3d;
"""
STYLE_TITLE = "color: #e0e0e0; font-size: 12px; font-weight: bold; letter-spacing: 0.5px;"
STYLE_STATUS_ERROR = "color: #ff6b6b; font-size: 12px;"
STYLE_STATUS_CONNECTING = "color: #f1c40f; font-size: 12px;"
STYLE_STATUS_NO_FEED = "color: #7f8c8d; font-size: 12px;"
STYLE_VIDEO_BG = "background-color: #000000; border-bottom-left-radius: 4px; border-bottom-right-radius: 4px;"
STYLE_MENU_BUTTON = """
    QToolButton {
        color: #bbbbbb; 
        border: none; 
        background: transparent;
        font-weight: bold;
    }
    QToolButton:hover {
        color: #ffffff;
        background-color: rgba(255, 255, 255, 0.1);
        border-radius: 2px;
    }
"""

class FeedWidget(QFrame):
    """A robust video feed widget with status handling, retry logic, and clean UI. Manages its own QMediaPlayer instance"""
    
    RETRY_INTERVAL_MS = 5000  # Retry connection every 5 seconds on failure

    def __init__(self, stream_url = None, title: str = ""):
        super().__init__()
        self.stream_url = stream_url
        self.title = title
        self.player = None
        self.retry_timer = QTimer(self)
        self.retry_timer.timeout.connect(self.connect_stream)
        
        self._setup_ui()
        
        if self.stream_url:
            # Delay initial connection slightly to allow UI to settle
            QTimer.singleShot(100, self.connect_stream)

    def _setup_ui(self):
        """Initialize the user interface"""
        self.setStyleSheet(STYLE_FEED_CONTAINER)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # --- Header ---
        header_widget = QWidget()
        header_widget.setStyleSheet(STYLE_FEED_HEADER)
        header_widget.setFixedHeight(28)
        header_layout = QHBoxLayout(header_widget)
        header_layout.setContentsMargins(8, 0, 8, 0)
        header_layout.setSpacing(8)

        # Status Indicator
        self.status_indicator = QLabel()
        self.status_indicator.setFixedSize(16, 16)
        self.status_indicator.setAlignment(Qt.AlignCenter)
        self.status_indicator.setStyleSheet("border: none; background: transparent;")
        header_layout.addWidget(self.status_indicator)

        # Title
        self.title_label = QLabel(self.title.upper())
        self.title_label.setStyleSheet(STYLE_TITLE + "border: none; background: transparent;")
        header_layout.addWidget(self.title_label)
        
        header_layout.addStretch()

        # Menu Button
        self.menu_btn = QToolButton()
        self.menu_btn.setText("⋮")
        self.menu_btn.setPopupMode(QToolButton.InstantPopup)
        self.menu_btn.setStyleSheet(STYLE_MENU_BUTTON)
        self.menu_btn.setFixedSize(20, 20)
        
        # Menu
        self.menu = QMenu(self)
        self.menu.setStyleSheet("""
            QMenu { background-color: #2d2d2d; color: white; border: 1px solid #444; }
            QMenu::item { padding: 4px 20px; }
            QMenu::item:selected { background-color: #3d3d3d; }
        """)
        self.action_restart = QAction("Restart Stream", self)
        self.action_restart.triggered.connect(self.restart_stream)
        self.menu.addAction(self.action_restart)
        
        self.menu_btn.setMenu(self.menu)
        header_layout.addWidget(self.menu_btn)

        layout.addWidget(header_widget)

        # --- Content (Stacked: Video vs Status Message) ---
        self.stacked_widget = QStackedWidget()
        
        # Status/Placeholder Page
        self.status_page = QWidget()
        self.status_page.setStyleSheet("background-color: #1a1a1a; border-radius: 0px;")
        status_layout = QVBoxLayout(self.status_page)
        status_layout.setAlignment(Qt.AlignCenter)
        self.status_message = QLabel("No Feed Configured")
        self.status_message.setAlignment(Qt.AlignCenter)
        self.status_message.setStyleSheet(LABEL_STYLE_CONNECTING)
        status_layout.addWidget(self.status_message)
        
        # Video Page
        self.video_container = QWidget()
        self.video_container.setStyleSheet("background-color: black;")
        video_layout = QVBoxLayout(self.video_container)
        video_layout.setContentsMargins(0,0,0,0)
        
        self.video_widget = QVideoWidget()
        self.video_widget.setStyleSheet(STYLE_VIDEO_BG)
        video_layout.addWidget(self.video_widget)

        self.stacked_widget.addWidget(self.status_page)     # Index 0
        self.stacked_widget.addWidget(self.video_container) # Index 1
        
        layout.addWidget(self.stacked_widget)
        
        self._update_status_ui("idle")

    def connect_stream(self):
        """Initialize and start the media player."""
        self.retry_timer.stop()
        
        if not self.stream_url:
            self._update_status_ui("no_url")
            return

        self._cleanup_player()
        
        self._update_status_ui("connecting")
        logger.info(f"Connecting to stream: {self.stream_url}")
        
        try:
            self.player = QMediaPlayer(self)
            self.player.setVideoOutput(self.video_widget)
            
            # Connect signals
            self.player.error.connect(self._handle_error)
            self.player.mediaStatusChanged.connect(self._handle_media_status)
            
            content = QMediaContent(QUrl(self.stream_url))
            self.player.setMedia(content)
            self.player.play()
            
        except Exception as e:
            logger.error(f"Exception connecting to {self.stream_url}: {e}")
            self._handle_error()

    def restart_stream(self):
        """Manually restart the stream."""
        logger.info(f"Manual restart requested for {self.title}")
        self.connect_stream()

    def _cleanup_player(self):
        """Safely stop and delete the player."""
        if self.player:
            try:
                self.player.stop()
                self.player.setMedia(QMediaContent())
                self.player.deleteLater()
            except Exception as e:
                logger.warning(f"Error cleaning up player: {e}")
            finally:
                self.player = None

    def _handle_error(self, error_code=None):
        """Handle media player errors."""
        err_msg = self.player.errorString() if self.player else "Unknown error"
        logger.error(f"Stream error ({self.title}): {err_msg}")
        self._update_status_ui("error", err_msg)
        
        # Schedule retry
        if not self.retry_timer.isActive():
            self.retry_timer.start(self.RETRY_INTERVAL_MS)

    def _handle_media_status(self, status):
        """Handle media status changes."""
        if status == QMediaPlayer.BufferedMedia or status == QMediaPlayer.LoadedMedia:
            self._update_status_ui("active")
        elif status == QMediaPlayer.BufferingMedia or status == QMediaPlayer.StalledMedia:
            self._update_status_ui("buffering")
        elif status == QMediaPlayer.InvalidMedia:
            self._handle_error()
        elif status == QMediaPlayer.EndOfMedia:
            # For streams, end of media usually means connection drop
            self._handle_error()

    def _update_status_ui(self, state: str, message: str = ""):
        """Update the UI based on the current state."""
        
        # Helper to update icon using a colored dot (semaphore style)
        def set_status_dot(color):
            self.status_indicator.setText("●")
            self.status_indicator.setStyleSheet(f"color: {color}; border: none; background: transparent; font-size: 12px;")

        if state == "active":
            self.stacked_widget.setCurrentIndex(1)
            set_status_dot("#2ecc71") # Green
            self.status_indicator.setToolTip("Stream Active")
        elif state == "buffering":
            self.stacked_widget.setCurrentIndex(1)
            set_status_dot("#3498db") # Blue
            self.status_indicator.setToolTip("Buffering...")
        elif state == "connecting":
            self.stacked_widget.setCurrentIndex(0)
            self.status_message.setText(f"Connecting to:\n{self.stream_url}")
            self.status_message.setStyleSheet(STYLE_STATUS_CONNECTING)
            set_status_dot("#f1c40f") # Yellow
            self.status_indicator.setToolTip("Connecting...")
        elif state == "error":
            self.stacked_widget.setCurrentIndex(0)
            self.status_message.setText(f"Connection Failed\n{message}\nRetrying...")
            self.status_message.setStyleSheet(STYLE_STATUS_ERROR)
            set_status_dot("#e74c3c") # Red
            self.status_indicator.setToolTip("Error")
        elif state == "no_url":
            self.stacked_widget.setCurrentIndex(0)
            self.status_message.setText("No Stream Configured")
            self.status_message.setStyleSheet(STYLE_STATUS_NO_FEED)
            set_status_dot("#7f8c8d") # Grey
            self.status_indicator.setToolTip("No Feed")
        else: # idle
            set_status_dot("#7f8c8d")

    def set_label(self, text: str):
        self.title_label.setText(text.upper())

    def closeEvent(self, event):
        """Ensure resources are released on close."""
        self.retry_timer.stop()
        self._cleanup_player()
        super().closeEvent(event)

class AgentCameraComposition(QWidget):
    """
    Layout composition for a single agent's camera feeds.
    Structure:
    - Left: Large Central Feed
    - Right: 2x2 Grid of Tracking Feeds
    """
    
    def __init__(self, agent_id: str, stream_urls: List[str]):
        super().__init__()
        self.agent_id = agent_id
        self.feeds: List[FeedWidget] = []
        
        self._setup_ui(stream_urls)

    def _setup_ui(self, stream_urls: List[str]):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)

        # Feed Labels
        feed_labels = [
            "CENTRAL VIEW",
            "TRACKING 1", "TRACKING 2",
            "TRACKING 3", "TRACKING 4"
        ]

        # 1. Central Feed (Large)
        central_url = stream_urls[0] if len(stream_urls) > 0 else None
        self.central_feed = FeedWidget(central_url, f"{self.agent_id} | {feed_labels[0]}")
        self.feeds.append(self.central_feed)
        layout.addWidget(self.central_feed, stretch=3)

        # 2. Tracking Feeds (2x2 Grid)
        grid_container = QWidget()
        grid_layout = QGridLayout(grid_container)
        grid_layout.setContentsMargins(0, 0, 0, 0)
        grid_layout.setSpacing(8)
        
        # Ensure equal sizes for all rows and columns in the grid
        grid_layout.setColumnStretch(0, 1)
        grid_layout.setColumnStretch(1, 1)
        grid_layout.setRowStretch(0, 1)
        grid_layout.setRowStretch(1, 1)

        for i in range(1, 5):
            url = stream_urls[i] if i < len(stream_urls) else None
            feed = FeedWidget(url, feed_labels[i])
            self.feeds.append(feed)
            
            row = (i - 1) // 2
            col = (i - 1) % 2
            grid_layout.addWidget(feed, row, col)

        layout.addWidget(grid_container, stretch=2)

    def closeEvent(self, event):
        """Close event to all feeds"""
        for feed in self.feeds:
            feed.close()
        super().closeEvent(event)

class MonitoringPanel(QWidget):
    """Main monitoring panel hosting multiple agent compositions in tabs"""
    
    def __init__(self):
        super().__init__()
        self.agent_widgets: Dict[str, AgentCameraComposition] = {}
        self._setup_ui()

    def _setup_ui(self):
        self.setStyleSheet(STYLE_PANEL_BG)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(10)
        
        # Title
        title = QLabel('Monitoring Feeds')
        title.setStyleSheet(LABEL_STYLE_TITLE_MEDIUM)
        layout.addWidget(title)

        # Tab
        self.tab_widget = QTabWidget()
        self.tab_widget.setStyleSheet(TAB_WIDGET_STYLE)
        layout.addWidget(self.tab_widget)
        
        self._add_placeholder()

    def _add_placeholder(self):
        """Add a placeholder tab when no agents are connected"""
        placeholder = QLabel("Waiting for agent connections...\n\nConfigure video streams to begin")
        placeholder.setAlignment(Qt.AlignCenter)
        placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
        self.tab_widget.addTab(placeholder, "No Active Feeds")

    def add_agent(self, agent_id: str, stream_urls: List[str]):
        """Add or update camera feeds for an agent"""
        logger.info(f"Adding monitoring for agent: {agent_id}")
        
        # Remove placeholder if it's the only tab
        if self.tab_widget.count() == 1:
            widget = self.tab_widget.widget(0)
            if isinstance(widget, QLabel):
                self.tab_widget.removeTab(0)
                widget.deleteLater()

        # Remove existing agent widget if present
        if agent_id in self.agent_widgets:
            self.remove_agent(agent_id)

        # Create new composition
        composition = AgentCameraComposition(agent_id, stream_urls)
        self.agent_widgets[agent_id] = composition
        self.tab_widget.addTab(composition, agent_id)
        self.tab_widget.setCurrentWidget(composition)

    def remove_agent(self, agent_id: str):
        """Remove an agent's feeds."""
        if agent_id not in self.agent_widgets:
            return

        logger.info(f"Removing monitoring for agent: {agent_id}")
        widget = self.agent_widgets.pop(agent_id)
        
        # Find and remove the tab
        index = self.tab_widget.indexOf(widget)
        if index != -1:
            self.tab_widget.removeTab(index)
        
        widget.close()
        widget.deleteLater()

        if self.tab_widget.count() == 0:
            self._add_placeholder()