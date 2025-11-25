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
        data.source = settings.value("source").toString();
        data.destination = settings.value("destination").toString();
        data.git = settings.value("git").toString();
        data.backup = settings.value("backup").toString();
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
        settings.setValue("source", m_systemConfigs[i].source);
        settings.setValue("destination", m_systemConfigs[i].destination);
        settings.setValue("git", m_systemConfigs[i].git);
        settings.setValue("backup", m_systemConfigs[i].backup);
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

    QLabel* descriptionLabel = new QLabel(QString("Description for sys%1:").arg(index + 1));
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

    panel.gitCompareButton = new QPushButton("Git Compare");
    panel.gitCompareButton->setStyleSheet("background-color: #F39C12; color: #1E1E1E; padding: 8px 16px; border-radius: 4px;");
    buttonsLayout->addWidget(panel.gitCompareButton);
    buttonsLayout->addStretch();

    tableRow->addLayout(buttonsLayout);
    panelLayout->addLayout(tableRow);

    int systemIndex = index;
    connect(panel.copyButton, &QPushButton::clicked, this, [this, systemIndex]() {
        handleCopyRequested(systemIndex);
    });
    connect(panel.copySendButton, &QPushButton::clicked, this, [this, systemIndex]() {
        handleCopySendRequested(systemIndex);
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
    for (auto& panel : m_systemPanels) {
        if (panel.watcher) {
            panel.watcher->stop();
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
    if (panel.table) {
        const QString relative = QDir(sourceRoot).relativeFilePath(filePath);
        panel.table->updateFileEntry(relative, "Modified");
    }

    QString newContent = readFileContent(filePath);
    if (!newContent.isNull() && panel.table) {
        const QString relative = QDir(sourceRoot).relativeFilePath(filePath);
        panel.table->setFileContent(relative, newContent);
    }

    m_logDialog->addLog(QString("System %1: File modified - %2").arg(systemIndex + 1).arg(filePath));

    if (m_notificationsEnabled && m_telegramService) {
        QString description = panel.descriptionEdit ? panel.descriptionEdit->text() : QString();
        QString summary = description.isEmpty()
            ? QString("System %1 file modified").arg(systemIndex + 1)
            : description;
        QString fromUser = m_username.isEmpty() ? QString("System %1").arg(systemIndex + 1) : m_username;
        m_telegramService->sendMessage(fromUser, summary, filePath);
    }
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

    m_logDialog->addLog(QString("System %1: File created - %2").arg(systemIndex + 1).arg(filePath));
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

    m_logDialog->addLog(QString("System %1: File deleted - %2").arg(systemIndex + 1).arg(filePath));
}

void FileWatcherApp::handleCopyRequested(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    m_logDialog->addLog(QString("Copy requested for system %1").arg(systemIndex + 1));
}

void FileWatcherApp::handleCopySendRequested(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    m_logDialog->addLog(QString("Copy & Send requested for system %1").arg(systemIndex + 1));
}

void FileWatcherApp::handleGitCompareRequested(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= m_systemPanels.size()) {
        return;
    }
    m_logDialog->addLog(QString("Git compare requested for system %1").arg(systemIndex + 1));
    showGitPage();
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
        m_logDialog->addLog(QString("System %1: Captured baseline for %2 files").arg(systemIndex + 1).arg(fileCount));
    }
}

bool FileWatcherApp::isPathExcluded(const QString& absolutePath,
                                    const QStringList& excludedFolders,
                                    const QStringList& excludedFiles) const
{
    const QString normalized = QDir::toNativeSeparators(absolutePath);

    for (const QString& folder : excludedFolders) {
        const QString trimmed = folder.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (normalized.contains(trimmed, Qt::CaseInsensitive)) {
            return true;
        }
    }

    for (const QString& filePattern : excludedFiles) {
        const QString trimmed = filePattern.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (normalized.endsWith(trimmed, Qt::CaseInsensitive)) {
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
        } else {
            m_telegramService.reset();
        }
    }
}

void FileWatcherApp::onToggleWatching()
{
    if (m_isWatching) {
        onStopWatching();
    } else {
        onStartWatching();
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

        connect(watcher, &WatcherThread::startedWatching, this, [this, i]() {
            m_logDialog->addLog(QString("System %1 watcher started").arg(i + 1));
        });
        connect(watcher, &WatcherThread::stoppedWatching, this, [this, i]() {
            m_logDialog->addLog(QString("System %1 watcher stopped").arg(i + 1));
        });
        connect(watcher, &WatcherThread::preloadComplete, this, [this, i]() {
            m_logDialog->addLog(QString("System %1 preload complete").arg(i + 1));
        });
        connect(watcher, &WatcherThread::fileChanged, this, [this, i](const QString& path) {
            handleFileChanged(i, path);
        });
        connect(watcher, &WatcherThread::fileCreated, this, [this, i](const QString& path) {
            handleFileCreated(i, path);
        });
        connect(watcher, &WatcherThread::fileDeleted, this, [this, i](const QString& path) {
            handleFileDeleted(i, path);
        });
        connect(watcher, &WatcherThread::logMessage, this, [this, i](const QString& msg) {
            m_logDialog->addLog(QString("Sys%1: %2").arg(i + 1).arg(msg));
        });

        watcher->start();
        startedAny = true;
    }

    if (startedAny) {
        m_isWatching = true;
        m_watchToggleButton->setEnabled(true);
        m_watchToggleButton->setText("Stop Watching");
        m_statusLabel->setText("Status: Watching...");
    } else {
        m_watchToggleButton->setText("Start Watching");
        QMessageBox::warning(this, "Watch Error", "No valid source directories to watch.");
    }
}

void FileWatcherApp::stopWatching()
{
    if (!m_isWatching) {
        return;
    }

    stopAllWatchers();
    m_isWatching = false;
    m_watchToggleButton->setEnabled(true);
    m_watchToggleButton->setText("Start Watching");
    m_statusLabel->setText("Status: Idle");
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
