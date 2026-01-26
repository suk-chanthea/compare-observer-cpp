#include "file_watcher.h"
#include <QFileInfo>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>

WatcherThread::WatcherThread(int tableIndex,
                            const QString& systemName,
                            const QString& watchPath,
                            const QStringList& excludedFolders,
                            const QStringList& excludedFiles)
    : m_tableIndex(tableIndex),
      m_systemName(systemName),
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
    {
        QMutexLocker locker(&m_mutex);
        m_running = true;
    }

    // Create file system watcher in this thread
    m_watcher = new QFileSystemWatcher();

    // Signal that baseline should be captured now
    emit preloadComplete();

    // Watch the root path and all subdirectories
    addWatchRecursively(m_watchPath);

    // Connect file change signal
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString& path) {
        handleFileChanged(path);
    }, Qt::QueuedConnection);

    // Connect directory change signal  
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
            [this](const QString& path) {
        handleDirectoryChanged(path);
    }, Qt::DirectConnection);

    emit startedWatching();

    // Use Qt's event loop
    exec();
    
    // Clean up after event loop exits
    if (m_watcher) {
        m_watcher->disconnect();
        delete m_watcher;
        m_watcher = nullptr;
    }
    
    emit stoppedWatching();
}

void WatcherThread::handleFileChanged(const QString& path)
{
    if (isExcluded(path)) {
        return;
    }

    QFileInfo info(path);
    if (!info.exists()) {
        emit fileDeleted(path);
        return;
    }

    qint64 currentModified = info.lastModified().toMSecsSinceEpoch();

    // Thread-safe duplicate check
    if (isDuplicateEvent(path, currentModified)) {
        return;
    }

    // Re-add watch (QFileSystemWatcher removes it after change on some systems)
    if (!m_watcher->files().contains(path)) {
        m_watcher->addPath(path);
    }

    emit fileChanged(path);
    emit logMessage(QString("Change detected: %1").arg(path));
}

void WatcherThread::handleDirectoryChanged(const QString& path)
{
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
}

bool WatcherThread::isDuplicateEvent(const QString& path, qint64 currentTime)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_lastChangeTime.contains(path)) {
        qint64 timeSinceLastChange = currentTime - m_lastChangeTime[path];
        if (timeSinceLastChange < WatcherConfig::DUPLICATE_EVENT_THRESHOLD_MS) {
            return true;
        }
    }
    
    m_lastChangeTime[path] = currentTime;
    return false;
}

void WatcherThread::stop()
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_running) {
            return;
        }
        m_running = false;
    }
    
    // Request thread to exit event loop
    quit();
    
    // Wait for thread to finish with timeout
    if (!wait(5000)) {
        emit logMessage("Warning: Watcher thread forced to terminate");
        terminate();
        wait();
    }
}

void WatcherThread::addWatchRecursively(const QString& path)
{
    emit logMessage(QString("Setting up file monitoring for %1").arg(m_systemName));
    
    QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, 
                    QDirIterator::Subdirectories);
    qint64 fileCount = 0;
    
    while (it.hasNext()) {
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running) {
                emit logMessage(QString("Stopped monitoring setup for %1").arg(m_systemName));
                return;
            }
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

    emit logMessage(QString("Monitoring %1 file(s) in %2").arg(fileCount).arg(m_systemName));
}

bool WatcherThread::isExcluded(const QString& filePath) const
{
    const QString normalized = QDir::toNativeSeparators(filePath);
    
    // Auto-exclude common temporary/backup file patterns
    QFileInfo fileInfo(normalized);
    QString fileName = fileInfo.fileName();
    
    // Exclude editor backup files
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
    
    QString sep = QDir::separator();
    
    // Check excluded folders
    for (const auto& folder : m_excludedFolders) {
        const QString trimmed = folder.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        
        QString folderInPath = sep + trimmed + sep;
        QString folderAtEnd = sep + trimmed;
        
        if (normalized.contains(folderInPath, Qt::CaseInsensitive) ||
            normalized.endsWith(folderAtEnd, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    // Check excluded files
    for (const auto& file : m_excludedFiles) {
        const QString trimmed = file.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        
        bool isFolder = !trimmed.contains('.') || trimmed.startsWith(".");
        
        if (isFolder) {
            QString folderInPath = sep + trimmed + sep;
            QString folderAtEnd = sep + trimmed;
            
            if (normalized.contains(folderInPath, Qt::CaseInsensitive) ||
                normalized.endsWith(folderAtEnd, Qt::CaseInsensitive)) {
                return true;
            }
        } else {
            if (normalized.endsWith(trimmed, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    
    return false;
}