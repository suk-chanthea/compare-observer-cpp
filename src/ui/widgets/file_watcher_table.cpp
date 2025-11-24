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
    QTableWidgetItem* timeItem = new QTableWidgetItem(QDateTime::currentDateTime().toString());

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
    item(row, 2)->setText(QDateTime::currentDateTime().toString());
}

void FileWatcherTable::removeFileEntry(const QString& filePath)
{
    if (m_fileRowMap.contains(filePath)) {
        int row = m_fileRowMap[filePath];
        removeRow(row);
        m_fileRowMap.remove(filePath);
        m_fileContents.remove(filePath);
    }
}

QString FileWatcherTable::getFileContent(const QString& filePath) const
{
    return m_fileContents.value(filePath, "");
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
