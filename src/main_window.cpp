#include "main_window.h"
#include "services/file_watcher.h"
#include "services/telegram_service.h"
#include "ui/dialogs/log_dialog.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/dialogs/file_diff_dialog.h"
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
#include <QWidgetAction>
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
#include <QCheckBox>
#include <QProgressDialog>
#include <QCoreApplication>
#include <QTimer>

FileWatcherApp::FileWatcherApp(QWidget* parent)
    : QMainWindow(parent),
      m_watchToggleButton(new QPushButton("Start Watching")),
      m_logsButton(new QPushButton("View Logs")),
      m_statusLabel(new QLabel("Status: Idle")),
      m_panelContainer(nullptr),
      m_panelLayout(nullptr),
      m_bodyStack(nullptr),
      m_watcherPage(nullptr),
      m_systemSelectionWidget(nullptr),
      m_logDialog(std::make_unique<LogDialog>(this)),
      m_settingsDialog(std::make_unique<SettingsDialog>(this)),
      m_diffDialog(std::make_unique<FileDiffDialog>(this)),
      m_changeReviewDialog(std::make_unique<ChangeReviewDialog>(this)),
      m_notificationsEnabled(false),
      m_isWatching(false)
{
    setWindowTitle("Compare Observer");
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

    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->addStretch();
    m_watchToggleButton->setCheckable(false);
    controlLayout->addWidget(m_watchToggleButton);
    controlLayout->addWidget(m_logsButton);
    mainLayout->addLayout(controlLayout);

    // Combined container for Select Systems and Status
    QWidget* infoContainer = new QWidget(this);
    infoContainer->setStyleSheet(
        "QWidget {"
        "    background: transparent;"
        "    border: none;"
        "}"
    );
    infoContainer->setObjectName("infoContainer");
    
    QHBoxLayout* infoMainLayout = new QHBoxLayout(infoContainer);
    infoMainLayout->setContentsMargins(0, 0, 0, 0);
    infoMainLayout->setSpacing(30);
    
    // Select Systems section
    m_systemSelectionWidget = new QWidget(infoContainer);
    m_systemSelectionWidget->setStyleSheet(
        "QWidget {"
        "    background: transparent;"
        "    border: none;"
        "    padding: 0px;"
        "}"
    );
    
    QHBoxLayout* selectionLayout = new QHBoxLayout(m_systemSelectionWidget);
    selectionLayout->setContentsMargins(0, 0, 0, 0);
    selectionLayout->setSpacing(12);
    selectionLayout->setAlignment(Qt::AlignLeft);
    
    QLabel* selectLabel = new QLabel("Select Systems:");
    selectLabel->setStyleSheet(
        "color: #B5B5B5;"
        "font-size: 13px;"
        "font-weight: 600;"
        "background: transparent;"
        "border: none;"
        "padding: 0px;"
    );
    selectionLayout->addWidget(selectLabel);
    
    infoMainLayout->addWidget(m_systemSelectionWidget);
    
    // Add stretch to push status to the right
    infoMainLayout->addStretch();
    
    // Status section on the right (dynamically populated)
    QWidget* statusWidget = new QWidget(infoContainer);
    statusWidget->setStyleSheet(
        "QWidget {"
        "    background: transparent;"
        "    border: none;"
        "    padding: 0px;"
        "}"
    );
    statusWidget->setObjectName("statusWidget");
    
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(0);
    
    // Status label placeholder (will be replaced dynamically)
    m_statusLabel = new QLabel("Idle", statusWidget);
    m_statusLabel->setStyleSheet(
        "color: #888888;"
        "font-size: 13px;"
        "background: transparent;"
        "border: none;"
        "padding: 0px;"
    );
    statusLayout->addWidget(m_statusLabel);
    
    infoMainLayout->addWidget(statusWidget);
    
    // Add info container with full width
    mainLayout->addWidget(infoContainer);

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
    m_bodyStack->setCurrentWidget(m_watcherPage);
    
    mainLayout->addWidget(m_bodyStack);
}

void FileWatcherApp::createMenuBar()
{
    // Add File Monitoring menu on the left
    QMenu* fileMonitoringMenu = menuBar()->addMenu("File Monitoring");
    fileMonitoringMenu->setStyleSheet(
        "QMenu {"
        "    background-color: #1E1E1E;"
        "    color: #E0E0E0;"
        "    border: 1px solid #3A3A3A;"
        "}"
        "QMenu::item {"
        "    padding: 6px 24px;"
        "}"
        "QMenu::item:selected {"
        "    background-color: transparent;"
        "}"
    );
    
    // Add Help menu next to File Monitoring
    QMenu* helpMenu = menuBar()->addMenu("Help");
    helpMenu->setStyleSheet(
        "QMenu {"
        "    background-color: #1E1E1E;"
        "    color: #E0E0E0;"
        "    border: 1px solid #1C1C1C;"
        "}"
        "QMenu::item {"
        "    padding: 6px 24px;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #2C2C2C;"
        "}"
    );
    
    QAction* openSettingsAction = helpMenu->addAction("Settings");
    QAction* aboutAction = helpMenu->addAction("About Us");

    connect(openSettingsAction, &QAction::triggered, this, &FileWatcherApp::onSettingsClicked);
    
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this,
            "About Compare Observer",
            "<h2>Compare Observer v1.0</h2>"
            "<p><b>Compare Observer</b> is a lightweight file monitoring application designed to "
            "track changes in selected directories and files in real time.</p>"
        
            "<p>The application automatically detects file creation, modification, and deletion, "
            "allowing users to stay informed of important changes without manual checking.</p>"
        
            "<p>Integrated with <b>Telegram notifications</b>, Compare Observer instantly sends alerts "
            "to your Telegram account or group whenever a monitored change occurs, ensuring you never "
            "miss critical updates.</p>"
        
            "<p>This tool is ideal for developers, system administrators, and teams who need "
            "reliable file change monitoring with fast and secure notifications.</p>"
        );
    });
}

void FileWatcherApp::connectSignals()
{
    connect(m_watchToggleButton, &QPushButton::clicked, this, &FileWatcherApp::onToggleWatching);
    connect(m_logsButton, &QPushButton::clicked, this, &FileWatcherApp::onViewLogs);
    
    // Connect diff dialog log messages to main log dialog
    connect(m_diffDialog.get(), &FileDiffDialog::logMessage, m_logDialog.get(), &LogDialog::addLog);
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
    
    qDebug() << "Loaded without rules:" << m_withoutRules.size() << "rows";
    qDebug() << "Extracting rules per system:";
    for (int sysIdx = 0; sysIdx < m_systemConfigs.size(); ++sysIdx) {
        QStringList rulesForSystem = ruleListForSystem(m_withoutRules, sysIdx);
        qDebug() << "  System" << sysIdx << "rules:" << rulesForSystem;
    }

    m_exceptRules.clear();
    int exceptRows = settings.beginReadArray("except");
    for (int i = 0; i < exceptRows; ++i) {
        settings.setArrayIndex(i);
        QStringList entries = settings.value("entries").toStringList();
        m_exceptRules.append(entries);
    }
    settings.endArray();
    
    qDebug() << "Loaded except rules:" << m_exceptRules.size() << "rows";
    
    // Load selected systems
    m_selectedSystemIndices.clear();
    int selectedCount = settings.beginReadArray("selectedSystems");
    for (int i = 0; i < selectedCount; ++i) {
        settings.setArrayIndex(i);
        int index = settings.value("index", -1).toInt();
        if (index >= 0) {
            m_selectedSystemIndices.append(index);
        }
    }
    settings.endArray();
    
    // If no saved selection, select all systems by default
    if (m_selectedSystemIndices.isEmpty()) {
        for (int i = 0; i < m_systemConfigs.size(); ++i) {
            m_selectedSystemIndices.append(i);
        }
    }

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

    // Try loading remote defaults asynchronously (if empty)
    if (needRemoteWithout || needRemoteExcept) {
        // Connect signal to update rules when remote data arrives
        connect(m_settingsDialog.get(), &SettingsDialog::remoteRulesLoaded, 
                this, [this]() {
            m_withoutRules = m_settingsDialog->withoutData();
            m_exceptRules = m_settingsDialog->exceptData();
            qInfo() << "Remote rules loaded successfully";
        });
        
        connect(m_settingsDialog.get(), &SettingsDialog::remoteRulesLoadFailed,
                this, [this, needRemoteWithout, needRemoteExcept](const QString& error) {
            qWarning() << "Failed to load remote rules:" << error;
            // Fall back to local defaults
            if (needRemoteWithout) {
                m_settingsDialog->loadWithoutDefaults();
                m_withoutRules = m_settingsDialog->withoutData();
            }
            if (needRemoteExcept) {
                m_settingsDialog->loadExceptDefaults();
                m_exceptRules = m_settingsDialog->exceptData();
            }
        });
        
        m_settingsDialog->loadRemoteRuleDefaults();
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
    
    // Save selected systems
    QVector<int> selectedIndices = getSelectedSystemIndices();
    settings.beginWriteArray("selectedSystems");
    for (int i = 0; i < selectedIndices.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("index", selectedIndices[i]);
    }
    settings.endArray();

    qDebug() << "Saving without rules:" << m_withoutRules.size() << "rows (table structure)";
    qDebug() << "Extracting rules per system before saving:";
    for (int sysIdx = 0; sysIdx < m_systemConfigs.size(); ++sysIdx) {
        QStringList rulesForSystem = ruleListForSystem(m_withoutRules, sysIdx);
        qDebug() << "  System" << sysIdx << "rules:" << rulesForSystem;
    }
    
    settings.beginWriteArray("without");
    for (int i = 0; i < m_withoutRules.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("entries", m_withoutRules[i]);
    }
    settings.endArray();

    qDebug() << "Saving except rules:" << m_exceptRules.size() << "rows";
    
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
    
    // Update system selection checkboxes to match current systems
    updateSystemCheckboxes();
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
    panelLayout->setContentsMargins(12, 12, 12, 12);

    QHBoxLayout* descriptionRow = new QHBoxLayout();
    descriptionRow->setSpacing(8);

    QString systemName = config.name.isEmpty() ? QString("System %1").arg(index + 1) : config.name;
    QLabel* descriptionLabel = new QLabel(QString("Description for %1:").arg(systemName));
    descriptionLabel->setStyleSheet("color:#CCCCCC; font-weight: 500;");
    panel.descriptionEdit = new QLineEdit();
    panel.descriptionEdit->setPlaceholderText("Enter description here...");
    panel.descriptionEdit->setStyleSheet("padding:6px 8px;");
    panel.descriptionEdit->setClearButtonEnabled(true);

    descriptionRow->addWidget(descriptionLabel);
    descriptionRow->addWidget(panel.descriptionEdit, 1);
    panelLayout->addLayout(descriptionRow);

    QHBoxLayout* tableRow = new QHBoxLayout();
    tableRow->setSpacing(12);

    panel.table = new FileWatcherTable();
    panel.table->setMinimumHeight(200);
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

    panel.watcher = nullptr;
    m_systemPanels.append(panel);
    m_panelLayout->addWidget(panel.container);
}

QStringList FileWatcherApp::ruleListForSystem(const QVector<QStringList>& rows, int systemIndex) const
{
    QStringList values;
    for (const QStringList& row : rows) {
        QString value;
        
        // Try to get value for this system
        if (systemIndex < row.size()) {
            value = row.at(systemIndex).trimmed();
        }
        
        // If empty, fallback to first non-empty value in the row
        if (value.isEmpty() && !row.isEmpty()) {
            for (const QString& fallbackValue : row) {
                QString trimmed = fallbackValue.trimmed();
                if (!trimmed.isEmpty()) {
                    value = trimmed;
                    break;
                }
            }
        }
        
            if (!value.isEmpty()) {
                values << value;
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
    auto& panel = m_systemPanels[systemIndex];
    QStringList files = panel.table->getAllFileKeys();
    
    if (!validateCopyRequest(systemIndex, files)) {
        return;
    }
    
    // Log without rules for this system
    QStringList withoutRules = ruleListForSystem(m_withoutRules, systemIndex);
    if (withoutRules.isEmpty()) {
        m_logDialog->addLog(QString("%1: No without rules defined!").arg(getSystemName(systemIndex)));
    } else {
        m_logDialog->addLog(QString("%1: Without rules: %2")
            .arg(getSystemName(systemIndex), withoutRules.join(", ")));
    }
    
    m_logDialog->addLog(QString("%1: Starting copy of %2 file(s)...")
        .arg(getSystemName(systemIndex), QString::number(files.size())));
    
    CopyOperationResult result = copyFilesToDestinations(systemIndex, files);
    
    // Show result message (auto-closes in 3 seconds)
    if (result.successCount > 0 && result.failCount == 0) {
        // Complete success
        cleanupAfterSuccessfulCopy(systemIndex);
        showAutoCloseMessage("Copy Complete", 
            QString("✓ Successfully copied %1 file(s)\n\n%2")
            .arg(result.successCount)
            .arg(getSystemName(systemIndex)),
            QMessageBox::Information);
    } else if (result.successCount > 0 && result.failCount > 0) {
        // Partial success
        cleanupAfterSuccessfulCopy(systemIndex);
        showAutoCloseMessage("Copy Completed with Errors", 
            QString("⚠ Copied %1 file(s) successfully\n✗ %2 file(s) failed\n\n%3\n\nCheck View Logs for details.")
            .arg(result.successCount)
            .arg(result.failCount)
            .arg(getSystemName(systemIndex)),
            QMessageBox::Warning);
    } else {
        // Complete failure
        showAutoCloseMessage("Copy Failed", 
            QString("✗ Failed to copy all %1 file(s)\n\n%2\n\nCheck View Logs for details.")
            .arg(files.size())
            .arg(getSystemName(systemIndex)),
            QMessageBox::Critical);
    }
}

void FileWatcherApp::handleCopySendRequested(int systemIndex)
{
    auto& panel = m_systemPanels[systemIndex];
    QStringList files = panel.table->getAllFileKeys();
    
    if (!validateCopyRequest(systemIndex, files)) {
        return;
    }
    
    // Log without rules for this system
    QStringList withoutRules = ruleListForSystem(m_withoutRules, systemIndex);
    if (withoutRules.isEmpty()) {
        m_logDialog->addLog(QString("%1: No without rules defined!").arg(getSystemName(systemIndex)));
    } else {
        m_logDialog->addLog(QString("%1: Without rules: %2")
            .arg(getSystemName(systemIndex), withoutRules.join(", ")));
    }
    
    m_logDialog->addLog(QString("%1: Starting copy & send of %2 file(s)...")
        .arg(getSystemName(systemIndex), QString::number(files.size())));
    
    CopyOperationResult result = copyFilesToDestinations(systemIndex, files);
    
    // Show result message (auto-closes in 3 seconds)
    if (result.successCount > 0 && result.failCount == 0) {
        // Complete success - send telegram and cleanup
        QString description = panel.descriptionEdit->text().trimmed();
        if (description.isEmpty()) {
            description = getSystemName(systemIndex);
        }
        
        sendTelegramNotification(systemIndex, result.copiedFiles, description);
        cleanupAfterSuccessfulCopy(systemIndex);
        
        showAutoCloseMessage("Copy & Send Complete", 
            QString("✓ Successfully copied %1 file(s)\n✓ Telegram notification sent\n\n%2")
            .arg(result.successCount)
            .arg(getSystemName(systemIndex)),
            QMessageBox::Information);
    } else if (result.successCount > 0 && result.failCount > 0) {
        // Partial success - still send telegram for successful files
        QString description = panel.descriptionEdit->text().trimmed();
        if (description.isEmpty()) {
            description = getSystemName(systemIndex);
        }
        
        sendTelegramNotification(systemIndex, result.copiedFiles, description);
        cleanupAfterSuccessfulCopy(systemIndex);
        
        showAutoCloseMessage("Copy & Send Completed with Errors", 
            QString("⚠ Copied %1 file(s) successfully\n✗ %2 file(s) failed\n✓ Telegram notification sent\n\n%3\n\nCheck View Logs for details.")
            .arg(result.successCount)
            .arg(result.failCount)
            .arg(getSystemName(systemIndex)),
            QMessageBox::Warning);
    } else {
        // Complete failure
        showAutoCloseMessage("Copy & Send Failed", 
            QString("✗ Failed to copy all %1 file(s)\n✗ Telegram notification not sent\n\n%2\n\nCheck View Logs for details.")
            .arg(files.size())
            .arg(getSystemName(systemIndex)),
            QMessageBox::Critical);
    }
}

void FileWatcherApp::sendTelegramNotification(int systemIndex, const QStringList& files, const QString& description)
{
    if (!m_notificationsEnabled) {
        m_logDialog->addLog(QString("%1: Telegram disabled").arg(getSystemName(systemIndex)));
        return;
    }
    
    if (m_telegramToken.isEmpty() || m_telegramChatId.isEmpty()) {
        m_logDialog->addLog(QString("%1: Telegram not configured").arg(getSystemName(systemIndex)));
        return;
    }
    
    if (!m_telegramService) {
        m_logDialog->addLog(QString("%1: Telegram service not initialized").arg(getSystemName(systemIndex)));
        return;
    }
    
    QString fileList = formatFileListForTelegram(systemIndex, files);
    
    auto escapeHtml = [](const QString& text) -> QString {
        QString escaped = text;
        escaped.replace("&", "&amp;");
        escaped.replace("<", "&lt;");
        escaped.replace(">", "&gt;");
        return escaped;
    };
    
    QString title = QString("<code>%1: %2</code>")
        .arg(escapeHtml(m_username), escapeHtml(description));
    QString message = QString("%1\n\n%2").arg(title, fileList);
    
    m_logDialog->addLog(QString("%1: Sending Telegram notification...").arg(getSystemName(systemIndex)));
    m_telegramService->sendMessage(m_username, message, "");
}

QString FileWatcherApp::formatFileListForTelegram(int systemIndex, const QStringList& files)
{
    QStringList formattedFiles;
    QStringList withoutRules = ruleListForSystem(m_withoutRules, systemIndex);
    
    qDebug() << "=== formatFileListForTelegram START ===";
    qDebug() << "System index:" << systemIndex;
    qDebug() << "Files to format:" << files;
    qDebug() << "Without rules for this system:" << withoutRules;
    
    for (const QString& file : files) {
        qDebug() << "\n--- Processing file:" << file;
        bool inWithoutList = isFileInWithoutList(systemIndex, file);
        
        qDebug() << "In without list:" << inWithoutList;
        
        if (inWithoutList) {
            QString fileName = QFileInfo(file).fileName();
            formattedFiles << "- " + fileName;
            qDebug() << "  ✓ Showing as:" << fileName;
        } else {
            formattedFiles << "- " + file;
            qDebug() << "  → Showing full path:" << file;
        }
    }
    
    qDebug() << "=== formatFileListForTelegram END ===\n";
    
    return formattedFiles.join("\n");
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
    
    qDebug() << "handleAssignToRequested: System" << systemIndex << "has" << filesToAssign.size() << "files";
    m_logDialog->addLog(QString("%1: Checking files to assign...").arg(getSystemName(systemIndex)));
    
    if (filesToAssign.isEmpty()) {
        m_logDialog->addLog(QString("%1: No files in watcher list").arg(getSystemName(systemIndex)));
        qDebug() << "Assign failed: No files to assign";
        QMessageBox::warning(this, "No Files", 
                            QString("%1: No files in watcher list to assign.\n\nPlease make some file changes first and they will appear in the watcher table.")
                            .arg(getSystemName(systemIndex)));
        return;
    }
    
    // Check if assign path is configured (folder will be created automatically)
    if (config.assign.isEmpty()) {
        m_logDialog->addLog(QString("%1: Assign path not configured").arg(getSystemName(systemIndex)));
        qDebug() << "Assign failed: No assign path configured";
        QMessageBox::warning(this, "No Assign Path", 
                            QString("%1: Assign path not configured.\n\nPlease set the Assign path in Settings.")
                            .arg(getSystemName(systemIndex)));
        return;
    }
    
    m_logDialog->addLog(QString("%1: Opening assign dialog for %2 files...").arg(getSystemName(systemIndex), QString::number(filesToAssign.size())));
    
    // Create custom dialog for name and description
    QDialog dialog(this);
    dialog.setWindowTitle("Assign To");
    dialog.setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Name input
    layout->addWidget(new QLabel("Name:"));
    QLineEdit* nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText("Enter name...");
    nameEdit->setTextMargins(4, 4, 4, 4);  // Use setTextMargins for QLineEdit
    nameEdit->setStyleSheet(
        "QLineEdit {"
        "    padding: 4px;"
        "    border-radius: 6px;"
        "    border: 1px solid #343434;"
        "    background-color: #1B1B1B;"
        "    color: #F3F3F3;"
        "}"
    );
    layout->addWidget(nameEdit);
    
    layout->addSpacing(10);
    
    // Description input
    layout->addWidget(new QLabel("Description:"));
    QTextEdit* descEdit = new QTextEdit(&dialog);
    descEdit->setPlaceholderText("Enter description...");
    descEdit->setMinimumHeight(100);
    descEdit->document()->setDocumentMargin(4);  // Add internal margin
    descEdit->setStyleSheet(
        "QTextEdit {"
        "    padding: 4px;"
        "    border-radius: 6px;"
        "    border: 1px solid #343434;"
        "    background-color: #1B1B1B;"
        "    color: #F3F3F3;"
        "}"
    );
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
        m_logDialog->addLog(QString("%1: Name is required").arg(getSystemName(systemIndex)));
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
        
        // Re-capture baseline from current file state so future changes are compared correctly
        QStringList excludedFolders = ruleListForSystem(m_withoutRules, systemIndex);
        QStringList excludedFiles = ruleListForSystem(m_exceptRules, systemIndex);
        captureBaselineForSystem(systemIndex, config, excludedFolders, excludedFiles);
        m_logDialog->addLog(QString("%1: Baseline updated to current state").arg(getSystemName(systemIndex)));
    }
    
    // Show result message (auto-closes in 3 seconds)
    if (successCount > 0 && failCount == 0) {
        // Complete success
        showAutoCloseMessage("Assign Complete", 
            QString("✓ Successfully assigned %1 file(s)\n\n📁 %2\n\nFolder: %3/%4")
            .arg(successCount)
            .arg(getSystemName(systemIndex))
            .arg(folderName)
            .arg(dateTimeFolder),
            QMessageBox::Information);
    } else if (successCount > 0 && failCount > 0) {
        // Partial success
        showAutoCloseMessage("Assign Completed with Errors", 
            QString("⚠ Assigned %1 file(s) successfully\n✗ %2 file(s) failed\n\n📁 Folder: %3/%4\n\nCheck View Logs for details.")
            .arg(successCount)
            .arg(failCount)
            .arg(folderName)
            .arg(dateTimeFolder),
            QMessageBox::Warning);
    } else {
        // Complete failure
        showAutoCloseMessage("Assign Failed", 
            QString("✗ Failed to assign all %1 file(s)\n\n%2\n\nCheck View Logs for details.")
            .arg(filesToAssign.size())
            .arg(getSystemName(systemIndex)),
            QMessageBox::Critical);
    }
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

void FileWatcherApp::captureBaselineForSystem(int systemIndex,
                                              const SettingsDialog::SystemConfigData& config,
                                              const QStringList& excludedFolders,
                                              const QStringList& excludedFiles)
{
    // This is a wrapper for backward compatibility
    captureBaselineForSystemWithProgress(systemIndex, config, excludedFolders, excludedFiles, 0, 0);
}

void FileWatcherApp::captureBaselineForSystemWithProgress(int systemIndex,
                                                          const SettingsDialog::SystemConfigData& config,
                                                          const QStringList& excludedFolders,
                                                          const QStringList& excludedFiles,
                                                          int cumulativeProcessed,
                                                          int totalFilesAllSystems)
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

    // Capture baseline with cumulative progress tracking
    QDirIterator it(config.source, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int fileCount = 0;
    int processedFiles = 0;

    while (it.hasNext()) {
        QString path = it.next();
        if (isPathExcluded(path, excludedFolders, excludedFiles)) {
            continue;
        }

        processedFiles++;
        int totalProcessed = cumulativeProcessed + processedFiles;
        
        // Update progress every 10 files or on last file
        if (processedFiles % 10 == 0) {
            updateProgress(totalProcessed);
            
            // Process events to keep UI responsive
            QCoreApplication::processEvents();
            
            // Log progress every 100 files
            if (totalProcessed % 100 == 0) {
                int percentage = totalFilesAllSystems > 0 ? (totalProcessed * 100) / totalFilesAllSystems : 0;
                m_logDialog->addLog(QString("Overall progress: %1% (%2/%3) - Processing %4...")
                    .arg(percentage)
                    .arg(totalProcessed)
                    .arg(totalFilesAllSystems)
                    .arg(getSystemName(systemIndex)));
            }
        }

        QString content = readFileContent(path);
        if (!content.isNull()) {
            const QString relative = QDir(config.source).relativeFilePath(path);
            panel.table->setFileContent(relative, content);
            ++fileCount;
        }
    }
    
    // Final progress update for this system
    updateProgress(cumulativeProcessed + processedFiles);
    QCoreApplication::processEvents();

    if (fileCount > 0) {
        m_logDialog->addLog(QString("%1: ✓ Captured baseline for %2 files").arg(getSystemName(systemIndex)).arg(fileCount));
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
        
        // Check if folder appears as a path component (exact name match)
        // Match "\foldername\" anywhere in path OR "\foldername" at end
        QString sep = QDir::separator();
        QString folderInPath = sep + trimmed + sep;
        QString folderAtEnd = sep + trimmed;
        
        if (normalized.contains(folderInPath, Qt::CaseInsensitive) ||
            normalized.endsWith(folderAtEnd, Qt::CaseInsensitive)) {
            return true;
        }
    }

    // Check excluded files (from "Except" list)
    for (const QString& filePattern : excludedFiles) {
        const QString trimmed = filePattern.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        
        // Check if it's a folder pattern (doesn't have file extension)
        bool isFolder = !trimmed.contains('.') || trimmed.startsWith(".");
        
        if (isFolder) {
            // Treat as folder - check as path component
            // Match "\foldername\" anywhere in path OR "\foldername" at end
            QString sep = QDir::separator();
            QString folderInPath = sep + trimmed + sep;
            QString folderAtEnd = sep + trimmed;
            
            if (normalized.contains(folderInPath, Qt::CaseInsensitive) ||
                normalized.endsWith(folderAtEnd, Qt::CaseInsensitive)) {
                return true;
            }
        } else {
            // Treat as file pattern - check if path ends with it
        if (normalized.endsWith(trimmed, Qt::CaseInsensitive)) {
            return true;
            }
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
        m_watchToggleButton->setText("Start Watching");
        m_watchToggleButton->setEnabled(true);
        onSettingsClicked();
        return;
    }

    startWatching();
    // Button will be re-enabled in startWatching after operation completes
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
        
        qDebug() << "=== Settings Dialog Accepted ===";
        qDebug() << "Updated without rules:" << m_withoutRules.size() << "rows";
        qDebug() << "Extracting rules per system:";
        for (int sysIdx = 0; sysIdx < m_systemConfigs.size(); ++sysIdx) {
            QStringList rulesForSystem = ruleListForSystem(m_withoutRules, sysIdx);
            qDebug() << "  System" << sysIdx << "rules:" << rulesForSystem;
        }
        qDebug() << "Updated except rules:" << m_exceptRules.size() << "rows";

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
        
        // Restart watching if it was active before opening settings
        if (wasWatching) {
            startWatching();
        }
    }
}

void FileWatcherApp::onToggleWatching()
{
    if (m_isWatching) {
        // Disable button during stop operation
        m_watchToggleButton->setEnabled(false);
        m_watchToggleButton->setText("Stopping...");
        onStopWatching();
        m_watchToggleButton->setEnabled(true);
    } else {
        // Disable button and show "Starting..." during start operation
        m_watchToggleButton->setEnabled(false);
        m_watchToggleButton->setText("Starting...");
        onStartWatching();
        // Button will be re-enabled in onStartWatching after validation/operation
    }
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

    // Get selected system indices
    QVector<int> selectedIndices = getSelectedSystemIndices();
    
    if (selectedIndices.isEmpty()) {
        QMessageBox::warning(this, "No Systems Selected", 
            "Please select at least one system to watch.");
        m_watchToggleButton->setText("Start Watching");
        m_watchToggleButton->setEnabled(true);
        return;
    }

    bool startedAny = false;
    
    // First pass: Count total files across all selected systems
    m_logDialog->addLog("=== Counting files for all systems ===");
    int totalFilesAllSystems = 0;
    QVector<int> systemFileCounts;
    
    for (int i : selectedIndices) {
        if (i >= m_systemConfigs.size() || i >= m_systemPanels.size()) {
            systemFileCounts.append(0);
            continue;
        }

        const auto& config = m_systemConfigs.at(i);
        if (config.source.isEmpty()) {
            systemFileCounts.append(0);
            continue;
        }
        
        QStringList excludedFolders;
        QStringList excludedFiles = ruleListForSystem(m_exceptRules, i);
        
        // Count files for this system
        QDirIterator countIt(config.source, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        int systemFileCount = 0;
        
        while (countIt.hasNext()) {
            QString path = countIt.next();
            if (!isPathExcluded(path, excludedFolders, excludedFiles)) {
                systemFileCount++;
            }
        }
        
        systemFileCounts.append(systemFileCount);
        totalFilesAllSystems += systemFileCount;
        m_logDialog->addLog(QString("%1: %2 files found").arg(getSystemName(i)).arg(systemFileCount));
    }
    
    if (totalFilesAllSystems == 0) {
        m_logDialog->addLog("No files found in any system");
        m_watchToggleButton->setText("Start Watching");
        m_watchToggleButton->setEnabled(true);
        QMessageBox::warning(this, "No Files", "No files found in any selected system source directories.");
        return;
    }
    
    // Show single progress dialog for all systems combined
    m_logDialog->addLog(QString("=== Capturing baseline for %1 total files ===").arg(totalFilesAllSystems));
    showProgressDialog(QString("Capturing Baseline (%1 files) - Please Wait...").arg(totalFilesAllSystems), totalFilesAllSystems);
    
    int processedSoFar = 0;

    // Second pass: Capture baseline for each system
    for (int idx = 0; idx < selectedIndices.size(); ++idx) {
        int i = selectedIndices[idx];
        
        if (i >= m_systemConfigs.size() || i >= m_systemPanels.size()) {
            continue;
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

        // Only use Except table for file exclusion (Without table is only for folder path removal when copying)
        QStringList excludedFolders;  // Empty - not used for exclusion
        QStringList excludedFiles = ruleListForSystem(m_exceptRules, i);

        m_logDialog->addLog(QString("=== %1: Processing %2 files ===").arg(getSystemName(i)).arg(systemFileCounts[idx]));
        
        // Capture baseline with cumulative progress
        captureBaselineForSystemWithProgress(i, config, excludedFolders, excludedFiles, processedSoFar, totalFilesAllSystems);
        
        processedSoFar += systemFileCounts[idx];

        WatcherThread* watcher = new WatcherThread(i, getSystemName(i), config.source, excludedFolders, excludedFiles);
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
    
    // Close progress dialog after all systems are processed
    closeProgressDialog();

    if (startedAny) {
        m_isWatching = true;
        m_watchToggleButton->setText("Stop Watching");
        m_watchToggleButton->setEnabled(true);  // Re-enable after successful start
        
        // Disable checkboxes while watching
        for (QCheckBox* checkbox : m_systemCheckboxes) {
            checkbox->setEnabled(false);
        }
        
        updateStatusLabel();
        m_logDialog->addLog("✓ File watching started successfully - All systems ready");
    } else {
        m_watchToggleButton->setText("Start Watching");
        m_watchToggleButton->setEnabled(true);  // Re-enable after failed start
        updateStatusLabel();
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
    m_logDialog->addLog("File watching stopped");
    
    // Re-enable checkboxes when stopped
    for (QCheckBox* checkbox : m_systemCheckboxes) {
        checkbox->setEnabled(true);
    }
    
    // Update status display (will show "Idle" automatically)
    updateStatusLabel();
}

void FileWatcherApp::updateSystemCheckboxes()
{
    // Clear existing checkboxes
    for (QCheckBox* checkbox : m_systemCheckboxes) {
        checkbox->deleteLater();
    }
    m_systemCheckboxes.clear();
    
    if (!m_systemSelectionWidget) {
        return;
    }
    
    // Get the HBoxLayout from the selection widget
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(m_systemSelectionWidget->layout());
    if (!layout) {
        return;
    }
    
    // Create checkbox for each system with tick mark
    for (int i = 0; i < m_systemConfigs.size(); ++i) {
        QString systemName = m_systemConfigs[i].name.isEmpty() 
            ? QString("System %1").arg(i + 1) 
            : m_systemConfigs[i].name;
        
        // Create checkbox without tick mark
        QCheckBox* checkbox = new QCheckBox(systemName, m_systemSelectionWidget);
        // Check if this system was selected
        checkbox->setChecked(m_selectedSystemIndices.contains(i));
        checkbox->setStyleSheet(
            "QCheckBox {"
            "    color: #DADADA;"
            "    font-size: 13px;"
            "    padding: 4px 8px 4px 12px;"
            "    background: #252525;"
            "    border: 1px solid #3A3A3A;"
            "    border-radius: 4px;"
            "}"
            "QCheckBox:hover {"
            "    background: #2A2A2A;"
            "    border-color: #4A4A4A;"
            "}"
            "QCheckBox:checked {"
            "    color: #FFFFFF;"
            "    border-color: #5A5A5A;"
            "    background: #2A2A2A;"
            "}"
            "QCheckBox:!checked {"
            "    color: #888888;"
            "    background: #1A1A1A;"
            "    border-color: #2A2A2A;"
            "}"
            "QCheckBox:disabled {"
            "    color: #555555;"
            "    background: #1A1A1A;"
            "    border-color: #2A2A2A;"
            "}"
            "QCheckBox::indicator {"
            "    width: 0px;"
            "    height: 0px;"
            "}"
        );
        
        // Connect to handle selection change during watching
        connect(checkbox, &QCheckBox::checkStateChanged, this, &FileWatcherApp::onSystemSelectionChanged);
        
        layout->addWidget(checkbox);
        m_systemCheckboxes.append(checkbox);
    }
    
    updateStatusLabel();
}

void FileWatcherApp::updateStatusLabel()
{
    // Find the status widget
    QWidget* infoContainer = centralWidget()->findChild<QWidget*>("infoContainer");
    QWidget* statusWidget = infoContainer ? infoContainer->findChild<QWidget*>("statusWidget") : nullptr;
    
    if (!statusWidget) {
        return;
    }
    
    // Clear existing status items
    QHBoxLayout* statusLayout = qobject_cast<QHBoxLayout*>(statusWidget->layout());
    if (!statusLayout) {
        return;
    }
    
    // Remove all existing widgets from status layout
    while (QLayoutItem* item = statusLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    
    if (m_systemCheckboxes.isEmpty()) {
        QLabel* idleLabel = new QLabel("Idle", statusWidget);
        idleLabel->setStyleSheet(
            "color: #888888;"
            "font-size: 13px;"
            "background: transparent;"
            "border: none;"
            "padding: 0px;"
        );
        statusLayout->addWidget(idleLabel);
        return;
    }
    
    int watchingCount = 0;
    
    // Add each system status with separators
    for (int i = 0; i < m_systemCheckboxes.size() && i < m_systemPanels.size(); ++i) {
        if (i > 0) {
            // Add vertical separator between status items
            QFrame* separator = new QFrame(statusWidget);
            separator->setFrameShape(QFrame::VLine);
            separator->setFrameShadow(QFrame::Plain);
            separator->setStyleSheet(
                "QFrame {"
                "    background-color: #353535;"
                "    border: none;"
                "    max-width: 1px;"
                "    min-height: 16px;"
                "}"
            );
            statusLayout->addWidget(separator);
        }
        
        QString systemName = m_systemConfigs[i].name.isEmpty() 
            ? QString("Sys%1").arg(i + 1) 
            : m_systemConfigs[i].name;
        
        bool isWatching = m_isWatching && m_systemPanels[i].watcher != nullptr;
        if (isWatching) {
            watchingCount++;
        }
        QString status = isWatching ? "Watching" : "Idle";
        
        // Status indicator dot
        QLabel* statusDot = new QLabel("●", statusWidget);
        statusDot->setStyleSheet(
            QString("color: %1;"
                    "font-size: 12px;"
                    "background: transparent;"
                    "border: none;"
                    "padding: 0px 0px 0px 8px;")
            .arg(isWatching ? "#4A9EFF" : "#888888")
        );
        statusLayout->addWidget(statusDot);
        
        // Status text
        QLabel* statusLabel = new QLabel(QString("%1: %2").arg(systemName).arg(status), statusWidget);
        statusLabel->setStyleSheet(
            "color: #D5D5D5;"
            "font-size: 13px;"
            "background: transparent;"
            "border: none;"
            "padding: 0px 8px 0px 0px;"
        );
        statusLayout->addWidget(statusLabel);
    }
}

QVector<int> FileWatcherApp::getSelectedSystemIndices() const
{
    QVector<int> selectedIndices;
    for (int i = 0; i < m_systemCheckboxes.size(); ++i) {
        if (m_systemCheckboxes[i]->isChecked()) {
            selectedIndices.append(i);
        }
    }
    return selectedIndices;
}

void FileWatcherApp::onSystemSelectionChanged()
{
    // If watching is active, prevent the change
    if (m_isWatching) {
        // Block signals temporarily to prevent infinite loop
        for (QCheckBox* checkbox : m_systemCheckboxes) {
            checkbox->blockSignals(true);
        }
        
        // Find which checkbox was clicked and revert its state
        QCheckBox* sender = qobject_cast<QCheckBox*>(QObject::sender());
        if (sender) {
            // Revert the checkbox state
            sender->setChecked(!sender->isChecked());
        }
        
        // Unblock signals
        for (QCheckBox* checkbox : m_systemCheckboxes) {
            checkbox->blockSignals(false);
        }
        
        // Show warning message
        QMessageBox::warning(this, "Cannot Change Selection", 
            "Stop watching before the change of system");
    } else {
        // Update selected system indices and save settings
        m_selectedSystemIndices = getSelectedSystemIndices();
        saveSettings();
        
        // Update status label
        updateStatusLabel();
    }
}

void FileWatcherApp::closeEvent(QCloseEvent* event)
{
    if (m_isWatching) {
        int result = QMessageBox::question(this, "Stop Watcher?",
            "File watcher is still running. Stop it before closing?",
            QMessageBox::Ok | QMessageBox::Cancel);
        
        if (result == QMessageBox::Ok) {
            stopWatching();
            saveSettings();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        saveSettings();
        event->accept();
    }
}
bool FileWatcherApp::validateCopyRequest(int systemIndex, const QStringList& files)
{
    qDebug() << "validateCopyRequest: systemIndex=" << systemIndex << "files=" << files;
    
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size() || 
        systemIndex >= m_systemConfigs.size()) {
        m_logDialog->addLog("Error: Invalid system index");
        qDebug() << "Validation failed: Invalid system index";
        return false;
    }
    
    if (files.isEmpty()) {
        m_logDialog->addLog(QString("%1: No files to copy").arg(getSystemName(systemIndex)));
        qDebug() << "Validation failed: No files to copy";
        QMessageBox::warning(this, "No Files", 
            QString("%1: No files to copy.\n\nPlease make some file changes first and they will appear in the watcher table.")
            .arg(getSystemName(systemIndex)));
        return false;
    }
    
    const auto& config = m_systemConfigs[systemIndex];
    qDebug() << "Config - destination:" << config.destination << "git:" << config.git << "backup:" << config.backup;
    
    if (config.destination.isEmpty() && config.git.isEmpty() && config.backup.isEmpty()) {
        m_logDialog->addLog(QString("%1: No destination paths configured").arg(getSystemName(systemIndex)));
        qDebug() << "Validation failed: No paths configured";
        QMessageBox::warning(this, "No Paths Configured", 
            QString("%1: No destination paths configured.\n\nConfigure at least one path in Settings:\n- Destination Path\n- Git Path\n- Backup Path")
            .arg(getSystemName(systemIndex)));
        return false;
    }
    
    qDebug() << "Validation passed!";
    m_logDialog->addLog(QString("%1: Validation passed - %2 files ready").arg(getSystemName(systemIndex), QString::number(files.size())));
    return true;
}
FileWatcherApp::CopyOperationResult FileWatcherApp::copyFilesToDestinations(int systemIndex, const QStringList& files)
{
    CopyOperationResult result;
    const auto& config = m_systemConfigs[systemIndex];
    
    // Get current date/time for backup folder structure
    QDateTime now = QDateTime::currentDateTime();
    QString dateFolder = now.toString("yyyy-MM-dd");
    QString timeFolder = now.toString("HH-mm-ss");
    
    showProgressDialog("Copying Files", files.size());
    
    for (int i = 0; i < files.size(); ++i) {
        updateProgress(i);
        
        const QString& relativeFilePath = files[i];
        QString sourceFile = QDir(config.source).filePath(relativeFilePath);
        
        if (!QFile::exists(sourceFile)) {
            m_logDialog->addLog(QString("  ✗ Source not found: %1").arg(relativeFilePath));
            result.failCount++;
            continue;
        }
        
        bool fileSuccess = true;
        bool inWithoutList = isFileInWithoutList(systemIndex, relativeFilePath);
        
        // Copy to destination
        if (!config.destination.isEmpty()) {
            QString destPath;
            if (inWithoutList) {
                // Flatten path - remove folder structure
                destPath = QDir(config.destination).filePath(QFileInfo(relativeFilePath).fileName());
                m_logDialog->addLog(QString("  → %1 (without folder)").arg(QFileInfo(relativeFilePath).fileName()));
            } else {
                // Keep full relative path structure
                destPath = QDir(config.destination).filePath(relativeFilePath);
                m_logDialog->addLog(QString("  → %1").arg(relativeFilePath));
            }
            
            if (!copyFileToDestination(sourceFile, destPath)) {
                fileSuccess = false;
            }
        }
        
        // Copy to git (always keep full path structure)
        if (!config.git.isEmpty()) {
            // Always keep full relative path structure in git
            QString gitPath = QDir(config.git).filePath(relativeFilePath);
            
            // Backup old file from git first
            if (!config.backup.isEmpty() && QFile::exists(gitPath)) {
                if (!backupOldFileFromGit(gitPath, relativeFilePath, systemIndex)) {
                    fileSuccess = false;
                }
            }
            
            // Copy new file to git
            if (!copyFileToDestination(sourceFile, gitPath)) {
                fileSuccess = false;
            }
        }
        
        if (fileSuccess) {
            result.successCount++;
            // Always store original relative path - formatting happens later
            result.copiedFiles << relativeFilePath;
        } else {
            result.failCount++;
        }
    }
    
    closeProgressDialog();
    
    m_logDialog->addLog(QString("%1: Copy complete - %2 succeeded, %3 failed")
        .arg(getSystemName(systemIndex), QString::number(result.successCount), QString::number(result.failCount)));
    
    return result;
}

bool FileWatcherApp::copyFileToDestination(const QString& sourceFile, const QString& destPath)
{
    QString destDir = QFileInfo(destPath).absolutePath();
    
    if (!QDir().mkpath(destDir)) {
        m_logDialog->addLog(QString("  ✗ Failed to create directory: %1").arg(destDir));
        return false;
    }
    
    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }
    
    if (!QFile::copy(sourceFile, destPath)) {
        m_logDialog->addLog(QString("  ✗ Failed to copy to: %1").arg(destPath));
        return false;
    }
    
    return true;
}

bool FileWatcherApp::isFileInWithoutList(int systemIndex, const QString& filePath)
{
    // Extract the column for this system from the row-based table structure
    QStringList rules = ruleListForSystem(m_withoutRules, systemIndex);
    
    qDebug() << "Checking file:" << filePath << "against" << rules.size() << "without rules for system" << systemIndex;
    qDebug() << "Rules for this system:" << rules;
    
    if (rules.isEmpty()) {
        qDebug() << "  No rules defined for this system";
        return false;
    }
    
    // Normalize file path to use forward slashes for comparison
    QString normalizedFilePath = filePath;
    normalizedFilePath.replace('\\', '/');
    
    for (const QString& rule : rules) {
        QString trimmedRule = rule.trimmed();
        if (trimmedRule.isEmpty()) {
            qDebug() << "  Skipping empty rule";
            continue;
        }
        
        // Normalize rule to use forward slashes
        QString normalizedRule = trimmedRule;
        normalizedRule.replace('\\', '/');
        
        // Ensure rule ends with / for proper folder matching
        if (!normalizedRule.endsWith('/')) {
            normalizedRule += '/';
        }
        
        qDebug() << "  Checking rule:'" << normalizedRule << "'";
        qDebug() << "  Against file:'" << normalizedFilePath << "'";
        
        // Check if the file path starts with the rule (is in this folder or subfolder)
        if (normalizedFilePath.startsWith(normalizedRule, Qt::CaseInsensitive)) {
            // Get the remaining path after the rule
            QString remainingPath = normalizedFilePath.mid(normalizedRule.length());
            
            qDebug() << "  Remaining path after rule:" << remainingPath;
            
            // Count slashes in remaining path
            // If there's more than 0 slashes (excluding trailing), it's in a subfolder
            int slashCount = remainingPath.count('/');
            
            qDebug() << "  Slash count in remaining path:" << slashCount;
            
            if (slashCount == 0) {
                // File is directly in this folder (no subfolders)
                qDebug() << "  ✓✓✓ MATCH! File is DIRECTLY in folder - will be flattened ✓✓✓";
                return true;
            } else {
                // File is in a subfolder - don't flatten, show full path
                qDebug() << "  ✗ File is in SUBFOLDER (" << slashCount << " level(s) deep) - showing full path";
            }
        } else {
            qDebug() << "  ✗ No match - doesn't start with rule";
        }
    }
    
    qDebug() << "  ✗✗✗ No rules matched - keeping full path";
    return false;
}

void FileWatcherApp::showProgressDialog(const QString& title, int max)
{
    if (m_progressDialog) {
        delete m_progressDialog;
    }
    
    m_progressDialog = new QProgressDialog(title, "Cancel", 0, max, this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->setValue(0);
    m_progressDialog->show();
}

void FileWatcherApp::updateProgress(int value)
{
    if (m_progressDialog) {
        m_progressDialog->setValue(value);
    }
}

void FileWatcherApp::closeProgressDialog()
{
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
}

void FileWatcherApp::cleanupAfterSuccessfulCopy(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    
    auto& panel = m_systemPanels[systemIndex];
    if (panel.table) {
        panel.table->clearTable();
    }
    
    if (panel.descriptionEdit) {
        panel.descriptionEdit->clear();
    }
    
    m_logDialog->addLog(QString("%1: Cleared watcher table and description").arg(getSystemName(systemIndex)));
}

bool FileWatcherApp::backupOldFileFromGit(const QString& gitPath, const QString& relativePath, int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemConfigs.size()) {
        return false;
    }
    
    const auto& config = m_systemConfigs[systemIndex];
    
    if (config.backup.isEmpty()) {
        return true; // No backup path configured, consider it success
    }
    
    if (!QFile::exists(gitPath)) {
        return true; // File doesn't exist yet, nothing to backup
    }
    
    // Create backup path with date and time folders
    QDateTime now = QDateTime::currentDateTime();
    QString dateFolder = now.toString("yyyy-MM-dd");  // e.g., 2026-01-26
    QString timeFolder = now.toString("HH-mm-ss");    // e.g., 14-30-45
    
    // Build backup path: backup_path/yyyy-MM-dd/HH-mm-ss/relative/path/to/file.php
    QString backupBasePath = QDir(config.backup).filePath(dateFolder);
    backupBasePath = QDir(backupBasePath).filePath(timeFolder);
    QString backupPath = QDir(backupBasePath).filePath(relativePath);
    
    // Create full backup directory structure if needed
    QString backupDir = QFileInfo(backupPath).absolutePath();
    if (!QDir().mkpath(backupDir)) {
        m_logDialog->addLog(QString("  ✗ Failed to create backup directory: %1").arg(backupDir));
        return false;
    }
    
    // Copy old file to backup (preserving full path structure)
    if (!QFile::copy(gitPath, backupPath)) {
        m_logDialog->addLog(QString("  ✗ Failed to backup old file to: %1").arg(backupPath));
        return false;
    }
    
    m_logDialog->addLog(QString("  ✓ Backed up: %1/%2/%3").arg(dateFolder, timeFolder, relativePath));
    return true;
}

void FileWatcherApp::showAutoCloseMessage(const QString& title, const QString& message, int icon, int milliseconds)
{
    QMessageBox* msgBox = new QMessageBox(static_cast<QMessageBox::Icon>(icon), title, message, QMessageBox::NoButton, this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowModality(Qt::NonModal);
    
    // Set minimum size to make it look nice
    msgBox->setMinimumWidth(400);
    
    // Set styling (using system default background)
    msgBox->setStyleSheet(
        "QMessageBox {"
        "   border-radius: 8px;"
        "   padding: 16px;"
        "}"
        "QLabel {"
        "   font-size: 13px;"
        "}"
    );
    
    msgBox->show();
    
    // Auto-close after specified milliseconds
    QTimer::singleShot(milliseconds, msgBox, &QMessageBox::close);
}