#include "file_watcher.h"
#include <QFileInfo>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>

WatcherThread::WatcherThread(int tableIndex,
                            const QString& watchPath,
                            const QStringList& excludedFolders,
                            const QStringList& excludedFiles)
    : m_tableIndex(tableIndex),
      m_watchPath(watchPath),
      m_excludedFolders(excludedFolders),
      m_excludedFiles(excludedFiles),
      m_watcher(nullptr),
      m_running(false)
{
}

WatcherThread::~WatcherThread()
{
    stop();
    if (m_watcher) {
        delete m_watcher;
    }
}

static void addWatchPath(QFileSystemWatcher* watcher, const QString& path)
{
    if (!watcher) {
        return;
    }

    QFileInfo info(path);
    if (info.isDir()) {
        if (!watcher->directories().contains(path)) {
            watcher->addPath(path);
        }
    } else if (info.isFile()) {
        if (!watcher->files().contains(path)) {
            watcher->addPath(path);
        }
    }
}

void WatcherThread::run()
{
    m_running = true;

    // Create file system watcher
    m_watcher = new QFileSystemWatcher();

    // Signal that baseline should be captured now
    emit preloadComplete();

    // Watch the root path and all subdirectories
    addWatchRecursively(m_watchPath);

    // Use a debounce timer to prevent duplicate events
    QTimer* debounceTimer = new QTimer();
    debounceTimer->setInterval(100); // 100ms debounce
    debounceTimer->setSingleShot(true);

    QMap<QString, qint64> pendingChanges; // file path -> last modified time

    connect(debounceTimer, &QTimer::timeout, this, [this, &pendingChanges]() {
        // Process all pending changes
        for (auto it = pendingChanges.begin(); it != pendingChanges.end(); ++it) {
            const QString& path = it.key();

            // Re-add watch (Qt removes it after change)
            if (QFileInfo::exists(path)) {
                // Small delay before re-adding to prevent immediate re-trigger
                QThread::msleep(50);
                m_watcher->addPath(path);
            }

            // Emit the change
            emit fileChanged(path);
        }
        pendingChanges.clear();
    });

    // Connect watcher signals
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
            [this, debounceTimer, &pendingChanges](const QString& path) {
        if (isExcluded(path)) {
            return;
        }

        QFileInfo info(path);
        if (!info.exists()) {
            // File was deleted
            emit fileDeleted(path);
            return;
        }

        // Get current modified time
        qint64 currentModified = info.lastModified().toMSecsSinceEpoch();

        // Check if this is a duplicate event (same file, within debounce window)
        if (pendingChanges.contains(path)) {
            qint64 previousModified = pendingChanges[path];
            if (qAbs(currentModified - previousModified) < 100) {
                // Duplicate event within 100ms - ignore
                return;
            }
        }

        // Add to pending changes
        pendingChanges[path] = currentModified;

        // Restart debounce timer
        debounceTimer->stop();
        debounceTimer->start();
    });

    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
            [this](const QString& path) {
        if (isExcluded(path)) {
            return;
        }

        // Scan for new files in changed directory
        QDir dir(path);
        const QFileInfoList entries = dir.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo& info : entries) {
            const QString filePath = info.absoluteFilePath();
            if (isExcluded(filePath)) {
                continue;
            }

            // Add watch if not already watching
            if (info.isFile() && !m_watcher->files().contains(filePath)) {
                addWatchPath(m_watcher, filePath);
                emit fileCreated(filePath);
            } else if (info.isDir() && !m_watcher->directories().contains(filePath)) {
                addWatchPath(m_watcher, filePath);
            }
        }
    });

    emit startedWatching();

    // Keep thread alive
    while (m_running) {
        QCoreApplication::processEvents();
        QThread::msleep(300);
    }

    delete debounceTimer;
}

void WatcherThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    quit();
    wait();
    emit stoppedWatching();
}

void WatcherThread::addWatchRecursively(const QString& path)
{
    const int systemIndex = m_tableIndex + 1;
    emit logMessage(QString("Setting up file monitoring for system %1").arg(systemIndex));
    
    QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, 
                    QDirIterator::Subdirectories);
    qint64 fileCount = 0;
    
    while (it.hasNext()) {
        if (!m_running) {
            emit logMessage(QString("Stopped monitoring setup for system %1").arg(systemIndex));
            return;
        }
        
        QString filePath = it.next();
        
        if (isExcluded(filePath)) {
            continue;
        }

        addWatchPath(m_watcher, filePath);
        
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile()) {
            ++fileCount;
        }
    }

    emit logMessage(QString("Monitoring %1 file(s) in system %2").arg(fileCount).arg(systemIndex));
}

bool WatcherThread::isExcluded(const QString& filePath) const
{
    const QString normalized = QDir::toNativeSeparators(filePath);
    
    // Check excluded folders
    for (const auto& folder : m_excludedFolders) {
        const QString trimmed = folder.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (normalized.contains(trimmed, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    // Check excluded files
    for (const auto& file : m_excludedFiles) {
        const QString trimmed = file.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (normalized.endsWith(trimmed, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}