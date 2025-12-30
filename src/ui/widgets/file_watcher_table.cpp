#include "file_watcher_table.h"
#include <QHeaderView>
#include <QDateTime>

FileWatcherTable::FileWatcherTable(QWidget* parent)
    : QTableWidget(parent)
{
    setColumnCount(3);
    setHorizontalHeaderLabels({"File Path", "Status", "Modified"});
    horizontalHeader()->setStretchLastSection(false);
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
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