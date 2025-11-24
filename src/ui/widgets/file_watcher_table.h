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

private:
    QMap<QString, QString> m_fileContents;
    QMap<QString, int> m_fileRowMap;
};

#endif // FILE_WATCHER_TABLE_H
