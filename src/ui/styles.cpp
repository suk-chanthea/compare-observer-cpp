#include "styles.h"

namespace Styles {

QString getMainStylesheet()
{
    return R"(
        QMainWindow {
            background-color: #121212;
            color: #E5E5E5;
        }
        QWidget {
            background-color: #121212;
            color: #DADADA;
        }
        QMenuBar {
            background-color: #1C1C1C;
            color: #EDEDED;
            padding: 4px;
        }
        QMenuBar::item {
            background-color: transparent;
            color: #E0E0E0;
            padding: 4px 12px;
            border-radius: 4px 4px 0 0;
        }
        QMenuBar::item[activeMenu="true"] {
            background-color: #1C1C1C;
            color: #FFFFFF;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #1C1C1C;
            color: #FFFFFF;
        }
        QPushButton {
            background-color: #2A2A2A;
            color: #FAFAFA;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            padding: 6px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #3A3A3A;
        }
        QPushButton:pressed {
            background-color: #4A4A4A;
        }
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: #1B1B1B;
            color: #F3F3F3;
            border: 1px solid #343434;
            border-radius: 4px;
        }
    )";
}

QString getDialogStylesheet()
{
    return R"(
        QDialog {
            background-color: #151515;
            color: #EDEDED;
        }
        QLabel {
            color: #E1E1E1;
        }
        QLineEdit, QTextEdit {
            border: 1px solid #343434;
            border-radius: 4px;
            padding: 6px;
            background-color: #1C1C1C;
            color: #F3F3F3;
        }
        QLineEdit:focus, QTextEdit:focus {
            border: 1px solid #0B57D0;
            background-color: #222222;
        }
        QPushButton {
            background-color: #2D2D2D;
            color: #FAFAFA;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #3A3A3A;
        }
    )";
}

QString getTableStylesheet()
{
    return R"(
        QTableWidget {
            background-color: #1A1A1A;
            gridline-color: #2A2A2A;
            border: 1px solid #2A2A2A;
            color: #F7F7F7;
        }
        QTableWidget::item {
            padding: 4px;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: #2E6ED6;
            color: white;
        }
        QHeaderView::section {
            background-color: #212121;
            color: #E0E0E0;
            padding: 6px;
            border: 1px solid #2A2A2A;
        }
    )";
}

}
