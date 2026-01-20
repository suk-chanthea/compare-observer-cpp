#include "main_window.h"
#include "services/file_watcher.h"
#include "services/telegram_service.h"
#include "ui/dialogs/log_dialog.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/dialogs/file_diff_dialog.h"
#include "ui/dialogs/git_compare_dialog.h"
#include "ui/dialogs/change_review_dialog.h"
#include "ui/widgets/file_watcher_table.h"
#include "ui/styles.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScrollArea>
#include <QDirIterator>
#include <QFile>
#include <QDateTime>
#include <QInputDialog>
#include <QDialog>
#include <QTextEdit>

FileWatcherApp::FileWatcherApp(QWidget* parent)
    : QMainWindow(parent),
      m_watchToggleButton(new QPushButton("Start Watching")),
      m_logsButton(new QPushButton("View Logs")),
      m_statusLabel(new QLabel("Status: Idle")),
      m_panelContainer(nullptr),
      m_panelLayout(nullptr),
      m_bodyStack(nullptr),
      m_watcherPage(nullptr),
      m_gitPage(nullptr),
      m_logDialog(std::make_unique<LogDialog>(this)),
      m_settingsDialog(std::make_unique<SettingsDialog>(this)),
      m_diffDialog(std::make_unique<FileDiffDialog>(this)),
      m_gitCompareWidget(nullptr),
      m_changeReviewDialog(std::make_unique<ChangeReviewDialog>(this)),
      m_notificationsEnabled(false),
      m_isWatching(false)
{
    setWindowTitle("Compare Observer - File Watcher");
    setGeometry(100, 100, 1200, 700);

    setupUI();
    createMenuBar();
    connectSignals();
    loadSettings();

    setStyleSheet(Styles::getMainStylesheet());
}

FileWatcherApp::~FileWatcherApp()
{
    stopAllWatchers();
}

void FileWatcherApp::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(16);

    m_titleLabel = new QLabel("File Monitoring");
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: 600; color: #F5F5F5;");
    mainLayout->addWidget(m_titleLabel);

    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->addStretch();
    m_watchToggleButton->setCheckable(false);
    controlLayout->addWidget(m_watchToggleButton);
    controlLayout->addWidget(m_logsButton);
    mainLayout->addLayout(controlLayout);

    m_statusLabel->setStyleSheet("color: #CFCFCF;");
    mainLayout->addWidget(m_statusLabel);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    QWidget* scrollWidget = new QWidget();
    m_panelLayout = new QVBoxLayout(scrollWidget);
    m_panelLayout->setSpacing(24);
    m_panelLayout->setContentsMargins(0, 0, 0, 0);
    scrollArea->setWidget(scrollWidget);
    m_panelContainer = scrollWidget;

    m_bodyStack = new QStackedWidget(this);

    m_watcherPage = new QWidget();
    QVBoxLayout* watcherPageLayout = new QVBoxLayout(m_watcherPage);
    watcherPageLayout->setContentsMargins(0, 0, 0, 0);
    watcherPageLayout->setSpacing(0);
    watcherPageLayout->addWidget(scrollArea);
    m_bodyStack->addWidget(m_watcherPage);

    m_gitPage = new QWidget();
    QVBoxLayout* gitLayout = new QVBoxLayout(m_gitPage);
    gitLayout->setContentsMargins(0, 0, 0, 0);
    gitLayout->setSpacing(0);
    m_gitCompareWidget = new GitSourceCompareDialog(this);
    m_gitCompareWidget->setWindowFlag(Qt::Dialog, false);
    m_gitCompareWidget->setModal(false);
    gitLayout->addWidget(m_gitCompareWidget);
    m_bodyStack->addWidget(m_gitPage);

    showWatcherPage();
    mainLayout->addWidget(m_bodyStack);
}

void FileWatcherApp::createMenuBar()
{
    m_watcherMenuAction = menuBar()->addAction("Watcher");
    connect(m_watcherMenuAction, &QAction::triggered, this, [this]() {
        showWatcherPage();
    });

    m_gitMenuAction = menuBar()->addAction("Git ↔ Source");
    connect(m_gitMenuAction, &QAction::triggered, this, [this]() {
        showGitPage();
    });

    QMenu* settingsMenu = menuBar()->addMenu("Settings");
    m_settingsMenuAction = settingsMenu->menuAction();
    QAction* openSettingsAction = settingsMenu->addAction("Application Settings");
    QAction* aboutAction = settingsMenu->addAction("About");

    connect(openSettingsAction, &QAction::triggered, this, [this]() {
        setMenuActive(m_settingsMenuAction);
        onSettingsClicked();
    });
    connect(aboutAction, &QAction::triggered, this, [this]() {
        setMenuActive(m_settingsMenuAction);
        QMessageBox::about(this, "About Compare Observer",
            "Compare Observer v1.0\n\nA file monitoring application with Telegram notifications.");
    });

    setMenuActive(m_watcherMenuAction);
}

void FileWatcherApp::connectSignals()
{
    connect(m_watchToggleButton, &QPushButton::clicked, this, &FileWatcherApp::onToggleWatching);
    connect(m_logsButton, &QPushButton::clicked, this, &FileWatcherApp::onViewLogs);
}

void FileWatcherApp::loadSettings()
{
    QSettings settings("CompareObserver", "FileWatcher");
    m_username = settings.value("username", "").toString();
    m_telegramToken = settings.value("telegramToken", "").toString();
    m_telegramChatId = settings.value("telegramChatId", "").toString();
    m_notificationsEnabled = settings.value("notificationsEnabled", false).toBool();

    m_systemConfigs.clear();
    int systemCount = settings.beginReadArray("systems");
    for (int i = 0; i < systemCount; ++i) {
        settings.setArrayIndex(i);
        SettingsDialog::SystemConfigData data;
        data.name = settings.value("name").toString();
        data.source = settings.value("source").toString();
        data.destination = settings.value("destination").toString();
        data.git = settings.value("git").toString();
        data.backup = settings.value("backup").toString();
        data.assign = settings.value("assign").toString();
        m_systemConfigs.append(data);
    }
    settings.endArray();

    if (m_systemConfigs.isEmpty()) {
        SettingsDialog::SystemConfigData data;
        m_systemConfigs.append(data);
    }

    m_watchPath = m_systemConfigs.first().source;

    m_withoutRules.clear();
    int withoutRows = settings.beginReadArray("without");
    for (int i = 0; i < withoutRows; ++i) {
        settings.setArrayIndex(i);
        QStringList entries = settings.value("entries").toStringList();
        m_withoutRules.append(entries);
    }
    settings.endArray();

    m_exceptRules.clear();
    int exceptRows = settings.beginReadArray("except");
    for (int i = 0; i < exceptRows; ++i) {
        settings.setArrayIndex(i);
        QStringList entries = settings.value("entries").toStringList();
        m_exceptRules.append(entries);
    }
    settings.endArray();

    m_settingsDialog->setUsername(m_username);
    m_settingsDialog->setTelegramToken(m_telegramToken);
    m_settingsDialog->setTelegramChatId(m_telegramChatId);
    m_settingsDialog->setNotificationsEnabled(m_notificationsEnabled);
    m_settingsDialog->setSystemConfigs(m_systemConfigs);

    bool needRemoteWithout = m_withoutRules.isEmpty();
    bool needRemoteExcept = m_exceptRules.isEmpty();

    if (!needRemoteWithout) {
        m_settingsDialog->setWithoutData(m_withoutRules);
    }
    if (!needRemoteExcept) {
        m_settingsDialog->setExceptData(m_exceptRules);
    }

    if ((needRemoteWithout || needRemoteExcept) && m_settingsDialog->loadRemoteRuleDefaults()) {
        m_withoutRules = m_settingsDialog->withoutData();
        m_exceptRules = m_settingsDialog->exceptData();
        needRemoteWithout = false;
        needRemoteExcept = false;
    }

    if (needRemoteWithout) {
        m_settingsDialog->loadWithoutDefaults();
        m_withoutRules = m_settingsDialog->withoutData();
    }
    if (needRemoteExcept) {
        m_settingsDialog->loadExceptDefaults();
        m_exceptRules = m_settingsDialog->exceptData();
    }

    // Initialize Telegram service if enabled and configured
    if (m_notificationsEnabled && !m_telegramToken.isEmpty() && !m_telegramChatId.isEmpty()) {
        m_telegramService = std::make_unique<TelegramService>(m_telegramToken, m_telegramChatId);
        
        // Connect to error signal to show Telegram errors in logs
        connect(m_telegramService.get(), &TelegramService::error, this, [this](const QString& errorMsg) {
            m_logDialog->addLog("❌ Telegram Error: " + errorMsg);
        });
        
        // Connect to success signal
        connect(m_telegramService.get(), &TelegramService::messageSent, this, [this](bool success) {
            if (success) {
                m_logDialog->addLog("✅ Telegram message delivered successfully!");
            }
        });
        
        m_logDialog->addLog("✅ Telegram service initialized on startup");
    } else {
        m_logDialog->addLog("ℹ️ Telegram service not initialized (enable in Settings)");
    }

    rebuildSystemPanels();
}

void FileWatcherApp::saveSettings()
{
    QSettings settings("CompareObserver", "FileWatcher");
    settings.setValue("username", m_username);
    settings.setValue("telegramToken", m_telegramToken);
    settings.setValue("telegramChatId", m_telegramChatId);
    settings.setValue("notificationsEnabled", m_notificationsEnabled);

    settings.beginWriteArray("systems");
    for (int i = 0; i < m_systemConfigs.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", m_systemConfigs[i].name);
        settings.setValue("source", m_systemConfigs[i].source);
        settings.setValue("destination", m_systemConfigs[i].destination);
        settings.setValue("git", m_systemConfigs[i].git);
        settings.setValue("backup", m_systemConfigs[i].backup);
        settings.setValue("assign", m_systemConfigs[i].assign);
    }
    settings.endArray();

    settings.beginWriteArray("without");
    for (int i = 0; i < m_withoutRules.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("entries", m_withoutRules[i]);
    }
    settings.endArray();

    settings.beginWriteArray("except");
    for (int i = 0; i < m_exceptRules.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("entries", m_exceptRules[i]);
    }
    settings.endArray();
}

void FileWatcherApp::rebuildSystemPanels()
{
    stopAllWatchers();
    if (!m_panelLayout) {
        return;
    }

    clearSystemPanels();

    if (m_systemConfigs.isEmpty()) {
        SettingsDialog::SystemConfigData data;
        m_systemConfigs.append(data);
    }

    for (int i = 0; i < m_systemConfigs.size(); ++i) {
        buildSystemPanel(i, m_systemConfigs.at(i));
    }

    m_panelLayout->addStretch();
}

void FileWatcherApp::clearSystemPanels()
{
    if (m_panelLayout) {
        QLayoutItem* item = nullptr;
        while ((item = m_panelLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }
    m_systemPanels.clear();
}

void FileWatcherApp::buildSystemPanel(int index, const SettingsDialog::SystemConfigData& config)
{
    if (!m_panelLayout) {
        return;
    }

    SystemPanel panel;
    panel.container = new QWidget();
    QVBoxLayout* panelLayout = new QVBoxLayout(panel.container);
    panelLayout->setSpacing(8);
    panelLayout->setContentsMargins(12, 0, 12, 0);

    QHBoxLayout* descriptionRow = new QHBoxLayout();
    descriptionRow->setSpacing(8);

    QString systemName = config.name.isEmpty() ? QString("System %1").arg(index + 1) : config.name;
    QLabel* descriptionLabel = new QLabel(QString("Description for %1:").arg(systemName));
    descriptionLabel->setStyleSheet("color: #CCCCCC; font-weight: 500;");
    panel.descriptionEdit = new QLineEdit();
    panel.descriptionEdit->setPlaceholderText("Enter description here...");
    panel.descriptionEdit->setClearButtonEnabled(true);

    descriptionRow->addWidget(descriptionLabel);
    descriptionRow->addWidget(panel.descriptionEdit, 1);
    panelLayout->addLayout(descriptionRow);

    QHBoxLayout* tableRow = new QHBoxLayout();
    tableRow->setSpacing(12);

    panel.table = new FileWatcherTable();
    panel.table->setMinimumHeight(160);
    tableRow->addWidget(panel.table, 1);

    QVBoxLayout* buttonsLayout = new QVBoxLayout();
    buttonsLayout->setSpacing(8);

    panel.copyButton = new QPushButton("Copy");
    panel.copyButton->setStyleSheet("background-color: #0B57D0; color: white; padding: 8px 16px; border-radius: 4px;");
    buttonsLayout->addWidget(panel.copyButton);

    panel.copySendButton = new QPushButton("Copy Send");
    panel.copySendButton->setStyleSheet("background-color: #1E8449; color: white; padding: 8px 16px; border-radius: 4px;");
    buttonsLayout->addWidget(panel.copySendButton);

    panel.assignToButton = new QPushButton("Assign To");
    panel.assignToButton->setStyleSheet("background-color: #8E44AD; color: white; padding: 8px 16px; border-radius: 4px;");
    buttonsLayout->addWidget(panel.assignToButton);

    panel.gitCompareButton = new QPushButton("Git Compare");
    panel.gitCompareButton->setStyleSheet("background-color: #F39C12; color: #1E1E1E; padding: 8px 16px; border-radius: 4px;");
    buttonsLayout->addWidget(panel.gitCompareButton);
    buttonsLayout->addStretch();

    tableRow->addLayout(buttonsLayout);
    panelLayout->addLayout(tableRow);

    int systemIndex = index;
    
    // Connect signal to show diff when file is clicked
    connect(panel.table, &FileWatcherTable::viewDiffRequested, this, [this, systemIndex](const QString& filePath) {
        handleViewDiffRequested(systemIndex, filePath);
    });
    
    connect(panel.copyButton, &QPushButton::clicked, this, [this, systemIndex]() {
        handleCopyRequested(systemIndex);
    });
    connect(panel.copySendButton, &QPushButton::clicked, this, [this, systemIndex]() {
        handleCopySendRequested(systemIndex);
    });
    connect(panel.assignToButton, &QPushButton::clicked, this, [this, systemIndex]() {
        handleAssignToRequested(systemIndex);
    });
    connect(panel.gitCompareButton, &QPushButton::clicked, this, [this, systemIndex]() {
        handleGitCompareRequested(systemIndex);
    });

    panel.watcher = nullptr;
    m_systemPanels.append(panel);
    m_panelLayout->addWidget(panel.container);
}

QStringList FileWatcherApp::ruleListForSystem(const QVector<QStringList>& rows, int systemIndex) const
{
    QStringList values;
    for (const QStringList& row : rows) {
        if (systemIndex < row.size()) {
            const QString value = row.at(systemIndex).trimmed();
            if (!value.isEmpty()) {
                values << value;
            }
        }
    }
    return values;
}

void FileWatcherApp::stopAllWatchers()
{
    for (int i = 0; i < m_systemPanels.size(); ++i) {
        auto& panel = m_systemPanels[i];
        if (panel.watcher) {
            m_logDialog->addLog(QString("Stopping %1 watcher...").arg(getSystemName(i)));
            panel.watcher->stop();
            m_logDialog->addLog(QString("%1 watcher stopped").arg(getSystemName(i)));
            panel.watcher->deleteLater();
            panel.watcher = nullptr;
        }
    }
}

void FileWatcherApp::handleFileChanged(int systemIndex, const QString& filePath)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }

    auto& panel = m_systemPanels[systemIndex];
    const QString sourceRoot = m_systemConfigs.value(systemIndex).source;
    const QString relative = QDir(sourceRoot).relativeFilePath(filePath);
    
    m_logDialog->addLog(QString("%1: Processing change event for %2").arg(getSystemName(systemIndex)).arg(relative));
    
    if (!panel.table) {
        return;
    }

    // Check if file still exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        m_logDialog->addLog(QString("%1: File disappeared during change - %2")
            .arg(getSystemName(systemIndex)).arg(relative));
        return;
    }

    // Read NEW content from disk
    QString newContent = readFileContent(filePath);
    if (newContent.isNull()) {
        m_logDialog->addLog(QString("%1: Failed to read file - %2")
            .arg(getSystemName(systemIndex)).arg(filePath));
        return;
    }

    // Get OLD content from baseline (stored when watching started)
    QString oldContent = panel.table->getFileContent(relative);

    // CRITICAL FIX: Check if we have a baseline for this file
    bool hasBaseline = !oldContent.isNull();

    if (!hasBaseline) {
        // This is a newly created file (no baseline exists)
        panel.table->setFileContent(relative, newContent);
        panel.table->addFileEntry(relative, "Created");
        m_logDialog->addLog(QString("%1: New file created - %2")
            .arg(getSystemName(systemIndex)).arg(relative));

        // Automatic Telegram notification removed - only send via "Copy Send" button
        return;
    }

    // CRITICAL: Compare old vs new content byte-by-byte
    if (oldContent == newContent) {
        // File content hasn't actually changed (maybe just timestamp/attributes)
        // This is a false alarm - log but don't show in table
        m_logDialog->addLog(QString("%1: Ignored false change for %2 (content identical)")
            .arg(getSystemName(systemIndex)).arg(relative));
        return;
    }

    // Content has REALLY changed - calculate change size for logging
    qint64 oldSize = oldContent.toUtf8().size();
    qint64 newSize = newContent.toUtf8().size();
    qint64 sizeDiff = newSize - oldSize;
    QString sizeInfo = QString("(%1 bytes → %2 bytes, %3%4)")
        .arg(oldSize)
        .arg(newSize)
        .arg(sizeDiff >= 0 ? "+" : "")
        .arg(sizeDiff);

    // Update table
    panel.table->updateFileEntry(relative, "Modified");
    m_logDialog->addLog(QString("%1: File modified - %2 %3")
        .arg(getSystemName(systemIndex)).arg(relative).arg(sizeInfo));

    // DO NOT update baseline - keep original content for comparison
    // panel.table->setFileContent(relative, newContent);

    // Automatic Telegram notification removed - only send via "Copy Send" button
}

void FileWatcherApp::handleFileCreated(int systemIndex, const QString& filePath)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }

    auto& panel = m_systemPanels[systemIndex];
    const QString sourceRoot = m_systemConfigs.value(systemIndex).source;
    if (panel.table) {
        const QString relative = QDir(sourceRoot).relativeFilePath(filePath);
        panel.table->addFileEntry(relative, "Created");
    }

    QString content = readFileContent(filePath);
    if (!content.isNull() && panel.table) {
        const QString relative = QDir(sourceRoot).relativeFilePath(filePath);
        panel.table->setFileContent(relative, content);
    }

    m_logDialog->addLog(QString("%1: File created - %2").arg(getSystemName(systemIndex)).arg(filePath));
}

void FileWatcherApp::handleFileDeleted(int systemIndex, const QString& filePath)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }

    auto& panel = m_systemPanels[systemIndex];
    const QString sourceRoot = m_systemConfigs.value(systemIndex).source;
    if (panel.table) {
        const QString relative = QDir(sourceRoot).relativeFilePath(filePath);
        panel.table->removeFileEntry(relative);
    }

    m_logDialog->addLog(QString("%1: File deleted - %2").arg(getSystemName(systemIndex)).arg(filePath));
}

void FileWatcherApp::handleCopyRequested(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    
    if (systemIndex >= m_systemConfigs.size()) {
        m_logDialog->addLog(QString("Error: Invalid system configuration for %1").arg(getSystemName(systemIndex)));
        return;
    }
    
    const auto& config = m_systemConfigs[systemIndex];
    const auto& panel = m_systemPanels[systemIndex];
    
    // Get all files from the watcher table
    QStringList filesToCopy = panel.table->getAllFileKeys();
    
    if (filesToCopy.isEmpty()) {
        QMessageBox::warning(this, "No Files", 
                            QString("%1: No files in watcher list to copy.").arg(getSystemName(systemIndex)));
        return;
    }
    
    // Check if at least one destination path is configured (folders will be created automatically)
    if (config.destination.isEmpty() && config.git.isEmpty() && config.backup.isEmpty()) {
        QMessageBox::warning(this, "No Paths Configured", 
                            QString("%1: No destination paths configured.\n\nPlease configure at least one path (Destination, Git, or Backup) in Settings.")
                            .arg(getSystemName(systemIndex)));
        return;
    }
    
    m_logDialog->addLog(QString("%1: Starting copy of %2 file(s)...").arg(getSystemName(systemIndex)).arg(filesToCopy.size()));
    
    int successCount = 0;
    int failCount = 0;
    
    // Get current date and time for backup folder structure
    QDateTime now = QDateTime::currentDateTime();
    QString dateFolder = now.toString("yyyy-MM-dd");
    QString timeFolder = now.toString("HH-mm-ss");
    
    for (const QString& relativeFilePath : filesToCopy) {
        QString sourceFile = QDir(config.source).filePath(relativeFilePath);
        
        if (!QFile::exists(sourceFile)) {
            m_logDialog->addLog(QString("  ✗ Source file not found: %1").arg(relativeFilePath));
            failCount++;
            continue;
        }
        
        bool fileSuccess = true;
        
        // Copy to destination (flat, except lang folder)
        if (!config.destination.isEmpty()) {
            QString destPath;
            QString fileName = QFileInfo(relativeFilePath).fileName();
            
            // Check if file is in lang folder
            if (relativeFilePath.startsWith("lang/", Qt::CaseInsensitive) || 
                relativeFilePath.startsWith("lang\\", Qt::CaseInsensitive)) {
                // Keep folder structure for lang folder
                destPath = QDir(config.destination).filePath(relativeFilePath);
            } else {
                // Flat copy (only filename)
                destPath = QDir(config.destination).filePath(fileName);
            }
            
            QString destDir = QFileInfo(destPath).absolutePath();
            if (!QDir().mkpath(destDir)) {
                m_logDialog->addLog(QString("  ✗ Failed to create dest directory: %1").arg(destDir));
                fileSuccess = false;
            } else {
                if (QFile::exists(destPath)) {
                    QFile::remove(destPath);
                }
                if (!QFile::copy(sourceFile, destPath)) {
                    m_logDialog->addLog(QString("  ✗ Failed to copy to dest: %1").arg(destPath));
                    fileSuccess = false;
                }
            }
        }
        
        // Copy to git (keep folder structure)
        if (!config.git.isEmpty()) {
            QString gitPath = QDir(config.git).filePath(relativeFilePath);
            QString gitDir = QFileInfo(gitPath).absolutePath();
            
            if (!QDir().mkpath(gitDir)) {
                m_logDialog->addLog(QString("  ✗ Failed to create git directory: %1").arg(gitDir));
                fileSuccess = false;
            } else {
                if (QFile::exists(gitPath)) {
                    QFile::remove(gitPath);
                }
                if (!QFile::copy(sourceFile, gitPath)) {
                    m_logDialog->addLog(QString("  ✗ Failed to copy to git: %1").arg(gitPath));
                    fileSuccess = false;
                }
            }
        }
        
        // Copy to backup with date/time folder structure
        if (!config.backup.isEmpty()) {
            QString backupPath = QDir(config.backup).filePath(dateFolder + "/" + timeFolder + "/" + relativeFilePath);
            QString backupDir = QFileInfo(backupPath).absolutePath();
            
            if (!QDir().mkpath(backupDir)) {
                m_logDialog->addLog(QString("  ✗ Failed to create backup directory: %1").arg(backupDir));
                fileSuccess = false;
            } else {
                if (QFile::exists(backupPath)) {
                    QFile::remove(backupPath);
                }
                if (!QFile::copy(sourceFile, backupPath)) {
                    m_logDialog->addLog(QString("  ✗ Failed to copy to backup: %1").arg(backupPath));
                    fileSuccess = false;
                }
            }
        }
        
        if (fileSuccess) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    m_logDialog->addLog(QString("%1: Copy complete - %2 succeeded, %3 failed")
                        .arg(getSystemName(systemIndex)).arg(successCount).arg(failCount));
    
    // Clear the watcher table after successful copy
    if (successCount > 0) {
        panel.table->clearTable();
        m_logDialog->addLog(QString("%1: Watcher list cleared").arg(getSystemName(systemIndex)));
    }
}

void FileWatcherApp::handleCopySendRequested(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    
    if (systemIndex >= m_systemConfigs.size()) {
        m_logDialog->addLog(QString("Error: Invalid system configuration for %1").arg(getSystemName(systemIndex)));
        return;
    }
    
    const auto& config = m_systemConfigs[systemIndex];
    const auto& panel = m_systemPanels[systemIndex];
    
    // Get all files from the watcher table
    QStringList filesToCopy = panel.table->getAllFileKeys();
    
    if (filesToCopy.isEmpty()) {
        QMessageBox::warning(this, "No Files", 
                            QString("%1: No files in watcher list to copy & send.").arg(getSystemName(systemIndex)));
        return;
    }
    
    // Check if at least one destination path is configured (folders will be created automatically)
    if (config.destination.isEmpty() && config.git.isEmpty() && config.backup.isEmpty()) {
        QMessageBox::warning(this, "No Paths Configured", 
                            QString("%1: No destination paths configured.\n\nPlease configure at least one path (Destination, Git, or Backup) in Settings.")
                            .arg(getSystemName(systemIndex)));
        return;
    }
    
    m_logDialog->addLog(QString("%1: Starting copy & send of %2 file(s)...").arg(getSystemName(systemIndex)).arg(filesToCopy.size()));
    
    int successCount = 0;
    int failCount = 0;
    QStringList copiedFiles;
    
    // Get current date and time for backup folder structure
    QDateTime now = QDateTime::currentDateTime();
    QString dateFolder = now.toString("yyyy-MM-dd");
    QString timeFolder = now.toString("HH-mm-ss");
    
    for (const QString& relativeFilePath : filesToCopy) {
        QString sourceFile = QDir(config.source).filePath(relativeFilePath);
        
        if (!QFile::exists(sourceFile)) {
            m_logDialog->addLog(QString("  ✗ Source file not found: %1").arg(relativeFilePath));
            failCount++;
            continue;
        }
        
        bool fileSuccess = true;
        
        // Copy to destination (flat, except lang folder)
        if (!config.destination.isEmpty()) {
            QString destPath;
            QString fileName = QFileInfo(relativeFilePath).fileName();
            
            // Check if file is in lang folder
            if (relativeFilePath.startsWith("lang/", Qt::CaseInsensitive) || 
                relativeFilePath.startsWith("lang\\", Qt::CaseInsensitive)) {
                // Keep folder structure for lang folder
                destPath = QDir(config.destination).filePath(relativeFilePath);
            } else {
                // Flat copy (only filename)
                destPath = QDir(config.destination).filePath(fileName);
            }
            
            QString destDir = QFileInfo(destPath).absolutePath();
            if (!QDir().mkpath(destDir)) {
                m_logDialog->addLog(QString("  ✗ Failed to create dest directory: %1").arg(destDir));
                fileSuccess = false;
            } else {
                if (QFile::exists(destPath)) {
                    QFile::remove(destPath);
                }
                if (!QFile::copy(sourceFile, destPath)) {
                    m_logDialog->addLog(QString("  ✗ Failed to copy to dest: %1").arg(destPath));
                    fileSuccess = false;
                }
            }
        }
        
        // Copy to git (keep folder structure)
        if (!config.git.isEmpty()) {
            QString gitPath = QDir(config.git).filePath(relativeFilePath);
            QString gitDir = QFileInfo(gitPath).absolutePath();
            
            if (!QDir().mkpath(gitDir)) {
                m_logDialog->addLog(QString("  ✗ Failed to create git directory: %1").arg(gitDir));
                fileSuccess = false;
            } else {
                if (QFile::exists(gitPath)) {
                    QFile::remove(gitPath);
                }
                if (!QFile::copy(sourceFile, gitPath)) {
                    m_logDialog->addLog(QString("  ✗ Failed to copy to git: %1").arg(gitPath));
                    fileSuccess = false;
                }
            }
        }
        
        // Copy to backup with date/time folder structure
        if (!config.backup.isEmpty()) {
            QString backupPath = QDir(config.backup).filePath(dateFolder + "/" + timeFolder + "/" + relativeFilePath);
            QString backupDir = QFileInfo(backupPath).absolutePath();
            
            if (!QDir().mkpath(backupDir)) {
                m_logDialog->addLog(QString("  ✗ Failed to create backup directory: %1").arg(backupDir));
                fileSuccess = false;
            } else {
                if (QFile::exists(backupPath)) {
                    QFile::remove(backupPath);
                }
                if (!QFile::copy(sourceFile, backupPath)) {
                    m_logDialog->addLog(QString("  ✗ Failed to copy to backup: %1").arg(backupPath));
                    fileSuccess = false;
                }
            }
        }
        
        if (fileSuccess) {
            successCount++;
            copiedFiles << relativeFilePath;
        } else {
            failCount++;
        }
    }
    
    m_logDialog->addLog(QString("%1: Copy complete - %2 succeeded, %3 failed")
                        .arg(getSystemName(systemIndex)).arg(successCount).arg(failCount));
    
    // Send Telegram notification if enabled and files were copied
    m_logDialog->addLog(QString("Telegram check - Enabled: %1, Service: %2, Token: %3, ChatID: %4")
                        .arg(m_notificationsEnabled ? "Yes" : "No")
                        .arg(m_telegramService ? "Yes" : "No")
                        .arg(m_telegramToken.isEmpty() ? "Empty" : "Set")
                        .arg(m_telegramChatId.isEmpty() ? "Empty" : "Set"));
    
    if (!copiedFiles.isEmpty()) {
        if (!m_notificationsEnabled) {
            m_logDialog->addLog(QString("%1: Telegram disabled (enable in Settings)").arg(getSystemName(systemIndex)));
        } else if (m_telegramToken.isEmpty() || m_telegramChatId.isEmpty()) {
            m_logDialog->addLog(QString("%1: Telegram not configured (set Token and Chat ID in Settings)").arg(getSystemName(systemIndex)));
        } else if (!m_telegramService) {
            m_logDialog->addLog(QString("%1: Telegram service not initialized").arg(getSystemName(systemIndex)));
        } else {
            // Get description from the panel
            QString description = panel.descriptionEdit->text().trimmed();
            if (description.isEmpty()) {
                description = getSystemName(systemIndex);
            }
            
            // Format file list: flat for regular files, keep path for lang files
            QStringList formattedFiles;
            for (const QString& file : copiedFiles) {
                if (file.startsWith("lang/", Qt::CaseInsensitive) || 
                    file.startsWith("lang\\", Qt::CaseInsensitive)) {
                    // Keep folder structure for lang files
                    formattedFiles << "- " + file;
                } else {
                    // Only filename for regular files
                    formattedFiles << "- " + QFileInfo(file).fileName();
                }
            }
            
            // Create HTML formatted message (escape HTML special characters)
            auto escapeHtml = [](const QString& text) -> QString {
                QString escaped = text;
                escaped.replace("&", "&amp;");
                escaped.replace("<", "&lt;");
                escaped.replace(">", "&gt;");
                return escaped;
            };
            
            QString title = QString("<code>%1: %2</code>").arg(escapeHtml(m_username)).arg(escapeHtml(description));
            QString fileList = formattedFiles.join("\n");
            QString message = QString("%1\n\n%2").arg(title).arg(fileList);
            
            m_logDialog->addLog(QString("%1: Sending Telegram message...").arg(getSystemName(systemIndex)));
            m_logDialog->addLog(QString("Message content:\n%1").arg(message));
            m_logDialog->addLog(QString("Chat ID: %1").arg(m_telegramChatId));
            m_logDialog->addLog(QString("Bot Token: %1...%2 (length: %3)")
                .arg(m_telegramToken.left(8))
                .arg(m_telegramToken.right(4))
                .arg(m_telegramToken.length()));
            
            m_telegramService->sendMessage(m_username, message, "");
            m_logDialog->addLog(QString("%1: Telegram send request initiated").arg(getSystemName(systemIndex)));
        }
    }
    
    // Clear the watcher table after successful copy
    if (successCount > 0) {
        panel.table->clearTable();
        m_logDialog->addLog(QString("%1: Watcher list cleared").arg(getSystemName(systemIndex)));
    }
}

void FileWatcherApp::handleAssignToRequested(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    
    if (systemIndex >= m_systemConfigs.size()) {
        m_logDialog->addLog(QString("Error: Invalid system configuration for %1").arg(getSystemName(systemIndex)));
        return;
    }
    
    const auto& config = m_systemConfigs[systemIndex];
    const auto& panel = m_systemPanels[systemIndex];
    
    // Get all files from the watcher table first
    QStringList filesToAssign = panel.table->getAllFileKeys();
    
    if (filesToAssign.isEmpty()) {
        QMessageBox::warning(this, "No Files", 
                            QString("%1: No files in watcher list to assign.").arg(getSystemName(systemIndex)));
        return;
    }
    
    // Check if assign path is configured (folder will be created automatically)
    if (config.assign.isEmpty()) {
        QMessageBox::warning(this, "No Assign Path", 
                            QString("%1: Assign path not configured.\n\nPlease set the Assign path in Settings.")
                            .arg(getSystemName(systemIndex)));
        return;
    }
    
    // Create custom dialog for name and description
    QDialog dialog(this);
    dialog.setWindowTitle("Assign To");
    dialog.setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Name input
    layout->addWidget(new QLabel("Folder Name:"));
    QLineEdit* nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText("Enter folder name...");
    layout->addWidget(nameEdit);
    
    layout->addSpacing(10);
    
    // Description input
    layout->addWidget(new QLabel("Description:"));
    QTextEdit* descEdit = new QTextEdit(&dialog);
    descEdit->setPlaceholderText("Enter description...");
    descEdit->setMinimumHeight(100);
    layout->addWidget(descEdit);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &dialog);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Show dialog
    if (dialog.exec() != QDialog::Accepted) {
        m_logDialog->addLog(QString("%1: Assign cancelled").arg(getSystemName(systemIndex)));
        return;
    }
    
    QString folderName = nameEdit->text().trimmed();
    QString description = descEdit->toPlainText().trimmed();
    
    if (folderName.isEmpty()) {
        m_logDialog->addLog(QString("%1: Folder name is required").arg(getSystemName(systemIndex)));
        return;
    }
    
    // Get current date and time for folder
    QDateTime now = QDateTime::currentDateTime();
    QString dateTimeFolder = now.toString("yyyy-MM-dd_HH-mm-ss");
    
    // Create folder structure: assign_path/folderName/dateTimeFolder/
    QString targetBasePath = QDir(config.assign).filePath(folderName + "/" + dateTimeFolder);
    
    if (!QDir().mkpath(targetBasePath)) {
        m_logDialog->addLog(QString("%1: Failed to create folder: %2").arg(getSystemName(systemIndex)).arg(targetBasePath));
        return;
    }
    
    m_logDialog->addLog(QString("%1: Starting assign to folder: %2").arg(getSystemName(systemIndex)).arg(folderName));
    
    // Create description.txt file if description is provided
    if (!description.isEmpty()) {
        QString descriptionFile = QDir(targetBasePath).filePath("description.txt");
        QFile file(descriptionFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << description;
            file.close();
            m_logDialog->addLog(QString("  ✓ Created description.txt"));
        } else {
            m_logDialog->addLog(QString("  ✗ Failed to create description.txt"));
        }
    }
    
    int successCount = 0;
    int failCount = 0;
    
    // Copy all files to the target folder
    for (const QString& relativeFilePath : filesToAssign) {
        QString sourceFile = QDir(config.source).filePath(relativeFilePath);
        
        if (!QFile::exists(sourceFile)) {
            m_logDialog->addLog(QString("  ✗ Source file not found: %1").arg(relativeFilePath));
            failCount++;
            continue;
        }
        
        // Copy to target folder (keep full folder structure)
        QString targetFile = QDir(targetBasePath).filePath(relativeFilePath);
        QString targetDir = QFileInfo(targetFile).absolutePath();
        
        bool fileSuccess = true;
        
        if (!QDir().mkpath(targetDir)) {
            m_logDialog->addLog(QString("  ✗ Failed to create directory: %1").arg(targetDir));
            fileSuccess = false;
        } else {
            if (QFile::exists(targetFile)) {
                QFile::remove(targetFile);
            }
            if (!QFile::copy(sourceFile, targetFile)) {
                m_logDialog->addLog(QString("  ✗ Failed to copy: %1").arg(relativeFilePath));
                fileSuccess = false;
            } else {
                m_logDialog->addLog(QString("  ✓ Copied: %1").arg(relativeFilePath));
            }
        }
        
        if (fileSuccess) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    m_logDialog->addLog(QString("%1: Assign complete - %2 succeeded, %3 failed")
                        .arg(getSystemName(systemIndex)).arg(successCount).arg(failCount));
    m_logDialog->addLog(QString("Files assigned to: %1").arg(targetBasePath));
    
    // Clear the watcher table after successful assignment
    if (successCount > 0) {
        panel.table->clearTable();
        m_logDialog->addLog(QString("%1: Watcher list cleared").arg(getSystemName(systemIndex)));
    }
}

void FileWatcherApp::handleGitCompareRequested(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    m_logDialog->addLog(QString("Git compare requested for %1").arg(getSystemName(systemIndex)));
    showGitPage();
}

void FileWatcherApp::handleViewDiffRequested(int systemIndex, const QString& filePath)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    
    auto& panel = m_systemPanels[systemIndex];
    if (!panel.table) {
        return;
    }
    
    // Get the baseline (old) content
    QString oldContent = panel.table->getFileContent(filePath);
    
    // Get the absolute file path
    const QString sourceRoot = m_systemConfigs.value(systemIndex).source;
    QString absolutePath = QDir(sourceRoot).absoluteFilePath(filePath);
    
    // Check if file exists
    if (!QFile::exists(absolutePath)) {
        QMessageBox::warning(this, "File Not Found", 
            QString("Could not read file: %1").arg(filePath));
        return;
    }
    
    // If no baseline, use empty string
    if (oldContent.isNull()) {
        oldContent = "";
        m_diffDialog->setWindowTitle(QString("%1: %2 (New File) - Live View").arg(getSystemName(systemIndex)).arg(filePath));
    } else {
        m_diffDialog->setWindowTitle(QString("%1: %2 - Live View").arg(getSystemName(systemIndex)).arg(filePath));
    }
    
    // Use live file monitoring
    m_diffDialog->setLiveFile(absolutePath, oldContent);
    m_diffDialog->show();
    m_diffDialog->raise();
    m_diffDialog->activateWindow();
    
    m_logDialog->addLog(QString("Viewing live diff for %1: %2").arg(getSystemName(systemIndex)).arg(filePath));
}

void FileWatcherApp::setMenuActive(QAction* activeAction)
{
    QList<QAction*> menuActions = { m_watcherMenuAction, m_gitMenuAction, m_settingsMenuAction };
    for (QAction* action : menuActions) {
        if (!action) {
            continue;
        }
        bool isActive = (action == activeAction);
        action->setProperty("activeMenu", isActive);
    }

    if (menuBar()) {
        menuBar()->style()->unpolish(menuBar());
        menuBar()->style()->polish(menuBar());
        menuBar()->update();
    }
}

void FileWatcherApp::showWatcherPage()
{
    if (m_bodyStack && m_watcherPage) {
        m_bodyStack->setCurrentWidget(m_watcherPage);
    }
    if (m_titleLabel) {
        m_titleLabel->setText("File Monitoring");
    }
    if (m_watchToggleButton) {
        m_watchToggleButton->setVisible(true);
    }
    if (m_logsButton) {
        m_logsButton->setVisible(true);
    }
    if (m_statusLabel) {
        m_statusLabel->setVisible(true);
    }
    setMenuActive(m_watcherMenuAction);
}

void FileWatcherApp::showGitPage()
{
    if (m_bodyStack && m_gitPage) {
        m_bodyStack->setCurrentWidget(m_gitPage);
    }
    if (m_titleLabel) {
        m_titleLabel->setText("Scan Compare Git vs Source");
    }
    if (m_watchToggleButton) {
        m_watchToggleButton->setVisible(false);
    }
    if (m_logsButton) {
        m_logsButton->setVisible(false);
    }
    if (m_statusLabel) {
        m_statusLabel->setVisible(false);
    }
    if (m_gitCompareWidget) {
        m_gitCompareWidget->raise();
        m_gitCompareWidget->activateWindow();
    }
    setMenuActive(m_gitMenuAction);
}

void FileWatcherApp::captureBaselineForSystem(int systemIndex,
                                              const SettingsDialog::SystemConfigData& config,
                                              const QStringList& excludedFolders,
                                              const QStringList& excludedFiles)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    if (config.source.isEmpty()) {
        return;
    }

    auto& panel = m_systemPanels[systemIndex];
    if (!panel.table) {
        return;
    }

    QDirIterator it(config.source, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int fileCount = 0;

    while (it.hasNext()) {
        QString path = it.next();
        if (isPathExcluded(path, excludedFolders, excludedFiles)) {
            continue;
        }

        QString content = readFileContent(path);
        if (!content.isNull()) {
            const QString relative = QDir(config.source).relativeFilePath(path);
            panel.table->setFileContent(relative, content);
            ++fileCount;
        }
    }

    if (fileCount > 0) {
        m_logDialog->addLog(QString("%1: Captured baseline for %2 files").arg(getSystemName(systemIndex)).arg(fileCount));
    }
}

bool FileWatcherApp::isPathExcluded(const QString& absolutePath,
                                    const QStringList& excludedFolders,
                                    const QStringList& excludedFiles) const
{
    const QString normalized = QDir::toNativeSeparators(absolutePath);

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
    for (const QString& folder : excludedFolders) {
        const QString trimmed = folder.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (normalized.contains(trimmed, Qt::CaseInsensitive)) {
            return true;
        }
    }

    // Check excluded files (from "Except" list)
    for (const QString& filePattern : excludedFiles) {
        const QString trimmed = filePattern.trimmed();
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

QString FileWatcherApp::readFileContent(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QByteArray data = file.readAll();
    file.close();
    return QString::fromUtf8(data);
}

QString FileWatcherApp::getSystemName(int systemIndex) const
{
    if (systemIndex >= 0 && systemIndex < m_systemConfigs.size()) {
        const QString& name = m_systemConfigs[systemIndex].name;
        if (!name.isEmpty()) {
            return name;
        }
    }
    return getSystemName(systemIndex);
}

void FileWatcherApp::onStartWatching()
{
    bool hasSource = false;
    for (const auto& config : m_systemConfigs) {
        if (!config.source.isEmpty()) {
            hasSource = true;
            break;
        }
    }

    if (!hasSource) {
        QMessageBox::warning(this, "Configuration Error", "Please set a source path for at least one system in settings.");
        onSettingsClicked();
        return;
    }

    startWatching();
}

void FileWatcherApp::onStopWatching()
{
    stopWatching();
}

void FileWatcherApp::onSettingsClicked()
{
    bool wasWatching = m_isWatching;
    if (wasWatching) {
        stopWatching();
    }

    if (m_settingsDialog->exec() == QDialog::Accepted) {
        m_username = m_settingsDialog->getUsername();
        m_watchPath = m_settingsDialog->getWatchPath();
        m_telegramToken = m_settingsDialog->getTelegramToken();
        m_telegramChatId = m_settingsDialog->getTelegramChatId();
        m_notificationsEnabled = m_settingsDialog->isNotificationsEnabled();
        m_systemConfigs = m_settingsDialog->systemConfigs();
        m_withoutRules = m_settingsDialog->withoutData();
        m_exceptRules = m_settingsDialog->exceptData();

        if (m_systemConfigs.isEmpty()) {
            SettingsDialog::SystemConfigData data;
            m_systemConfigs.append(data);
        }
        if (m_watchPath.isEmpty()) {
            m_watchPath = m_systemConfigs.first().source;
        }

        saveSettings();
        rebuildSystemPanels();

        if (m_notificationsEnabled && !m_telegramToken.isEmpty() && !m_telegramChatId.isEmpty()) {
            m_telegramService = std::make_unique<TelegramService>(m_telegramToken, m_telegramChatId);
            
            // Connect to error signal to show Telegram errors in logs
            connect(m_telegramService.get(), &TelegramService::error, this, [this](const QString& errorMsg) {
                m_logDialog->addLog("❌ Telegram Error: " + errorMsg);
            });
            
            // Connect to success signal
            connect(m_telegramService.get(), &TelegramService::messageSent, this, [this](bool success) {
                if (success) {
                    m_logDialog->addLog("✅ Telegram message delivered successfully!");
                } else {
                    m_logDialog->addLog("❌ Telegram message failed to deliver");
                }
            });
            
            m_logDialog->addLog("✅ Telegram service initialized successfully");
        } else {
            m_telegramService.reset();
            m_logDialog->addLog("Telegram service disabled");
        }
    }
}

void FileWatcherApp::onToggleWatching()
{
    // Disable button during operation to prevent double-clicks
    m_watchToggleButton->setEnabled(false);
    
    if (m_isWatching) {
        onStopWatching();
    } else {
        onStartWatching();
    }
    
    // Re-enable button after operation completes
    m_watchToggleButton->setEnabled(true);
}

void FileWatcherApp::onViewLogs()
{
    m_logDialog->show();
}

void FileWatcherApp::startWatching()
{
    if (m_isWatching) {
        return;
    }

    if (m_systemPanels.size() != m_systemConfigs.size()) {
        rebuildSystemPanels();
    }

    bool startedAny = false;

    for (int i = 0; i < m_systemConfigs.size(); ++i) {
        if (i >= m_systemPanels.size()) {
            break;
        }

        const auto& config = m_systemConfigs.at(i);
        if (config.source.isEmpty()) {
            continue;
        }

        auto& panel = m_systemPanels[i];
        if (!panel.table) {
            continue;
        }
        panel.table->clearTable();

        QStringList excludedFolders = ruleListForSystem(m_withoutRules, i);
        QStringList excludedFiles = ruleListForSystem(m_exceptRules, i);

        captureBaselineForSystem(i, config, excludedFolders, excludedFiles);

        WatcherThread* watcher = new WatcherThread(i, config.source, excludedFolders, excludedFiles);
        panel.watcher = watcher;

        // Use Qt::QueuedConnection for all cross-thread signals
        connect(watcher, &WatcherThread::startedWatching, this, [this, i]() {
            m_logDialog->addLog(QString("%1 watcher started").arg(getSystemName(i)));
        }, Qt::QueuedConnection);
        connect(watcher, &WatcherThread::stoppedWatching, this, [this, i]() {
            m_logDialog->addLog(QString("%1 watcher stopped").arg(getSystemName(i)));
        }, Qt::QueuedConnection);
        connect(watcher, &WatcherThread::preloadComplete, this, [this, i]() {
            m_logDialog->addLog(QString("%1 preload complete").arg(getSystemName(i)));
        }, Qt::QueuedConnection);
        connect(watcher, &WatcherThread::fileChanged, this, [this, i](const QString& path) {
            handleFileChanged(i, path);
        }, Qt::QueuedConnection);
        connect(watcher, &WatcherThread::fileCreated, this, [this, i](const QString& path) {
            handleFileCreated(i, path);
        }, Qt::QueuedConnection);
        connect(watcher, &WatcherThread::fileDeleted, this, [this, i](const QString& path) {
            handleFileDeleted(i, path);
        }, Qt::QueuedConnection);
        connect(watcher, &WatcherThread::logMessage, this, [this, i](const QString& msg) {
            m_logDialog->addLog(QString("%1: %2").arg(getSystemName(i)).arg(msg));
        }, Qt::QueuedConnection);

        watcher->start();
        startedAny = true;
    }

    if (startedAny) {
        m_isWatching = true;
        m_watchToggleButton->setText("Stop Watching");
        m_statusLabel->setText("Status: Watching...");
        m_logDialog->addLog("File watching started successfully");
    } else {
        m_watchToggleButton->setText("Start Watching");
        m_statusLabel->setText("Status: Idle");
        QMessageBox::warning(this, "Watch Error", "No valid source directories to watch.");
    }
}

void FileWatcherApp::stopWatching()
{
    if (!m_isWatching) {
        return;
    }

    m_logDialog->addLog("Stopping file watchers...");
    stopAllWatchers();
    m_isWatching = false;
    m_watchToggleButton->setText("Start Watching");
    m_statusLabel->setText("Status: Idle");
    m_logDialog->addLog("File watching stopped");
}

void FileWatcherApp::closeEvent(QCloseEvent* event)
{
    if (m_isWatching) {
        int result = QMessageBox::question(this, "Stop Watcher?",
            "File watcher is still running. Stop it before closing?",
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            stopWatching();
        }
    }
    saveSettings();
    event->accept();
}
