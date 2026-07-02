#include <QApplication>
#include <QTimer>
#include <string>
#include "MainWindow.h"

// Neutral, high-contrast light theme: light-gray canvas, white cards with a
// defined gray border and crisp (barely rounded) corners. Contrast comes from
// dark near-black elements (the Compute button, checked boxes, titles) rather
// than a colour accent. Segoe UI for chrome, monospace for the math I/O.
static const char* kStyle = R"QSS(
* {
    font-family: "Segoe UI";
    font-size: 10pt;
    color: #24292f;
}
QMainWindow, QWidget {
    background: #f4f5f7;
}
QGroupBox {
    background: #ffffff;
    border: 1px solid #d0d4d9;
    border-radius: 3px;
    margin-top: 18px;
    padding: 14px 12px 12px 12px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 1px 6px;
    background-color: #f4f5f7;
    color: #24292f;
    font-weight: 700;
}
QMenuBar {
    background: #f4f5f7;
    color: #24292f;
    padding: 2px 4px;
}
QMenuBar::item {
    background: transparent;
    padding: 5px 10px;
    border-radius: 3px;
}
QMenuBar::item:selected {
    background: #e2e5e9;
}
QMenuBar::item:pressed {
    background: #d7dbe0;
}
QMenu {
    background: #ffffff;
    border: 1px solid #d0d4d9;
    padding: 4px;
}
QMenu::item {
    padding: 6px 26px 6px 12px;
    border-radius: 3px;
}
QMenu::item:selected {
    background: #2f363d;
    color: #ffffff;
}
QMenu::separator {
    height: 1px;
    background: #e2e5e9;
    margin: 4px 6px;
}
QPlainTextEdit {
    background: #ffffff;
    border: 1px solid #d0d4d9;
    border-radius: 3px;
    padding: 8px;
    font-family: "JetBrains Mono", "Cascadia Mono", "Consolas", monospace;
    font-size: 10pt;
    selection-background-color: #d7dbe0;
    selection-color: #24292f;
}
QPushButton {
    background: #f6f8fa;
    border: 1px solid #ccd1d6;
    border-radius: 3px;
    padding: 6px 14px;
}
QPushButton:hover {
    background: #eceef1;
}
QPushButton#compute {
    background: #2f363d;
    border: 1px solid #2f363d;
    border-radius: 3px;
    color: #ffffff;
    font-weight: 600;
    padding: 9px 20px;
}
QPushButton#compute:hover {
    background: #3a424a;
}
QPushButton#compute:disabled {
    background: #a9b0b8;
    border-color: #a9b0b8;
}
QCheckBox {
    background: transparent;
    spacing: 8px;
    padding: 4px 2px;
}
QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid #ccd1d6;
    border-radius: 2px;
    background: #ffffff;
}
QCheckBox::indicator:hover {
    border-color: #57606a;
}
QCheckBox::indicator:checked {
    background: #2f363d;
    border-color: #2f363d;
}
QRadioButton {
    background: transparent;
    spacing: 8px;
    padding: 3px 2px;
}
QRadioButton:disabled {
    color: #9aa2ab;
}
QRadioButton::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid #ccd1d6;
    border-radius: 7px;
    background: #ffffff;
}
QRadioButton::indicator:checked {
    background: #2f363d;
    border-color: #2f363d;
}
QRadioButton::indicator:disabled {
    border-color: #dfe3e8;
    background: #f0f2f4;
}
QLabel#sectionLabel {
    color: #57606a;
    font-weight: 600;
    padding-top: 4px;
}
QToolBar {
    background: #f4f5f7;
    border: none;
    border-bottom: 1px solid #d0d4d9;
    spacing: 2px;
    padding: 4px 6px;
}
QComboBox {
    background: #ffffff;
    border: 1px solid #ccd1d6;
    border-radius: 3px;
    padding: 3px 8px;
    min-width: 84px;
}
QComboBox::drop-down {
    border: none;
    width: 18px;
}
QComboBox QAbstractItemView {
    background: #ffffff;
    border: 1px solid #d0d4d9;
    selection-background-color: #2f363d;
    selection-color: #ffffff;
}
QStatusBar {
    background: #f4f5f7;
    color: #57606a;
}
)QSS";

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyleSheet(kStyle);

    MainWindow w;
    w.resize(740, 560);
    w.show();

    // Hidden: `--shot <path>` renders the window to a PNG and exits.
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--shot") {
            QString path = QString::fromLocal8Bit(argv[i + 1]);
            QTimer::singleShot(500, [&w, path]() {
                w.grab().save(path);
                QApplication::quit();
            });
        }
    }

    return app.exec();
}
