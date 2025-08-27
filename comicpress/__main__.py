import sys
from PyQt6 import QtWidgets
from .gui import App

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)

    app.setStyleSheet("""
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding-left: 5px;
        }
    """)

    font = app.font()
    font.setPointSize(14)
    app.setFont(font)

    window = App()
    window.show()
    sys.exit(app.exec())
