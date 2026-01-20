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
    // m_watcher is cleaned up in run() when thread exits
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

    // Connect file change signal
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString& path) {
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

        // Check if this is a duplicate event (within 500ms)
        if (m_lastChangeTime.contains(path)) {
            qint64 timeSinceLastChange = currentModified - m_lastChangeTime[path];
            if (timeSinceLastChange < 500) {
                // Too soon - likely duplicate event, ignore
                return;
            }
        }

        // Update last change time
        m_lastChangeTime[path] = currentModified;

        // Re-add watch (QFileSystemWatcher removes it after change on some systems)
        if (!m_watcher->files().contains(path)) {
            m_watcher->addPath(path);
        }

        // Emit the change signal
        emit fileChanged(path);
        emit logMessage(QString("Change detected: %1").arg(path));
    }, Qt::QueuedConnection);

    // Connect directory change signal  
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
            [this](const QString& path) {
        if (isExcluded(path)) {
            return;
        }

        // Re-add watch for the directory itself
        if (!m_watcher->directories().contains(path)) {
            m_watcher->addPath(path);
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
                emit logMessage(QString("New file detected: %1").arg(filePath));
            } else if (info.isDir() && !m_watcher->directories().contains(filePath)) {
                addWatchPath(m_watcher, filePath);
                emit logMessage(QString("New directory detected: %1").arg(filePath));
            }
        }
    }, Qt::DirectConnection);

    emit startedWatching();

    // Use Qt's event loop instead of manual processing
    exec();
    
    // Clean up after event loop exits
    if (m_watcher) {
        m_watcher->disconnect();
        delete m_watcher;
        m_watcher = nullptr;
    }
}

void WatcherThread::stop()
{
    QMutexLocker locker(&m_mutex);
    if (!m_running) {
        return;
    }
    m_running = false;
    locker.unlock();
    
    // Request thread to exit event loop
    quit();
    
    // Wait for thread to finish with timeout
    if (!wait(5000)) {
        // Force terminate if it takes too long
        emit logMessage("Warning: Watcher thread forced to terminate");
        terminate();
        wait();
    }
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
    
    // Auto-exclude common temporary/backup file patterns
    QFileInfo fileInfo(normalized);
    QString fileName = fileInfo.fileName();
    
    // Exclude editor backup files (ending with ~)
    if (fileName.endsWith('~')) {
        return true;
    }
    
    // Exclude vim swap files
    if (fileName.endsWith(".swp") || fileName.endsWith(".swo") || 
        (fileName.startsWith(".") && fileName.contains(".sw"))) {
        return true;
    }
    
    // Exclude common temp files
    if (fileName.endsWith(".tmp") || fileName.endsWith(".temp") || 
        fileName.endsWith(".bak") || fileName.endsWith(".old")) {
        return true;
    }
    
    // Check excluded folders (from "Without" list)
    for (const auto& folder : m_excludedFolders) {
        const QString trimmed = folder.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (normalized.contains(trimmed, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    // Check excluded files (from "Except" list)
    for (const auto& file : m_excludedFiles) {
        const QString trimmed = file.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        
        // If pattern looks like a folder (starts with . and no extension, or ends with /)
        // Check if it appears as a path component
        if (trimmed.startsWith(".") && !trimmed.contains("..", Qt::CaseInsensitive) && 
            trimmed.lastIndexOf('.') == 0) {
            // It's a hidden folder like .idea, .git, .vscode
            QString folderPattern = QDir::separator() + trimmed + QDir::separator();
            if (normalized.contains(folderPattern, Qt::CaseInsensitive) ||
                normalized.endsWith(QDir::separator() + trimmed, Qt::CaseInsensitive)) {
                return true;
            }
        }
        // Check if path ends with the file pattern
        else if (normalized.endsWith(trimmed, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}