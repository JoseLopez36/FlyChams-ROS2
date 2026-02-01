"""Centralized GUI style definitions for FlyChams Dashboard"""

# Color Constants
COLOR_BACKGROUND_PRIMARY = "#1e1e1e"
COLOR_BACKGROUND_SECONDARY = "#2d2d2d"
COLOR_BACKGROUND_TERTIARY = "#3d3d3d"
COLOR_BACKGROUND_DARK = "#1a1a1a"
COLOR_BACKGROUND_DANGER = "#8b2d2d"
COLOR_BACKGROUND_DANGER_HOVER = "#9b3d3d"
COLOR_BACKGROUND_DANGER_PRESSED = "#7b1d1d"

COLOR_TEXT_PRIMARY = "#ffffff"
COLOR_TEXT_SECONDARY = "#e0e0e0"
COLOR_TEXT_TERTIARY = "#cccccc"
COLOR_TEXT_PLACEHOLDER = "#888888"
COLOR_TEXT_DISABLED = "#808080"

COLOR_ACCENT_PRIMARY = "#007a7a"  # Lighter Prussian Green
COLOR_ACCENT_PRIMARY_HOVER = "#008a8a"
COLOR_ACCENT_PRIMARY_PRESSED = "#006a6a"
COLOR_ACCENT_DANGER = "#ff4444"
COLOR_BORDER_PRIMARY = "#2d2d2d"
COLOR_BORDER_SECONDARY = "#3d3d3d"
COLOR_BORDER_ACCENT = "#007a7a"

# Button Styles
BUTTON_STYLE_STANDARD = """
    QPushButton {{
        background-color: {accent_primary};
        color: {text_primary};
        border: none;
        border-radius: 6px;
        padding: 12px 20px;
        font-size: 22px;
        font-weight: bold;
        text-align: left;
        min-height: 44px;
    }}
    QPushButton:hover {{
        background-color: {accent_hover};
    }}
    QPushButton:pressed {{
        background-color: {accent_pressed};
    }}
""".format(
    bg_secondary=COLOR_BACKGROUND_SECONDARY,
    text_secondary=COLOR_TEXT_SECONDARY,
    border_secondary=COLOR_BORDER_SECONDARY,
    bg_tertiary=COLOR_BACKGROUND_TERTIARY,
    accent_primary=COLOR_ACCENT_PRIMARY,
    accent_hover=COLOR_ACCENT_PRIMARY_HOVER,
    accent_pressed=COLOR_ACCENT_PRIMARY_PRESSED,
    text_primary=COLOR_TEXT_PRIMARY,
    bg_primary=COLOR_BACKGROUND_PRIMARY
)

BUTTON_STYLE_SECONDARY = """
    QPushButton {{
        background-color: {bg_primary};
        color: {text_secondary};
        border: 1px solid {border_primary};
        border-radius: 6px;
        padding: 12px 20px;
        font-size: 22px;
        font-weight: bold;
        text-align: left;
        min-height: 44px;
    }}
    QPushButton:hover {{
        background-color: {bg_secondary};
    }}
""".format(
    bg_primary=COLOR_BACKGROUND_PRIMARY,
    text_secondary=COLOR_TEXT_SECONDARY,
    border_primary=COLOR_BORDER_PRIMARY,
    bg_secondary=COLOR_BACKGROUND_SECONDARY
)

BUTTON_STYLE_DANGER = """
    QPushButton {{
        background-color: {bg_danger};
        color: {text_primary};
        border: 1px solid {border_danger};
        border-radius: 6px;
        padding: 12px 20px;
        font-size: 32px;
        font-weight: bold;
        text-align: center;
        min-height: 56px;
    }}
    QPushButton:hover {{
        background-color: {bg_danger_hover};
        border: 1px solid {accent_danger};
    }}
    QPushButton:pressed {{
        background-color: {bg_danger_pressed};
        border: 1px solid {accent_danger};
    }}
""".format(
    bg_danger=COLOR_BACKGROUND_DANGER,
    text_primary=COLOR_TEXT_PRIMARY,
    border_danger="#9b3d3d",
    bg_danger_hover=COLOR_BACKGROUND_DANGER_HOVER,
    accent_danger=COLOR_ACCENT_DANGER,
    bg_danger_pressed=COLOR_BACKGROUND_DANGER_PRESSED
)

# Label Styles
LABEL_STYLE_TITLE = """
    QLabel {{
        font-size: 24px;
        font-weight: bold;
        color: {text_primary};
        padding: 12px 0px;
        border-bottom: 2px solid {accent_primary};
        margin-bottom: 8px;
    }}
""".format(
    text_primary=COLOR_TEXT_PRIMARY,
    accent_primary=COLOR_ACCENT_PRIMARY
)

LABEL_STYLE_TITLE_MEDIUM = """
    QLabel {{
        font-size: 22px;
        font-weight: bold;
        color: {text_primary};
        padding: 10px 0px;
        border-bottom: 2px solid {accent_primary};
        margin-bottom: 8px;
    }}
""".format(
    text_primary=COLOR_TEXT_PRIMARY,
    accent_primary=COLOR_ACCENT_PRIMARY
)

LABEL_STYLE_SEPARATOR = """
    QLabel {{
        font-size: 24px;
        font-weight: bold;
        color: {text_primary};
        padding: 4px 0px 4px 0px;
        margin-top: 8px;
        border-bottom: 1px solid {border_secondary};
    }}
""".format(
    text_primary=COLOR_TEXT_PRIMARY,
    border_secondary=COLOR_BORDER_SECONDARY
)

LABEL_STYLE_PLACEHOLDER = """
    QLabel {{
        color: {text_placeholder};
        font-size: 16px;
        padding: 40px;
    }}
""".format(
    text_placeholder=COLOR_TEXT_PLACEHOLDER
)

LABEL_STYLE_CONNECTING = """
    QLabel {{
        background-color: {bg_dark};
        color: {text_tertiary};
        border: 1px solid {border_primary};
        border-radius: 4px;
        padding: 20px;
        font-size: 16px;
    }}
""".format(
    bg_dark=COLOR_BACKGROUND_DARK,
    text_tertiary=COLOR_TEXT_TERTIARY,
    border_primary=COLOR_BORDER_PRIMARY
)

# Tab Widget Styles
TAB_WIDGET_STYLE = """
    QTabWidget::pane {{
        border: 1px solid {border_primary};
        background-color: {bg_primary};
        border-radius: 4px;
    }}
    QTabBar::tab {{
        background-color: {bg_secondary};
        color: {text_tertiary};
        padding: 10px 20px;
        margin-right: 2px;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
        min-width: 120px;
        font-size: 18px;
    }}
    QTabBar::tab:selected {{
        background-color: {bg_primary};
        color: {accent_primary};
        border-bottom: 2px solid {accent_primary};
    }}
    QTabBar::tab:hover:!selected {{
        background-color: {bg_tertiary};
        color: {text_primary};
    }}
""".format(
    border_primary=COLOR_BORDER_PRIMARY,
    bg_primary=COLOR_BACKGROUND_PRIMARY,
    bg_secondary=COLOR_BACKGROUND_SECONDARY,
    text_tertiary=COLOR_TEXT_TERTIARY,
    accent_primary=COLOR_ACCENT_PRIMARY,
    bg_tertiary=COLOR_BACKGROUND_TERTIARY,
    text_primary=COLOR_TEXT_PRIMARY
)

TAB_WIDGET_STYLE_COMPACT = """
    QTabWidget::pane {{
        border: 1px solid {border_primary};
        background-color: {bg_primary};
        border-radius: 4px;
    }}
    QTabBar::tab {{
        background-color: {bg_secondary};
        color: {text_tertiary};
        padding: 8px 16px;
        margin-right: 2px;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
        min-width: 100px;
        font-size: 14px;
    }}
    QTabBar::tab:selected {{
        background-color: {bg_primary};
        color: {accent_primary};
        border-bottom: 2px solid {accent_primary};
    }}
    QTabBar::tab:hover:!selected {{
        background-color: {bg_tertiary};
        color: {text_primary};
    }}
""".format(
    border_primary=COLOR_BORDER_PRIMARY,
    bg_primary=COLOR_BACKGROUND_PRIMARY,
    bg_secondary=COLOR_BACKGROUND_SECONDARY,
    text_tertiary=COLOR_TEXT_TERTIARY,
    accent_primary=COLOR_ACCENT_PRIMARY,
    bg_tertiary=COLOR_BACKGROUND_TERTIARY,
    text_primary=COLOR_TEXT_PRIMARY
)

# Terminal/Text Area Styles
TERMINAL_STYLE = """
    QPlainTextEdit {{
        background-color: {bg_dark};
        color: {text_secondary};
        border: 1px solid {border_primary};
        border-radius: 4px;
        padding: 10px;
        font-size: 14px;
        font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
        selection-background-color: {accent_primary};
        selection-color: {text_primary};
    }}
""".format(
    bg_dark=COLOR_BACKGROUND_DARK,
    text_secondary=COLOR_TEXT_SECONDARY,
    border_primary=COLOR_BORDER_PRIMARY,
    accent_primary=COLOR_ACCENT_PRIMARY,
    text_primary=COLOR_TEXT_PRIMARY
)

# Splitter Style
SPLITTER_STYLE = """
    QSplitter::handle {{
        background-color: {bg_secondary};
        width: 4px;
    }}
    QSplitter::handle:hover {{
        background-color: {bg_tertiary};
    }}
""".format(
    bg_secondary=COLOR_BACKGROUND_SECONDARY,
    bg_tertiary=COLOR_BACKGROUND_TERTIARY
)

# Panel Background Style
PANEL_BACKGROUND_STYLE = "background-color: {bg_primary};".format(bg_primary=COLOR_BACKGROUND_PRIMARY)

# Navigation Drawer Styles
NAV_DRAWER_STYLE = """
    QListWidget {{
        background-color: {bg_secondary};
        border: none;
        outline: none;
        padding-top: 10px;
    }}
    QListWidget::item {{
        color: {text_secondary};
        padding: 20px 25px;
        border-bottom: 1px solid {border_primary};
        font-size: 16px;
        font-weight: bold;
    }}
    QListWidget::item:selected {{
        background-color: {accent_primary};
        color: {text_primary};
    }}
    QListWidget::item:hover:!selected {{
        background-color: {bg_tertiary};
    }}
""".format(
    bg_secondary=COLOR_BACKGROUND_SECONDARY,
    text_secondary=COLOR_TEXT_SECONDARY,
    border_primary=COLOR_BORDER_PRIMARY,
    accent_primary=COLOR_ACCENT_PRIMARY,
    text_primary=COLOR_TEXT_PRIMARY,
    bg_tertiary=COLOR_BACKGROUND_TERTIARY
)

# Toolbar Style
TOOLBAR_STYLE = """
    QLabel#ToolbarTitle {{
        font-size: 24px;
        font-weight: bold;
        color: {text_primary};
        margin-left: 10px;
    }}
""".format(
    text_primary=COLOR_TEXT_PRIMARY
)

HAMBURGER_BUTTON_STYLE = """
    QPushButton {{
        background: transparent;
        color: white;
        font-size: 40px;
        border: none;
        padding: 8px;
    }}
    QPushButton:hover {{
        background-color: {bg_secondary};
        border-radius: 4px;
    }}
""".format(bg_secondary=COLOR_BACKGROUND_SECONDARY)

