#include "antdesigntheme.h"

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QPalette>
#include <QString>
#include <QStyle>
#include <QStyleFactory>

void AntDesignTheme::Install(QApplication* _pApp)
{
    if (_pApp == nullptr) {
        return;
    }

    QStyle* _pStyleFusion = QStyleFactory::create("Fusion");
    if (_pStyleFusion != nullptr) {
        _pApp->setStyle(_pStyleFusion);
    }

    _pApp->setFont(BuildFont());
    _pApp->setPalette(BuildPalette());
    _pApp->setStyleSheet(BuildAppStyleSheet());
}

QFont AntDesignTheme::BuildFont()
{
    QFont _fontApp("Microsoft YaHei UI");
    _fontApp.setPointSize(10);
    _fontApp.setStyleHint(QFont::SansSerif);
    return _fontApp;
}

QPalette AntDesignTheme::BuildPalette()
{
    QPalette _paletteApp;
    _paletteApp.setColor(QPalette::Window, QColor("#F5F7FB"));
    _paletteApp.setColor(QPalette::WindowText, QColor("#1F2937"));
    _paletteApp.setColor(QPalette::Base, QColor("#FFFFFF"));
    _paletteApp.setColor(QPalette::AlternateBase, QColor("#F8FAFC"));
    _paletteApp.setColor(QPalette::Text, QColor("#1F2937"));
    _paletteApp.setColor(QPalette::Button, QColor("#FFFFFF"));
    _paletteApp.setColor(QPalette::ButtonText, QColor("#1F2937"));
    _paletteApp.setColor(QPalette::Highlight, QColor("#1677FF"));
    _paletteApp.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    _paletteApp.setColor(QPalette::ToolTipBase, QColor("#1F2937"));
    _paletteApp.setColor(QPalette::ToolTipText, QColor("#FFFFFF"));
    _paletteApp.setColor(QPalette::Disabled, QPalette::Text, QColor("#9CA3AF"));
    _paletteApp.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#9CA3AF"));
    return _paletteApp;
}

QString AntDesignTheme::BuildAppStyleSheet()
{
    return QStringLiteral(R"qss(
QMainWindow {
    background: #F5F7FB;
}

QWidget {
    color: #1F2937;
    selection-background-color: #BAE0FF;
    selection-color: #111827;
}

QMenuBar {
    background: #FFFFFF;
    color: #1F2937;
    border-bottom: 1px solid #E5E7EB;
    padding: 2px 8px;
}

QMenuBar::item {
    background: transparent;
    padding: 6px 10px;
    border-radius: 4px;
}

QMenuBar::item:selected {
    background: #E6F4FF;
    color: #0958D9;
}

QMenu {
    background: #FFFFFF;
    border: 1px solid #D9D9D9;
    border-radius: 6px;
    padding: 6px;
}

QMenu::item {
    min-width: 120px;
    padding: 7px 26px 7px 12px;
    border-radius: 4px;
}

QMenu::item:selected {
    background: #E6F4FF;
    color: #0958D9;
}

QMenu::separator {
    height: 1px;
    background: #F0F0F0;
    margin: 5px 4px;
}

QTabWidget::pane {
    background: #FFFFFF;
    border: 1px solid #E5E7EB;
    border-radius: 6px;
    top: -1px;
}

QTabWidget#tabWidgetRibbon::pane {
    border-left: none;
    border-right: none;
    border-bottom: 1px solid #E5E7EB;
    border-top: 1px solid #E5E7EB;
    border-radius: 0px;
}

QTabBar::tab {
    background: #F8FAFC;
    color: #4B5563;
    border: 1px solid #E5E7EB;
    border-bottom: none;
    padding: 8px 16px;
    min-width: 76px;
    min-height: 20px;
    margin-right: 2px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}

QTabBar::tab:hover {
    background: #E6F4FF;
    color: #0958D9;
}

QTabBar::tab:selected {
    background: #FFFFFF;
    color: #1677FF;
    border-color: #91CAFF;
}

QDockWidget {
    background: #FFFFFF;
    border: 1px solid #E5E7EB;
    titlebar-close-icon: none;
    titlebar-normal-icon: none;
}

QDockWidget::title {
    background: #FFFFFF;
    color: #1F2937;
    border-bottom: 1px solid #E5E7EB;
    padding: 8px 10px;
    text-align: left;
}

QGroupBox {
    background: #FFFFFF;
    border: 1px solid #E5E7EB;
    border-radius: 8px;
    margin-top: 8px;
    padding: 8px;
}

QGroupBox::title {
    color: #475569;
    subcontrol-origin: margin;
    left: 10px;
    padding: 0px 4px;
}

QLabel {
    color: #334155;
}

QPushButton {
    background: #FFFFFF;
    color: #1F2937;
    border: 1px solid #D9D9D9;
    border-radius: 6px;
    padding: 6px 12px;
    min-height: 24px;
}

QPushButton:hover {
    color: #1677FF;
    border-color: #1677FF;
    background: #F0F7FF;
}

QPushButton:pressed {
    color: #0958D9;
    border-color: #0958D9;
    background: #E6F4FF;
}

QPushButton:disabled {
    color: #A3AAB8;
    border-color: #E5E7EB;
    background: #F5F7FB;
}

QLineEdit,
QTextEdit,
QPlainTextEdit,
QListWidget,
QTreeView,
QTableView,
QSpinBox,
QDoubleSpinBox {
    background: #FFFFFF;
    color: #1F2937;
    border: 1px solid #D9D9D9;
    border-radius: 6px;
    padding: 5px;
}

QLineEdit:hover,
QTextEdit:hover,
QPlainTextEdit:hover,
QListWidget:hover,
QTreeView:hover,
QTableView:hover,
QSpinBox:hover,
QDoubleSpinBox:hover {
    border-color: #91CAFF;
}

QLineEdit:focus,
QTextEdit:focus,
QPlainTextEdit:focus,
QListWidget:focus,
QTreeView:focus,
QTableView:focus,
QSpinBox:focus,
QDoubleSpinBox:focus {
    border-color: #1677FF;
}

QListView::item,
QTreeView::item,
QTableView::item {
    min-height: 26px;
    padding: 4px 6px;
}

QListView::item:selected,
QTreeView::item:selected,
QTableView::item:selected {
    background: #E6F4FF;
    color: #0958D9;
}

QHeaderView::section {
    background: #F8FAFC;
    color: #475569;
    border: none;
    border-right: 1px solid #E5E7EB;
    border-bottom: 1px solid #E5E7EB;
    padding: 6px 8px;
}

QScrollBar:vertical {
    background: transparent;
    width: 9px;
    margin: 2px;
}

QScrollBar:horizontal {
    background: transparent;
    height: 9px;
    margin: 2px;
}

QScrollBar::handle {
    background: #CBD5E1;
    border-radius: 4px;
}

QScrollBar::handle:hover {
    background: #94A3B8;
}

QScrollBar::add-line,
QScrollBar::sub-line,
QScrollBar::add-page,
QScrollBar::sub-page {
    width: 0px;
    height: 0px;
    background: transparent;
}

QStatusBar {
    background: #FFFFFF;
    color: #475569;
    border-top: 1px solid #E5E7EB;
}

QStatusBar QLabel {
    color: #475569;
}

QSplitter::handle {
    background: #E5E7EB;
}

QToolTip {
    background: #1F2937;
    color: #FFFFFF;
    border: none;
    border-radius: 4px;
    padding: 6px 8px;
}
)qss");
}
