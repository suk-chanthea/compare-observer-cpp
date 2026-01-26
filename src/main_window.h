#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QLabel>
#include <QStackedWidget>
#include <memory>

#include "ui/dialogs/settings_dialog.h"

class QTableWidget;
class QPushButton;
class QLineEdit;
class QWidget;
class QVBoxLayout;
class QCheckBox;
class QProgressDialog;
class WatcherThread;
class TelegramService;
class FileWatcherTable;
class LogDialog;
class FileDiffDialog;
class ChangeReviewDialog;

/**
 * @brief Main application window
 */
class FileWatcherApp : public QMainWindow {
    Q_OBJECT

public:
    explicit FileWatcherApp(QWidget* parent = nullptr);
    ~FileWatcherApp();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onToggleWatching();
    void onSettingsClicked();
    void onViewLogs();

private:
    void setupUI();
    void createMenuBar();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    void startWatching();
    void stopWatching();
    void onStartWatching();
    void onStopWatching();
    void rebuildSystemPanels();
    void clearSystemPanels();
    void buildSystemPanel(int index, const SettingsDialog::SystemConfigData& config);
    QStringList ruleListForSystem(const QVector<QStringList>& rows, int systemIndex) const;
    void handleFileChanged(int systemIndex, const QString& filePath);
    void handleFileCreated(int systemIndex, const QString& filePath);
    void handleFileDeleted(int systemIndex, const QString& filePath);
    void handleCopyRequested(int systemIndex);
    void handleCopySendRequested(int systemIndex);
    void handleAssignToRequested(int systemIndex);
    void handleViewDiffRequested(int systemIndex, const QString& filePath);
    void stopAllWatchers();
    void captureBaselineForSystem(int systemIndex,
                                  const SettingsDialog::SystemConfigData& config,
                                  const QStringList& excludedFolders,
                                  const QStringList& excludedFiles);
    void captureBaselineForSystemWithProgress(int systemIndex,
                                              const SettingsDialog::SystemConfigData& config,
                                              const QStringList& excludedFolders,
                                              const QStringList& excludedFiles,
                                              int cumulativeProcessed,
                                              int totalFilesAllSystems);
    bool isPathExcluded(const QString& absolutePath,
                        const QStringList& excludedFolders,
                        const QStringList& excludedFiles) const;
    QString readFileContent(const QString& filePath) const;
    QString getSystemName(int systemIndex) const;
    void updateSystemCheckboxes();
    void updateStatusLabel();
    QVector<int> getSelectedSystemIndices() const;
    void onSystemSelectionChanged();

    struct CopyOperationResult {
        int successCount = 0;
        int failCount = 0;
        QStringList copiedFiles;
    };
    
    bool validateCopyRequest(int systemIndex, const QStringList& files);
    CopyOperationResult copyFilesToDestinations(int systemIndex, const QStringList& files);
    bool copyFileToDestination(const QString& sourceFile, const QString& destPath);
    bool backupOldFileFromGit(const QString& gitPath, const QString& relativePath, int systemIndex);
    void sendTelegramNotification(int systemIndex, const QStringList& files, const QString& description);
    void cleanupAfterSuccessfulCopy(int systemIndex);
    QString formatFileListForTelegram(int systemIndex, const QStringList& files);
    bool isFileInWithoutList(int systemIndex, const QString& filePath);
    void showProgressDialog(const QString& title, int max);
    void updateProgress(int value);
    void closeProgressDialog();
    void showAutoCloseMessage(const QString& title, const QString& message, int icon, int milliseconds = 3000);
    
    QProgressDialog* m_progressDialog = nullptr;

    // UI Components
    QPushButton* m_watchToggleButton;
    QPushButton* m_logsButton;
    QLabel* m_statusLabel;
    QLabel* m_titleLabel;
    QWidget* m_panelContainer;
    QVBoxLayout* m_panelLayout;
    QStackedWidget* m_bodyStack;
    QWidget* m_watcherPage;
    QWidget* m_systemSelectionWidget;
    QVector<QCheckBox*> m_systemCheckboxes;

    // Dialogs
    std::unique_ptr<LogDialog> m_logDialog;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    std::unique_ptr<FileDiffDialog> m_diffDialog;
    std::unique_ptr<ChangeReviewDialog> m_changeReviewDialog;

    // Services
    std::unique_ptr<TelegramService> m_telegramService;

    // Settings
    struct SystemPanel {
        QWidget* container = nullptr;
        QLineEdit* descriptionEdit = nullptr;
        FileWatcherTable* table = nullptr;
        QPushButton* copyButton = nullptr;
        QPushButton* copySendButton = nullptr;
        QPushButton* assignToButton = nullptr;
        WatcherThread* watcher = nullptr;
    };

    QVector<SystemPanel> m_systemPanels;
    QString m_username;
    QString m_watchPath;
    QString m_telegramToken;
    QString m_telegramChatId;
    bool m_notificationsEnabled;
    bool m_isWatching;
    QVector<SettingsDialog::SystemConfigData> m_systemConfigs;
    QVector<QStringList> m_withoutRules;
    QVector<QStringList> m_exceptRules;
    QVector<int> m_selectedSystemIndices;
};

#endif // MAIN_WINDOW_H
