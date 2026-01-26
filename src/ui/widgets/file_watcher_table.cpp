#include "file_watcher_table.h"
#include <QHeaderView>
#include <QDateTime>
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidget>

FileWatcherTable::FileWatcherTable(QWidget* parent)
    : QTableWidget(parent)
{
    setColumnCount(4);
    setHorizontalHeaderLabels({"File Path", "Status", "Modified", "Action"});
    horizontalHeader()->setStretchLastSection(false);
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    setColumnWidth(3, 80);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Connect cell click to show diff
    connect(this, &QTableWidget::cellClicked, this, &FileWatcherTable::onCellClicked);
}

void FileWatcherTable::addFileEntry(const QString& filePath, const QString& status)
{
    if (m_fileRowMap.contains(filePath)) {
        updateFileEntry(filePath, status);
        return;
    }

    int row = rowCount();
    insertRow(row);

    QTableWidgetItem* pathItem = new QTableWidgetItem(filePath);
    QTableWidgetItem* statusItem = new QTableWidgetItem(status);
    QTableWidgetItem* timeItem = new QTableWidgetItem(
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    setItem(row, 0, pathItem);
    setItem(row, 1, statusItem);
    setItem(row, 2, timeItem);
    
    // No background color - keep it plain
    
    // Add delete button
    addDeleteButton(row);

    m_fileRowMap[filePath] = row;
}

void FileWatcherTable::updateFileEntry(const QString& filePath, const QString& status)
{
    if (!m_fileRowMap.contains(filePath)) {
        addFileEntry(filePath, status);
        return;
    }

    int row = m_fileRowMap[filePath];
    item(row, 1)->setText(status);
    item(row, 2)->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    
    // No background color - keep it plain
}

void FileWatcherTable::removeFileEntry(const QString& filePath)
{
    if (m_fileRowMap.contains(filePath)) {
        int row = m_fileRowMap[filePath];
        removeRow(row);

        // Rebuild the row map since row indices changed
        m_fileRowMap.clear();
        for (int i = 0; i < rowCount(); ++i) {
            if (item(i, 0)) {
                m_fileRowMap[item(i, 0)->text()] = i;
            }
        }

        m_fileContents.remove(filePath);
    }
}

QString FileWatcherTable::getFileContent(const QString& filePath) const
{
    // Return null QString if no baseline exists (different from empty string "")
    if (!m_fileContents.contains(filePath)) {
        return QString(); // null QString indicates "no baseline"
    }
    return m_fileContents.value(filePath);
}

void FileWatcherTable::setFileContent(const QString& filePath, const QString& content)
{
    m_fileContents[filePath] = content;
}

void FileWatcherTable::clearTable()
{
    setRowCount(0);
    m_fileRowMap.clear();
    m_fileContents.clear();
}

void FileWatcherTable::addDeleteButton(int row)
{
    // Create container widget with centered layout
    QWidget* container = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);
    
    // Add stretch before button to center it
    layout->addStretch();
    
    // Create delete button with trash icon
    QPushButton* deleteBtn = new QPushButton("🗑");
    deleteBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #C62828;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 6px 10px;"
        "   font-size: 16px;"
        "   min-width: 32px;"
        "   max-width: 32px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #D32F2F;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #B71C1C;"
        "}"
    );
    deleteBtn->setToolTip("Remove from list");
    deleteBtn->setCursor(Qt::PointingHandCursor);
    
    // Connect delete button - find row dynamically to handle row shifts
    connect(deleteBtn, &QPushButton::clicked, this, [this, deleteBtn]() {
        // Find the current row of this button (handles row index changes after deletions)
        for (int i = 0; i < rowCount(); ++i) {
            QWidget* widget = cellWidget(i, 3);
            if (widget && widget->findChild<QPushButton*>() == deleteBtn) {
                onDeleteClicked(i);
                break;
            }
        }
    });
    
    layout->addWidget(deleteBtn);
    
    // Add stretch after button to center it
    layout->addStretch();
    
    setCellWidget(row, 3, container);
}

void FileWatcherTable::onCellClicked(int row, int column)
{
    // Only show diff when clicking on file path, status, or modified columns
    // Don't trigger on action column
    if (column >= 0 && column < 3 && row >= 0 && row < rowCount()) {
        QTableWidgetItem* pathItem = item(row, 0);
        if (pathItem) {
            QString filePath = pathItem->text();
            emit viewDiffRequested(filePath);
        }
    }
}

void FileWatcherTable::onDeleteClicked(int row)
{
    if (row >= 0 && row < rowCount()) {
        QTableWidgetItem* pathItem = item(row, 0);
        if (pathItem) {
            QString filePath = pathItem->text();
            removeFileEntry(filePath);
        }
    }
}

QStringList FileWatcherTable::getAllFileKeys() const
{
    return m_fileRowMap.keys();
}