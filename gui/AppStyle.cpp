#include "gui/AppStyle.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QStyleFactory>

namespace scapms::gui {

void applyAppStyle(QApplication& app) {
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));   // same look on Windows, macOS and Linux

    QFont font = app.font();
    font.setPointSize(10);
    app.setFont(font);

    QPalette palette = app.palette();
    palette.setColor(QPalette::Window, QColor(0xF4, 0xF6, 0xF9));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(0xF7, 0xF9, 0xFC));
    palette.setColor(QPalette::Text, QColor(0x1F, 0x29, 0x37));
    palette.setColor(QPalette::WindowText, QColor(0x1F, 0x29, 0x37));
    palette.setColor(QPalette::Highlight, QColor(0xD6, 0xE6, 0xFA));
    palette.setColor(QPalette::HighlightedText, QColor(0x10, 0x2A, 0x43));
    palette.setColor(QPalette::ToolTipBase, QColor(0x1F, 0x29, 0x37));
    palette.setColor(QPalette::ToolTipText, Qt::white);
    app.setPalette(palette);

    // Combo boxes, date/time edits and spin boxes are left to the native Fusion style
    // (styling them with a style sheet would hide their arrow buttons).
    app.setStyleSheet(QStringLiteral(R"(
        QGroupBox {
            background: white; border: 1px solid #DDE3EA; border-radius: 8px;
            margin-top: 14px; padding: 12px 10px 10px 10px; font-weight: 600;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; color: #0B5C8E; }
        QTabWidget::pane { border: none; }
        QTabBar::tab {
            padding: 9px 18px; margin-right: 2px; color: #4B5563; background: transparent;
            border-bottom: 3px solid transparent;
        }
        QTabBar::tab:selected { color: #0B5C8E; border-bottom: 3px solid #0B5C8E; font-weight: 600; }
        QTabBar::tab:hover:!selected { color: #111827; }
        QPushButton {
            padding: 6px 14px; border: 1px solid #C9D2DC; border-radius: 6px; background: white;
        }
        QPushButton:hover { background: #EEF4FA; }
        QPushButton:disabled { color: #A0A8B3; background: #F3F4F6; }
        QPushButton#primaryButton { background: #0B5C8E; color: white; border-color: #0B5C8E; font-weight: 600; }
        QPushButton#primaryButton:hover { background: #094A73; }
        QPushButton#primaryButton:disabled { background: #9DB8CC; border-color: #9DB8CC; }
        QPushButton#dangerButton { color: #B42318; border-color: #F0B4AE; }
        QPushButton#dangerButton:hover { background: #FDECEA; }
        QLineEdit {
            padding: 4px 6px; border: 1px solid #C9D2DC; border-radius: 5px; background: white; min-height: 22px;
        }
        QLineEdit:focus { border-color: #0B5C8E; }
        QLineEdit:read-only { background: #F3F4F6; color: #4B5563; }
        QTableWidget { border: 1px solid #DDE3EA; border-radius: 6px; gridline-color: #EEF1F5; }
        QHeaderView::section {
            background: #EEF2F7; padding: 6px; border: none; border-bottom: 1px solid #DDE3EA; font-weight: 600;
        }
        QListWidget { border: 1px solid #DDE3EA; border-radius: 6px; background: white; }
        QListWidget::item { padding: 4px 2px; }
        QToolBar { background: white; border-bottom: 1px solid #DDE3EA; spacing: 4px; padding: 3px; }
        QToolBar QToolButton { padding: 4px 10px; border-radius: 5px; }
        QToolBar QToolButton:hover { background: #EEF4FA; }
        QStatusBar { background: white; border-top: 1px solid #DDE3EA; }
        QLabel#statusField { padding: 0 10px; color: #4B5563; }
        QLabel#pageTitle { font-size: 20px; font-weight: 700; color: #0B2540; }
        QLabel#sectionTitle { font-size: 15px; font-weight: 700; color: #0B2540; }
        QLabel#hintLabel { color: #6B7280; }
        QLabel#infoLabel { color: #0B5C8E; }
        QLabel#clockLabel { font-size: 24px; font-weight: 700; color: #0B5C8E; }
        QFrame#statCard { background: white; border: 1px solid #DDE3EA; border-radius: 8px; }
        QLabel#cardValue { font-size: 26px; font-weight: 700; color: #0B2540; }
        QLabel#cardCaption { color: #6B7280; }
        QFrame#servingFrame { background: #E8F3FB; border: 1px solid #B9D8EE; border-radius: 8px; }
        QLabel#servingLabel { font-size: 16px; font-weight: 700; color: #0B5C8E; }
        QLabel#summaryBox { background: white; border: 1px solid #DDE3EA; border-radius: 6px; padding: 8px; }
        QProgressBar { border: 1px solid #C9D2DC; border-radius: 4px; text-align: center; background: #F3F4F6; }
        QProgressBar::chunk { background: #0B5C8E; border-radius: 3px; }
    )"));
}

} // namespace scapms::gui
