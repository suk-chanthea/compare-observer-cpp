#ifndef FILE_WATCHER_TABLE_H
#define FILE_WATCHER_TABLE_H

#include <QTableWidget>
#include <QMap>
#include <QString>

/**
 * @brief Custom table widget for displaying watched files
 */
class FileWatcherTable : public QTableWidget {
    Q_OBJECT

public:
    explicit FileWatcherTable(QWidget* parent = nullptr);

    /**
     * @brief Adds a file entry to the table
     */
    void addFileEntry(const QString& filePath, const QString& status);

    /**
     * @brief Updates a file entry
     */
    void updateFileEntry(const QString& filePath, const QString& status);

    /**
     * @brief Removes a file entry
     */
    void removeFileEntry(const QString& filePath);

    /**
     * @brief Gets the stored file content
     */
    QString getFileContent(const QString& filePath) const;

    /**
     * @brief Sets the file content
     */
    void setFileContent(const QString& filePath, const QString& content);

    /**
     * @brief Clears the table
     */
    void clearTable();

    /**
     * @brief Gets all file keys (relative paths) from the table
     */
    QStringList getAllFileKeys() const;

signals:
    /**
     * @brief Emitted when user wants to view file diff
     */
    void viewDiffRequested(const QString& filePath);

private slots:
    void onCellClicked(int row, int column);
    void onDeleteClicked(int row);

private:
    void addDeleteButton(int row);

    QMap<QString, QString> m_fileContents;
    QMap<QString, int> m_fileRowMap;
};

#endif // FILE_WATCHER_TABLE_H
