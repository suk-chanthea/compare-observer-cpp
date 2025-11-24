#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QVector>
#include <QStringList>
#include <QNetworkAccessManager>

/**
 * @brief Dialog for application settings
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    struct SystemConfigData {
        QString source;
        QString destination;
        QString git;
        QString backup;
    };

    explicit SettingsDialog(QWidget* parent = nullptr);

    QString getUsername() const;
    void setUsername(const QString& username);

    /**
     * @brief Gets the watch path setting
     */
    QString getWatchPath() const;

    /**
     * @brief Gets the Telegram token setting
     */
    QString getTelegramToken() const;
    void setTelegramToken(const QString& token);

    /**
     * @brief Gets the Telegram chat ID setting
     */
    QString getTelegramChatId() const;
    void setTelegramChatId(const QString& id);

    /**
     * @brief Checks if notifications are enabled
     */
    bool isNotificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    QVector<SystemConfigData> systemConfigs() const;
    void setSystemConfigs(const QVector<SystemConfigData>& configs);

    QVector<QStringList> withoutData() const;
    void setWithoutData(const QVector<QStringList>& rows);

    QVector<QStringList> exceptData() const;
    void setExceptData(const QVector<QStringList>& rows);

    int systemCount() const;

    void loadWithoutDefaults();
    void loadExceptDefaults();
    bool loadRemoteRuleDefaults();

private slots:
    void addSystem();
    void removeSystem();
    void addWithoutRow();
    void deleteWithoutRow();
    void addExceptRow();
    void deleteExceptRow();

private:
    struct SystemRowWidgets {
        QGroupBox* groupBox;
        QLineEdit* sourceEdit;
        QLineEdit* destinationEdit;
        QLineEdit* gitEdit;
        QLineEdit* backupEdit;
    };

    void createSystemRowWidget(int index, const SystemConfigData& data);
    void refreshSystemTitles();
    void updateTableColumns();
    void ensureTableItems(QTableWidget* table);
    QVector<QStringList> collectTableData(QTableWidget* table) const;
    void applyTableData(QTableWidget* table, const QVector<QStringList>& rows);
    void clearSystemRows();
    QVector<QStringList> rulesFromJson(const QJsonArray& array) const;

    QLineEdit* m_usernameEdit;
    QLineEdit* m_tokenEdit;
    QLineEdit* m_chatIdEdit;
    QCheckBox* m_notificationsCheckbox;

    QWidget* m_systemsContainer;
    QVBoxLayout* m_systemsLayout;
    QVector<SystemRowWidgets> m_systemRows;

    QTableWidget* m_withoutTable;
    QTableWidget* m_exceptTable;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;

    QNetworkAccessManager m_networkManager;
};

#endif // SETTINGS_DIALOG_H
