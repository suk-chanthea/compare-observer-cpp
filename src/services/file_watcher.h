#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <QThread>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QMap>

/**
 * @brief WatcherThread monitors a directory for file changes
 * 
 * This class watches for file changes and reports them.
 * The baseline content is managed by FileWatcherApp, not here.
 */
class WatcherThread : public QThread {
    Q_OBJECT

public:
    /**
     * @brief Constructs a WatcherThread
     * @param tableIndex Index of the monitoring table
     * @param systemName Name of the system being monitored
     * @param watchPath Root path to watch
     * @param excludedFolders Folders to exclude from monitoring
     * @param excludedFiles Files to exclude from monitoring
     */
    WatcherThread(int tableIndex,
                 const QString& systemName,
                 const QString& watchPath,
                 const QStringList& excludedFolders,
                 const QStringList& excludedFiles);
    
    ~WatcherThread();

    /**
     * @brief Stops the watcher thread
     */
    void stop();

signals:
    void startedWatching();
    void stoppedWatching();
    void preloadComplete();
    void fileChanged(const QString& filePath);
    void fileCreated(const QString& filePath);
    void fileDeleted(const QString& filePath);
    void logMessage(const QString& message);

protected:
    void run() override;

private:
    void addWatchRecursively(const QString& path);
    bool isExcluded(const QString& filePath) const;

    int m_tableIndex;
    QString m_systemName;
    QString m_watchPath;
    QStringList m_excludedFolders;
    QStringList m_excludedFiles;
    QFileSystemWatcher* m_watcher;
    bool m_running;
    QMutex m_mutex;
    QMap<QString, qint64> m_lastChangeTime;
};

#endif // FILE_WATCHER_H