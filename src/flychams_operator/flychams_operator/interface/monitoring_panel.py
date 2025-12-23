"""Monitoring panel: central camera stream + tracking views rendered as crops (windows) of the main image."""

import logging
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, 
    QLabel, QTabWidget, QStackedWidget,
    QToolButton, QMenu, QAction
)
from PyQt5.QtCore import Qt, QTimer, QUrl, QObject, pyqtSignal, pyqtSlot
from PyQt5.QtGui import QImage, QPixmap
from PyQt5.QtMultimedia import QMediaPlayer, QMediaContent, QVideoProbe, QVideoFrame, QAbstractVideoBuffer
from PyQt5.QtMultimediaWidgets import QVideoWidget
from typing import Optional, Dict, List, Tuple
from .styles import (
    LABEL_STYLE_CONNECTING,
    LABEL_STYLE_TITLE_MEDIUM,
    LABEL_STYLE_PLACEHOLDER,
    TAB_WIDGET_STYLE
)

logger = logging.getLogger(__name__)

def _qvideoframe_to_qimage(frame: QVideoFrame) -> Optional[QImage]:
    """
    Convert a QVideoFrame to a detached QImage (safe after unmap()).

    Returns None if the pixel format cannot be represented as a QImage directly.
    """
    if not frame or not frame.isValid():
        return None

    image_format = QVideoFrame.imageFormatFromPixelFormat(frame.pixelFormat())
    if image_format == QImage.Format_Invalid:
        return None

    if not frame.map(QAbstractVideoBuffer.ReadOnly):
        return None

    try:
        w = frame.width()
        h = frame.height()
        bytes_per_line = frame.bytesPerLine()
        # In PyQt5, QVideoFrame.bits() returns a sip.voidptr.
        img = QImage(frame.bits(), w, h, bytes_per_line, image_format)
        return img.copy()  # detach from the underlying buffer before unmap()
    finally:
        frame.unmap()


class CropView(QWidget):
    """A lightweight view that displays a crop (window) of the latest main-frame QImage."""

    makeMainRequested = pyqtSignal(object)  # kept for UI consistency (can be used to promote crop later)

    def __init__(self, label_text: str, *, is_main: bool = False, allow_promote: bool = False):
        super().__init__()
        self.label_text = label_text or ""
        self.is_main = is_main
        self.allow_promote = allow_promote

        self._latest_frame: Optional[QImage] = None
        self._crop: Optional[Tuple[int, int, int, int]] = None  # x, y, w, h
        self._out_of_bounds: bool = False

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        header_widget = QWidget()
        header_widget.setStyleSheet(
            "background-color: #2d2d2d; border-top-left-radius: 4px; border-top-right-radius: 4px;"
        )
        header_layout = QHBoxLayout(header_widget)
        header_layout.setContentsMargins(8, 4, 8, 4)

        self.title_label = QLabel(self.label_text.upper())
        self.title_label.setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold; border: none;")
        header_layout.addWidget(self.title_label)

        # Menu (optional future use; kept to avoid breaking layout expectations)
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
        make_main_action.triggered.connect(lambda: self.makeMainRequested.emit(self))
        make_main_action.setEnabled(self.allow_promote)
        self.menu.addAction(make_main_action)

        self.menu_button = QToolButton()
        self.menu_button.setText("⋮")
        self.menu_button.setMenu(self.menu)
        self.menu_button.setPopupMode(QToolButton.InstantPopup)
        self.menu_button.setStyleSheet("""
            QToolButton {
                color: #ffffff;
                font-size: 22px;
                border: none;
                background: transparent;
                font-weight: bold;
                padding: 0px;
                margin: 0px;
                min-width: 30px;
                max-width: 30px;
                min-height: 30px;
                max-height: 30px;
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

        self.stacked_widget = QStackedWidget()

        self.status_label = QLabel('Waiting for video...' if self.is_main else 'Waiting for main feed...')
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet(LABEL_STYLE_CONNECTING)

        self.image_label = QLabel()
        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setStyleSheet("background-color: black;")
        self.image_label.setMinimumSize(10, 10)

        self.stacked_widget.addWidget(self.status_label)  # Index 0
        self.stacked_widget.addWidget(self.image_label)   # Index 1
        self.stacked_widget.setCurrentIndex(0)

        layout.addWidget(self.stacked_widget)

    def set_crop(self, x: int, y: int, w: int, h: int, *, is_out_of_bounds: bool = False):
        self._crop = (int(x), int(y), int(w), int(h))
        self._out_of_bounds = bool(is_out_of_bounds)
        if self._out_of_bounds:
            self.status_label.setText('Crop outside image bounds')
            self.stacked_widget.setCurrentIndex(0)
            return
        self._render()

    def clear_crop(self):
        self._crop = None
        self._out_of_bounds = False
        self._render()

    def set_frame(self, frame_img: Optional[QImage]):
        self._latest_frame = frame_img
        self._render()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._render()

    def _render(self):
        if self._latest_frame is None or self._latest_frame.isNull():
            self.status_label.setText('Waiting for video...' if self.is_main else 'Waiting for main feed...')
            self.stacked_widget.setCurrentIndex(0)
            return

        src = self._latest_frame

        # Main view shows the full frame.
        if self.is_main or self._crop is None:
            img = src
        else:
            x, y, w, h = self._crop
            img = src.copy(x, y, w, h)

        pixmap = QPixmap.fromImage(img)
        target_size = self.image_label.size()
        if target_size.width() > 0 and target_size.height() > 0:
            pixmap = pixmap.scaled(target_size, Qt.KeepAspectRatio, Qt.SmoothTransformation)

        self.image_label.setPixmap(pixmap)
        self.stacked_widget.setCurrentIndex(1)

        # Visual hint when the requested crop is out of bounds.
        if (not self.is_main) and self._out_of_bounds:
            self.image_label.setStyleSheet("background-color: black; border: 2px solid #d64a4a;")
        else:
            self.image_label.setStyleSheet("background-color: black; border: none;")


class MainVideoStream(QObject):
    """Owns a QMediaPlayer and probes frames to feed CropViews."""

    frameUpdated = pyqtSignal(object)  # QImage

    def __init__(self, stream_url: Optional[str], video_output: QVideoWidget):
        super().__init__()
        self.stream_url = stream_url
        self.video_output = video_output

        self.media_player: Optional[QMediaPlayer] = None
        self.video_probe: Optional[QVideoProbe] = None

        if self.stream_url:
            QTimer.singleShot(100, self.connect_stream)

    def connect_stream(self):
        if not self.stream_url:
            return
        logger.info(f"Connecting main stream: {self.stream_url}")
        try:
            self.media_player = QMediaPlayer(self.video_output)
            self.media_player.setVideoOutput(self.video_output)

            self.video_probe = QVideoProbe(self.video_output)
            self.video_probe.videoFrameProbed.connect(self._on_frame_probed)
            if not self.video_probe.setSource(self.media_player):
                logger.warning("Could not attach QVideoProbe to QMediaPlayer; crop views will not update.")

            stream_url = QUrl(self.stream_url)
            self.media_player.setMedia(QMediaContent(stream_url))
            self.media_player.play()
        except Exception as e:
            logger.error(f"Error connecting main stream {self.stream_url}: {e}")

    @pyqtSlot(QVideoFrame)
    def _on_frame_probed(self, frame: QVideoFrame):
        img = _qvideoframe_to_qimage(frame)
        if img is not None and not img.isNull():
            self.frameUpdated.emit(img)

    def close(self):
        try:
            if self.media_player:
                self.media_player.stop()
                self.media_player.setMedia(QMediaContent())
        finally:
            self.media_player = None
            self.video_probe = None


class CameraFeed(QWidget):
    """(Deprecated) Widget for a single camera feed using QtMultimedia (kept for backwards compatibility)."""
    
    makeMainRequested = pyqtSignal(object)  # Signal to request becoming the main feed
    
    def __init__(self, stream_url: Optional[str] = None, label_text: Optional[str] = None):
        super().__init__()
        
        self.stream_url = stream_url
        self.label_text = label_text or ""
        self.media_player: Optional[QMediaPlayer] = None

        if stream_url:
            logger.info(f"Creating camera feed for stream: {stream_url}")
        else:
            logger.debug("Creating placeholder camera feed (no stream URL)")
        
        # Create main layout for the feed (header + video/status)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        # Add header bar
        header_widget = QWidget()
        header_widget.setStyleSheet(f"background-color: #2d2d2d; border-top-left-radius: 4px; border-top-right-radius: 4px;")
        header_layout = QHBoxLayout(header_widget)
        header_layout.setContentsMargins(8, 4, 8, 4)
        
        self.title_label = QLabel(self.label_text.upper())
        self.title_label.setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold; border: none;")
        header_layout.addWidget(self.title_label)
        
        # Create menu
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
        make_main_action.triggered.connect(lambda: self.makeMainRequested.emit(self))
        self.menu.addAction(make_main_action)

        # Add "three dots" menu button
        self.menu_button = QToolButton()
        self.menu_button.setText("⋮")
        self.menu_button.setMenu(self.menu)
        self.menu_button.setPopupMode(QToolButton.InstantPopup)
        self.menu_button.setStyleSheet("""
            QToolButton {
                color: #ffffff; 
                font-size: 22px; 
                border: none; 
                background: transparent;
                font-weight: bold;
                padding: 0px;
                margin: 0px;
                min-width: 30px;
                max-width: 30px;
                min-height: 30px;
                max-height: 30px;
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
        
        # Create stacked widget to switch between video and status label
        self.stacked_widget = QStackedWidget()
        
        # Status label for connection state
        self.status_label = QLabel('No feed' if stream_url is None else 'Connecting...')
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet(LABEL_STYLE_CONNECTING)
        
        # Video widget for QtMultimedia
        self.video_widget = QVideoWidget()
        self.video_widget.setStyleSheet("background-color: black;")
        
        # Add widgets to stack
        self.stacked_widget.addWidget(self.status_label)  # Index 0
        self.stacked_widget.addWidget(self.video_widget)  # Index 1
        self.stacked_widget.setCurrentIndex(0)  # Show status label initially
        
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
            self.media_player.error.connect(self.handle_error)
            self.media_player.mediaStatusChanged.connect(self.handle_media_status_changed)
            
            # Set up RTP stream URL
            stream_url = QUrl(self.stream_url)
            
            # Set media source
            self.media_player.setMedia(QMediaContent(stream_url))
            
            # Start playback
            self.media_player.play()
            
            # Switch to video widget after a short delay
            QTimer.singleShot(1000, lambda: self.stacked_widget.setCurrentIndex(1))
                
        except Exception as e:
            logger.error(f"Error connecting to stream {self.stream_url}: {e}")
            self.status_label.setText(f'Error connecting to stream {self.stream_url}:\n{str(e)}')
            self.stacked_widget.setCurrentIndex(0)
    
    # ================================ Callback handling ================================
    def handle_error(self, error):
        """Handle media player errors"""
        error_string = self.media_player.errorString()
        error_msg = error_string if error_string else f"Media player error: {error}"
        logger.error(f"Media player error for stream {self.stream_url}: {error_msg}")
        self.status_label.setText(f'Error connecting to stream {self.stream_url}:\n{error_msg}')
        self.stacked_widget.setCurrentIndex(0)
    
    def handle_media_status_changed(self, status):
        """Handle media status changes"""
        if status == QMediaPlayer.LoadedMedia:
            # Media loaded successfully, switch to video widget
            logger.info(f"Media loaded successfully for stream: {self.stream_url}")
            self.stacked_widget.setCurrentIndex(1)
        elif status == QMediaPlayer.InvalidMedia:
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
            self.media_player.setMedia(QMediaContent())
            self.media_player = None
        
        event.accept()

class AgentCameraComposition(QWidget):
    """
    Widget displaying camera feeds for a single agent:
    - 1 large main feed (central camera stream)
    - 2x2 grid of tracking feeds rendered as crops of the main image
    """
    
    def __init__(self, agent_id: str, stream_urls: list):
        super().__init__()
        
        self.agent_id = agent_id
        self.camera_feeds: List[QWidget] = []
        self._crop_views: List[CropView] = []
        self._main_stream: Optional[MainVideoStream] = None
        
        logger.info(f"Creating camera composition for agent '{agent_id}' with {len(stream_urls)} stream(s)")
        
        # Define labels based on the reference image distribution
        labels = [
            "CENTRAL CAMERA",
            "TRACKING 1",
            "TRACKING 2",
            "TRACKING 3",
            "TRACKING 4"
        ]
        
        # Main stream URL: use the first configured stream URL (historically CENTRAL CAMERA)
        main_url = stream_urls[0] if len(stream_urls) > 0 else None

        # Create main view (full frame) + 4 crop views
        self.main_view = CropView(labels[0], is_main=True, allow_promote=False)

        self.crop_views = [CropView(labels[i], allow_promote=False) for i in range(1, 5)]

        self.camera_feeds = [self.main_view] + self.crop_views
        self._crop_views = self.crop_views

        # Under the hood, still render the video with QVideoWidget, and probe frames for cropping.
        # We embed the QVideoWidget inside the main_view by overlaying it in the image_label area.
        # Instead of restructuring layouts heavily, we use the probed frames to drive main_view rendering.
        self._video_widget = QVideoWidget()
        self._video_widget.setStyleSheet("background-color: black;")
        self._video_widget.setSizePolicy(self.main_view.sizePolicy())

        # Replace the main_view's image_label content with the QVideoWidget visual output.
        # The main_view will still render from frames; the QVideoWidget gives "native" playback.
        self.main_view.image_label.hide()
        self.main_view.stacked_widget.insertWidget(1, self._video_widget)
        self.main_view.stacked_widget.setCurrentIndex(0)

        self._main_stream = MainVideoStream(main_url, self._video_widget)
        self._main_stream.frameUpdated.connect(self._on_main_frame)
        if main_url:
            # Show the video widget immediately (even if probing fails); it will render once frames arrive.
            self.main_view.stacked_widget.setCurrentIndex(1)
        
        # Create main horizontal layout
        self.main_layout = QHBoxLayout(self)
        self.main_layout.setContentsMargins(10, 10, 10, 10)
        self.main_layout.setSpacing(10)
        
        # Grid for cameras 1-4 (2x2 layout)
        self.grid_layout = QGridLayout()
        self.grid_layout.setSpacing(10)
        
        self.setup_layout()

    @pyqtSlot(object)
    def _on_main_frame(self, img: QImage):
        # Main view: show the full frame if the QVideoWidget isn't visible yet.
        # Crop views: render their crops from the same frame.
        if self.main_view.stacked_widget.currentIndex() == 0:
            # Show video widget once we start receiving frames
            self.main_view.stacked_widget.setCurrentIndex(1)
        for v in self._crop_views:
            v.set_frame(img)

    def update_gui_setpoints(self, camera_ids: List[str], crops: List[object]):
        """
        Update the tracking crop windows.

        `camera_ids` and `crops` are parallel arrays from flychams_interfaces/GuiSetpoints.
        """
        # Default: clear all crops, then apply up to 4 setpoints.
        for v in self._crop_views:
            v.clear_crop()

        n = min(len(camera_ids), len(crops), 4)
        for i in range(n):
            crop = crops[i]
            cam_id = camera_ids[i] if i < len(camera_ids) else ""

            # Best effort: show camera_id in the header so the operator knows what window it is.
            if cam_id:
                self._crop_views[i].title_label.setText(cam_id.upper())

            try:
                self._crop_views[i].set_crop(
                    crop.x, crop.y, crop.w, crop.h,
                    is_out_of_bounds=getattr(crop, "is_out_of_bounds", False),
                )
            except Exception as e:
                logger.warning(f"Failed to apply crop for agent '{self.agent_id}' index {i}: {e}")

    def setup_layout(self):
        """Initial layout setup or refresh after swapping"""
        # Clear existing layout items
        # Note: We don't delete the widgets, just remove them from layouts
        for i in reversed(range(self.main_layout.count())):
            item = self.main_layout.itemAt(i)
            if item.widget():
                # We don't want to delete the widget, just remove it from the layout
                # setParent(None) removes it from layout without deleting
                item.widget().setParent(None)
            elif item.layout():
                self.clear_layout(item.layout())
                self.main_layout.removeItem(item)

        # Left side: Camera 0 (Main) - occupies more space
        self.main_layout.addWidget(self.camera_feeds[0], stretch=3)
        self.camera_feeds[0].setParent(self) # Re-add to this widget
        
        # Right side: Grid for cameras 1-4 (2x2 layout)
        self.grid_layout = QGridLayout()
        self.grid_layout.setSpacing(10)
        
        # Arrange in 2x2 grid
        self.grid_layout.addWidget(self.camera_feeds[1], 0, 0)
        self.grid_layout.addWidget(self.camera_feeds[2], 0, 1)
        self.grid_layout.addWidget(self.camera_feeds[3], 1, 0)
        self.grid_layout.addWidget(self.camera_feeds[4], 1, 1)
        
        for i in range(1, 5):
            self.camera_feeds[i].setParent(self) # Re-add to this widget

        # Ensure all feeds in the grid have the same size
        self.grid_layout.setRowStretch(0, 1)
        self.grid_layout.setRowStretch(1, 1)
        self.grid_layout.setColumnStretch(0, 1)
        self.grid_layout.setColumnStretch(1, 1)
        
        # Add the grid layout to main layout with a smaller stretch
        self.main_layout.addLayout(self.grid_layout, stretch=2)

    def clear_layout(self, layout):
        """Helper to clear a layout without deleting widgets"""
        if layout is not None:
            while layout.count():
                item = layout.takeAt(0)
                if item.widget():
                    item.widget().setParent(None)
                elif item.layout():
                    self.clear_layout(item.layout())

    def promote_to_main(self, feed_widget):
        """
        Swap the selected feed with the current main feed (index 0).

        Note: with crop-based tracking views, promoting a crop to main would require swapping
        rendering roles; for now we keep the behavior (swap widgets in layout) for UI parity,
        but only the original central stream has the live QMediaPlayer.
        """
        if feed_widget not in self.camera_feeds:
            return
            
        index = self.camera_feeds.index(feed_widget)
        if index == 0:
            logger.debug("Feed is already the main one")
            return
            
        logger.info(f"Promoting feed at index {index} ('{feed_widget.label_text}') to main for agent '{self.agent_id}'")
        
        # Swap positions in the list
        self.camera_feeds[0], self.camera_feeds[index] = self.camera_feeds[index], self.camera_feeds[0]
        
        # Re-apply layout
        self.setup_layout()
    
    def closeEvent(self, event):
        """Clean up when widget is closed"""
        logger.debug(f"Closing camera composition for agent '{self.agent_id}'")
        # Close all camera feeds
        for feed in self.camera_feeds:
            feed.close()
        if self._main_stream:
            self._main_stream.close()
            self._main_stream = None
        event.accept()


class MonitoringPanel(QWidget):
    """Widget for displaying multiple camera feeds"""
    
    def __init__(self):
        super().__init__()

        self._agent_widgets: Dict[str, AgentCameraComposition] = {}
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)
        
        # Title with improved styling
        title = QLabel('Monitoring Feeds')
        title.setStyleSheet(LABEL_STYLE_TITLE_MEDIUM)
        layout.addWidget(title)
        
        # Tab widget for multiple feeds with improved styling
        self.tab_widget = QTabWidget()
        self.tab_widget.setStyleSheet(TAB_WIDGET_STYLE)
        layout.addWidget(self.tab_widget)
        
        # Placeholder message with improved styling
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
        self._agent_widgets[agent_id] = composition_widget
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
        self._agent_widgets.pop(agent_id, None)
        
        # Add placeholder if no feeds remain
        if self.tab_widget.count() == 0:
            logger.debug("No agents remaining, adding placeholder")
            placeholder = QLabel('No camera feeds configured\n\n'
                              'To add a feed, configure UDP/RTP stream')
            placeholder.setAlignment(Qt.AlignCenter)
            placeholder.setStyleSheet(LABEL_STYLE_PLACEHOLDER)
            self.tab_widget.addTab(placeholder, 'No Feeds')

    def update_agent_gui_setpoints(self, agent_id: str, msg):
        """Update crop windows for an agent based on GuiSetpoints."""
        widget = self._agent_widgets.get(agent_id)
        if not widget:
            return
        try:
            camera_ids = list(getattr(msg, "camera_ids", []))
            crops = list(getattr(msg, "crops", []))
            widget.update_gui_setpoints(camera_ids, crops)
        except Exception as e:
            logger.warning(f"Failed to update GUI setpoints for agent '{agent_id}': {e}")
