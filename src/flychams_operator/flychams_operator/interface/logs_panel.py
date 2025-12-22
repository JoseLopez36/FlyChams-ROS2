"""Logs panel for displaying tabbed terminal output"""

from PyQt5.QtWidgets import QWidget, QVBoxLayout, QTabWidget, QPlainTextEdit, QLabel
from PyQt5.QtGui import QFont
from PyQt5.QtCore import Qt
from .styles import TERMINAL_STYLE, TAB_WIDGET_STYLE_COMPACT, LABEL_STYLE_TITLE_MEDIUM

class Terminal(QPlainTextEdit):
    """Read-only terminal-like text area"""
    def __init__(self):
        super().__init__()
        self.setReadOnly(True)
        font = QFont("Monospace")
        font.setStyleHint(QFont.StyleHint.Monospace)
        font.setPointSize(10)
        self.setFont(font)
        self.setStyleSheet(TERMINAL_STYLE)
        self.setMaximumBlockCount(1000)  # Keep memory usage sane

    def append_line(self, text: str):
        self.appendPlainText(text)
        self.verticalScrollBar().setValue(self.verticalScrollBar().maximum())

class LogsPanel(QWidget):
    """Panel with tabbed terminals for each component"""
    def __init__(self, process_manager):
        super().__init__()
        self.process_manager = process_manager
        self.terminals = {}
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)
        
        title = QLabel('System Logs')
        title.setStyleSheet(LABEL_STYLE_TITLE_MEDIUM)
        layout.addWidget(title)
        
        self.tabs = QTabWidget()
        self.tabs.setStyleSheet(TAB_WIDGET_STYLE_COMPACT)
        layout.addWidget(self.tabs)
        
        # Initial placeholder tab
        self.add_terminal("General")
        self.terminals["General"].append_line("Waiting for system components...")
        
        # Connect to process manager
        self.process_manager.line_received.connect(self.on_line_received)

    def add_terminal(self, name: str) -> Terminal:
        if name not in self.terminals:
            terminal = Terminal()
            self.terminals[name] = terminal
            self.tabs.addTab(terminal, name)
            # Remove placeholder if it's the only one and we're adding something real
            if name != "General" and "General" in self.terminals and self.tabs.count() == 2:
                # Keep General for now or just switch? Let's just switch.
                self.tabs.setCurrentWidget(terminal)
        return self.terminals[name]

    def on_line_received(self, component: str, message: str):
        terminal = self.add_terminal(component)
        terminal.append_line(message)

