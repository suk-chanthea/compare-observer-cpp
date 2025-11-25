#include "file_watcher.h"
#include <QFileInfo>
#include <QDirIterator>
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QCoreApplication>
#include <QThread>

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
    
    // Preload file hashes to establish baseline
    preloadFileHashes();
    
    emit preloadComplete();
    
    // Watch the root path
    addWatchPath(m_watcher, m_watchPath);
    
    // Connect watcher signals
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        if (!isExcluded(path)) {
            emit fileChanged(path);
            calculateFileHash(path);
        }
    });
    
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
        if (isExcluded(path)) {
            return;
        }

        // Ensure new files/directories are watched
        QDir dir(path);
        const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& info : entries) {
            if (!isExcluded(info.absoluteFilePath())) {
                addWatchPath(m_watcher, info.absoluteFilePath());
                if (info.isFile()) {
                    emit fileCreated(info.absoluteFilePath());
                    calculateFileHash(info.absoluteFilePath());
                }
            }
        }
    });
    
    emit startedWatching();
    
    // Keep thread alive
    while (m_running) {
        QCoreApplication::processEvents();
        QThread::msleep(300);
    }
}

void WatcherThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    quit();
    wait();
    emit stoppedWatching();
}

void WatcherThread::preloadFileHashes()
{
    const int systemIndex = m_tableIndex + 1;
    emit logMessage(QString("Preloading file hashes for system %1").arg(systemIndex));
    
    QDirIterator it(m_watchPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    qint64 fileCount = 0;
    
    while (it.hasNext()) {
        if (!m_running) {
            emit logMessage(QString("Stopped preloading file hashes for system %1 after %2 files").arg(systemIndex).arg(fileCount));
            return;
        }
        
        QString path = it.next();
        
        if (isExcluded(path)) {
            continue;
        }

        addWatchPath(m_watcher, path);
        
        QFileInfo fileInfo(path);
        if (fileInfo.isFile()) {
            calculateFileHash(path);
            ++fileCount;
        }
    }

    emit logMessage(QString("Captured baseline for %1 file(s) in system %2").arg(fileCount).arg(systemIndex));
}

void WatcherThread::calculateFileHash(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logMessage("Error reading file: " + filePath);
        return;
    }
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    while (!file.atEnd()) {
        hash.addData(file.read(8192));
    }
    file.close();
    
    QString hashValue = hash.result().toHex();
    QMutexLocker locker(&m_mutex);
    m_fileHashes[filePath] = hashValue;
}

bool WatcherThread::isExcluded(const QString& filePath) const
{
    // Check if file matches excluded patterns
    for (const auto& folder : m_excludedFolders) {
        if (filePath.contains(folder)) {
            return true;
        }
    }
    
    for (const auto& file : m_excludedFiles) {
        if (filePath.endsWith(file)) {
            return true;
        }
    }
    
    return false;
}
