#include "file_watcher.h"
#include <QFileInfo>
#include <QDirIterator>
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

void WatcherThread::run()
{
    m_running = true;
    
    // Create file system watcher
    m_watcher = new QFileSystemWatcher();
    
    // Preload file hashes to establish baseline
    preloadFileHashes();
    
    emit preloadComplete();
    
    // Watch the root path
    m_watcher->addPath(m_watchPath);
    
    // Connect watcher signals
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        if (!isExcluded(path)) {
            emit fileChanged(path);
            calculateFileHash(path);
        }
    });
    
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
        if (!isExcluded(path)) {
            emit logMessage("Directory changed: " + path);
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
    emit logMessage("Preloading file hashes for table " + QString::number(m_tableIndex));
    
    QDirIterator it(m_watchPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    
    while (it.hasNext()) {
        if (!m_running) {
            emit logMessage("Stopped preloading file hashes");
            return;
        }
        
        QString path = it.next();
        
        if (isExcluded(path)) {
            continue;
        }
        
        QFileInfo fileInfo(path);
        if (fileInfo.isFile()) {
            calculateFileHash(path);
            emit logMessage(path);
        }
    }
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
