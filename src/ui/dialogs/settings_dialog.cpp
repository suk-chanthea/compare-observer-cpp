#include "settings_dialog.h"
#include "ui/styles.h"
#include "config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QSpacerItem>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QMessageBox>

namespace {
constexpr int kDefaultSystems = 3;
}

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_usernameEdit(new QLineEdit())
    , m_tokenEdit(new QLineEdit())
    , m_chatIdEdit(new QLineEdit())
    , m_notificationsCheckbox(new QCheckBox("Enable Telegram Notifications"))
    , m_systemsContainer(new QWidget())
    , m_systemsLayout(new QVBoxLayout())
    , m_withoutTable(new QTableWidget(0, 0))
    , m_exceptTable(new QTableWidget(0, 0))
    , m_saveButton(new QPushButton("Save"))
    , m_cancelButton(new QPushButton("Cancel"))
{
    setWindowTitle("Application Settings");
    setMinimumSize(1000, 720);
    setStyleSheet(Styles::getDialogStylesheet());

    m_tokenEdit->setEchoMode(QLineEdit::Password);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollWidget);
    contentLayout->setSpacing(18);

    // User and Telegram section
    QGroupBox* userGroup = new QGroupBox("User & Telegram");
    QGridLayout* userLayout = new QGridLayout(userGroup);
    userLayout->addWidget(new QLabel("Username:"), 0, 0);
    userLayout->addWidget(m_usernameEdit, 0, 1, 1, 3);

    userLayout->addWidget(new QLabel("Telegram Token:"), 1, 0);
    userLayout->addWidget(m_tokenEdit, 1, 1);
    userLayout->addWidget(new QLabel("Telegram Group ID:"), 1, 2);
    userLayout->addWidget(m_chatIdEdit, 1, 3);
    userLayout->addWidget(m_notificationsCheckbox, 2, 0, 1, 4);
    contentLayout->addWidget(userGroup);

    // Systems configuration section
    QGroupBox* systemsGroup = new QGroupBox("Systems Configuration");
    QVBoxLayout* systemsGroupLayout = new QVBoxLayout(systemsGroup);
    QHBoxLayout* systemButtons = new QHBoxLayout();
    QPushButton* addSystemBtn = new QPushButton("Add System");
    QPushButton* removeSystemBtn = new QPushButton("Remove Last System");
    systemButtons->addWidget(addSystemBtn);
    systemButtons->addWidget(removeSystemBtn);
    systemButtons->addStretch();
    systemsGroupLayout->addLayout(systemButtons);

    m_systemsLayout->setSpacing(12);
    m_systemsContainer->setLayout(m_systemsLayout);
    systemsGroupLayout->addWidget(m_systemsContainer);
    contentLayout->addWidget(systemsGroup);

    // Without section
    QGroupBox* withoutGroup = new QGroupBox("Without");
    QVBoxLayout* withoutLayout = new QVBoxLayout(withoutGroup);
    m_withoutTable->setMinimumHeight(240);
    withoutLayout->addWidget(m_withoutTable);
    QHBoxLayout* withoutButtons = new QHBoxLayout();
    QPushButton* withoutDefault = new QPushButton("Default");
    QPushButton* withoutAdd = new QPushButton("Add");
    QPushButton* withoutDelete = new QPushButton("Delete");
    withoutButtons->addWidget(withoutDefault);
    withoutButtons->addWidget(withoutAdd);
    withoutButtons->addWidget(withoutDelete);
    withoutButtons->addStretch();
    withoutLayout->addLayout(withoutButtons);
    contentLayout->addWidget(withoutGroup);

    // Except section
    QGroupBox* exceptGroup = new QGroupBox("Except");
    QVBoxLayout* exceptLayout = new QVBoxLayout(exceptGroup);
    m_exceptTable->setMinimumHeight(240);
    exceptLayout->addWidget(m_exceptTable);
    QHBoxLayout* exceptButtons = new QHBoxLayout();
    QPushButton* exceptDefault = new QPushButton("Default");
    QPushButton* exceptAdd = new QPushButton("Add");
    QPushButton* exceptDelete = new QPushButton("Delete");
    exceptButtons->addWidget(exceptDefault);
    exceptButtons->addWidget(exceptAdd);
    exceptButtons->addWidget(exceptDelete);
    exceptButtons->addStretch();
    exceptLayout->addLayout(exceptButtons);
    contentLayout->addWidget(exceptGroup);
    contentLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_saveButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(addSystemBtn, &QPushButton::clicked, this, &SettingsDialog::addSystem);
    connect(removeSystemBtn, &QPushButton::clicked, this, &SettingsDialog::removeSystem);
    connect(withoutDefault, &QPushButton::clicked, this, [this]() {
        // Try to load from API first, fallback to hardcoded defaults if API fails
        if (!loadRemoteRuleDefaults()) {
            loadWithoutDefaults();
        }
    });
    connect(withoutAdd, &QPushButton::clicked, this, &SettingsDialog::addWithoutRow);
    connect(withoutDelete, &QPushButton::clicked, this, &SettingsDialog::deleteWithoutRow);
    connect(exceptDefault, &QPushButton::clicked, this, [this]() {
        // Try to load from API first, fallback to hardcoded defaults if API fails
        if (!loadRemoteRuleDefaults()) {
            loadExceptDefaults();
        }
    });
    connect(exceptAdd, &QPushButton::clicked, this, &SettingsDialog::addExceptRow);
    connect(exceptDelete, &QPushButton::clicked, this, &SettingsDialog::deleteExceptRow);

    // Initialize defaults
    setSystemConfigs({});
    loadWithoutDefaults();
    loadExceptDefaults();
}

QString SettingsDialog::getUsername() const
{
    return m_usernameEdit->text();
}

void SettingsDialog::setUsername(const QString& username)
{
    m_usernameEdit->setText(username);
}

QString SettingsDialog::getWatchPath() const
{
    if (!m_systemRows.isEmpty()) {
        return m_systemRows.first().sourceEdit->text();
    }
    return {};
}

QString SettingsDialog::getTelegramToken() const
{
    return m_tokenEdit->text();
}

void SettingsDialog::setTelegramToken(const QString& token)
{
    m_tokenEdit->setText(token);
}

QString SettingsDialog::getTelegramChatId() const
{
    return m_chatIdEdit->text();
}

void SettingsDialog::setTelegramChatId(const QString& id)
{
    m_chatIdEdit->setText(id);
}

bool SettingsDialog::isNotificationsEnabled() const
{
    return m_notificationsCheckbox->isChecked();
}

void SettingsDialog::setNotificationsEnabled(bool enabled)
{
    m_notificationsCheckbox->setChecked(enabled);
}

QVector<SettingsDialog::SystemConfigData> SettingsDialog::systemConfigs() const
{
    QVector<SystemConfigData> configs;
    configs.reserve(m_systemRows.size());
    for (const auto& row : m_systemRows) {
        SystemConfigData data;
        data.name = row.nameEdit->text();
        data.source = row.sourceEdit->text();
        data.destination = row.destinationEdit->text();
        data.git = row.gitEdit->text();
        data.backup = row.backupEdit->text();
        data.assign = row.assignEdit->text();
        configs.append(data);
    }
    return configs;
}

void SettingsDialog::setSystemConfigs(const QVector<SystemConfigData>& configs)
{
    clearSystemRows();
    int count = configs.isEmpty() ? kDefaultSystems : configs.size();
    for (int i = 0; i < count; ++i) {
        SystemConfigData data;
        if (i < configs.size()) {
            data = configs.at(i);
        }
        createSystemRowWidget(i + 1, data);
    }
    refreshSystemTitles();
    updateTableColumns();
    ensureTableItems(m_withoutTable);
    ensureTableItems(m_exceptTable);
}

QVector<QStringList> SettingsDialog::withoutData() const
{
    return collectTableData(m_withoutTable);
}

void SettingsDialog::setWithoutData(const QVector<QStringList>& rows)
{
    applyTableData(m_withoutTable, rows);
}

QVector<QStringList> SettingsDialog::exceptData() const
{
    return collectTableData(m_exceptTable);
}

void SettingsDialog::setExceptData(const QVector<QStringList>& rows)
{
    applyTableData(m_exceptTable, rows);
}

int SettingsDialog::systemCount() const
{
    return m_systemRows.size();
}

void SettingsDialog::addSystem()
{
    bool ok;
    QString systemName = QInputDialog::getText(this, 
                                               "Add System",
                                               "Enter system name:",
                                               QLineEdit::Normal,
                                               QString("System %1").arg(m_systemRows.size() + 1),
                                               &ok);
    
    if (ok && !systemName.isEmpty()) {
        SystemConfigData data;
        data.name = systemName;
        createSystemRowWidget(m_systemRows.size() + 1, data);
        refreshSystemTitles();
        updateTableColumns();
        ensureTableItems(m_withoutTable);
        ensureTableItems(m_exceptTable);
    }
}

void SettingsDialog::removeSystem()
{
    if (m_systemRows.size() <= 1) {
        return;
    }

    SystemRowWidgets row = m_systemRows.takeLast();
    delete row.groupBox;
    refreshSystemTitles();
    updateTableColumns();
    ensureTableItems(m_withoutTable);
    ensureTableItems(m_exceptTable);
}

void SettingsDialog::addWithoutRow()
{
    int row = m_withoutTable->rowCount();
    m_withoutTable->insertRow(row);
    ensureTableItems(m_withoutTable);
}

void SettingsDialog::deleteWithoutRow()
{
    if (m_withoutTable->rowCount() > 0) {
        m_withoutTable->removeRow(m_withoutTable->rowCount() - 1);
    }
}

void SettingsDialog::addExceptRow()
{
    int row = m_exceptTable->rowCount();
    m_exceptTable->insertRow(row);
    ensureTableItems(m_exceptTable);
}

void SettingsDialog::deleteExceptRow()
{
    if (m_exceptTable->rowCount() > 0) {
        m_exceptTable->removeRow(m_exceptTable->rowCount() - 1);
    }
}

void SettingsDialog::loadWithoutDefaults()
{
    const QStringList defaults = {
        "config",
        "config/include",
        "config/include/lang",
        "config/include/title"
    };
    applyTableData(m_withoutTable, QVector<QStringList>(defaults.size(), QStringList()));
    for (int row = 0; row < defaults.size(); ++row) {
        for (int col = 0; col < systemCount(); ++col) {
            m_withoutTable->item(row, col)->setText(defaults[row]);
        }
    }
}

void SettingsDialog::loadExceptDefaults()
{
    const QStringList defaults = {
        "content",
        ".git",
        ".idea",
        "index.php"
    };
    applyTableData(m_exceptTable, QVector<QStringList>(defaults.size(), QStringList()));
    for (int row = 0; row < defaults.size(); ++row) {
        for (int col = 0; col < systemCount(); ++col) {
            m_exceptTable->item(row, col)->setText(defaults[row]);
        }
    }
}

void SettingsDialog::createSystemRowWidget(int index, const SystemConfigData& data)
{
    SystemRowWidgets row;
    
    QString groupTitle = data.name.isEmpty() ? QString("System %1").arg(index) : data.name;
    row.groupBox = new QGroupBox(groupTitle, this); // ✅ Added parent
    QGridLayout* grid = new QGridLayout(row.groupBox);

    row.nameEdit = new QLineEdit(data.name.isEmpty() ? QString("System %1").arg(index) : data.name, row.groupBox);
    row.sourceEdit = new QLineEdit(data.source, row.groupBox);
    row.destinationEdit = new QLineEdit(data.destination, row.groupBox);
    row.gitEdit = new QLineEdit(data.git, row.groupBox);
    row.backupEdit = new QLineEdit(data.backup, row.groupBox);
    row.assignEdit = new QLineEdit(data.assign, row.groupBox);

    // Connect name edit to update group box title and table headers
    connect(row.nameEdit, &QLineEdit::textChanged, this, [this, groupBox = row.groupBox](const QString& text) {
        groupBox->setTitle(text.isEmpty() ? "Unnamed System" : text);
        updateTableColumns();
    });

    // Add labels with parent
    grid->addWidget(new QLabel("Name:", row.groupBox), 0, 0);
    grid->addWidget(row.nameEdit, 0, 1);
    grid->addWidget(new QLabel("Source:", row.groupBox), 0, 2);
    grid->addWidget(row.sourceEdit, 0, 3);

    grid->addWidget(new QLabel("Destination:", row.groupBox), 1, 0);
    grid->addWidget(row.destinationEdit, 1, 1);
    grid->addWidget(new QLabel("Git:", row.groupBox), 1, 2);
    grid->addWidget(row.gitEdit, 1, 3);

    grid->addWidget(new QLabel("Backup:", row.groupBox), 2, 0);
    grid->addWidget(row.backupEdit, 2, 1);
    grid->addWidget(new QLabel("Assign:", row.groupBox), 2, 2);
    grid->addWidget(row.assignEdit, 2, 3);

    m_systemRows.append(row);
    m_systemsLayout->addWidget(row.groupBox);
}

void SettingsDialog::refreshSystemTitles()
{
    for (int i = 0; i < m_systemRows.size(); ++i) {
        QString name = m_systemRows[i].nameEdit->text();
        m_systemRows[i].groupBox->setTitle(name.isEmpty() ? QString("System %1").arg(i + 1) : name);
    }
}

void SettingsDialog::updateTableColumns()
{
    QStringList headers;
    for (int i = 0; i < systemCount(); ++i) {
        // Use the actual system name if available, otherwise default to "Sys X"
        QString name = m_systemRows[i].nameEdit->text();
        headers << (name.isEmpty() ? QString("Sys%1").arg(i + 1) : name);
    }
    
    // Configure Without table with equal column widths
    m_withoutTable->setColumnCount(systemCount());
    m_withoutTable->setHorizontalHeaderLabels(headers);
    m_withoutTable->horizontalHeader()->setStretchLastSection(false);
    m_withoutTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Configure Except table with equal column widths
    m_exceptTable->setColumnCount(systemCount());
    m_exceptTable->setHorizontalHeaderLabels(headers);
    m_exceptTable->horizontalHeader()->setStretchLastSection(false);
    m_exceptTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void SettingsDialog::ensureTableItems(QTableWidget* table)
{
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            if (!table->item(row, col)) {
                QTableWidgetItem* item = new QTableWidgetItem();
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                table->setItem(row, col, item);
            }
        }
    }
}

QVector<QStringList> SettingsDialog::collectTableData(QTableWidget* table) const
{
    QVector<QStringList> rows;
    rows.reserve(table->rowCount());
    
    qDebug() << "collectTableData: Table has" << table->rowCount() << "rows and" << table->columnCount() << "columns";
    
    for (int row = 0; row < table->rowCount(); ++row) {
        QStringList entries;
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            QString text = item ? item->text() : QString();
            entries << text;
        }
        qDebug() << "  Row" << row << ":" << entries;
        rows.append(entries);
    }
    return rows;
}

void SettingsDialog::applyTableData(QTableWidget* table, const QVector<QStringList>& rows)
{
    // Update column count first to match system count
    updateTableColumns();
    
    // Set row count
    table->setRowCount(rows.size());
    
    // Ensure ALL cells exist and are editable BEFORE setting data
    for (int row = 0; row < rows.size(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                table->setItem(row, col, item);
            }
        }
    }
    
    // Now fill in the data - safely
    for (int row = 0; row < rows.size(); ++row) {
        const QStringList& entries = rows[row];
        
        // Determine the default value to use
        QString defaultValue = entries.isEmpty() ? QString() : entries[0];
        
        // Apply to EVERY column
        for (int col = 0; col < table->columnCount(); ++col) {
            QString value;
            if (col < entries.size() && !entries[col].isEmpty()) {
                value = entries[col];
            } else {
                value = defaultValue;
            }
            
            // Safe access - item guaranteed to exist now
            table->item(row, col)->setText(value);
        }
    }
}

void SettingsDialog::clearSystemRows()
{
    for (auto& row : m_systemRows) {
        delete row.groupBox;
    }
    m_systemRows.clear();
}

bool SettingsDialog::loadRemoteRuleDefaults()
{
    QString apiUrl = AppConfig::instance().apiUrl();
    QNetworkRequest request(QUrl(apiUrl + "log_sys.php"));
    QNetworkReply* reply = m_networkManager.get(request);

    // Use async connection instead of blocking event loop
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray payload = reply->readAll();
        QNetworkReply::NetworkError error = reply->error();
        reply->deleteLater();

        if (error != QNetworkReply::NoError) {
            emit remoteRulesLoadFailed("Network error: " + reply->errorString());
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit remoteRulesLoadFailed("JSON parse error: " + parseError.errorString());
            return;
        }

        QJsonObject root = doc.object();
        bool updated = false;

        if (root.contains("without") && root["without"].isArray()) {
            m_withoutTable->setRowCount(0);
            m_withoutTable->clearContents();
            
            QVector<QStringList> rows = rulesFromJson(root["without"].toArray());
            if (!rows.isEmpty()) {
                setWithoutData(rows);
                updated = true;
                
                #ifndef QT_NO_DEBUG
                qDebug() << "Loaded" << rows.size() << "'Without' rules from API";
                #endif
            }
        }

        if (root.contains("except") && root["except"].isArray()) {
            m_exceptTable->setRowCount(0);
            m_exceptTable->clearContents();
            
            QVector<QStringList> rows = rulesFromJson(root["except"].toArray());
            if (!rows.isEmpty()) {
                setExceptData(rows);
                updated = true;
                
                #ifndef QT_NO_DEBUG
                qDebug() << "Loaded" << rows.size() << "'Except' rules from API";
                #endif
            }
        }
        
        if (!updated) {
            emit remoteRulesLoadFailed("No valid rules in API response");
        } else {
            emit remoteRulesLoaded();
        }
    });

    // Return immediately - processing happens asynchronously
    return true;
}

QVector<QStringList> SettingsDialog::rulesFromJson(const QJsonArray& array) const
{
    QVector<QStringList> rows;
    int cols = qMax(1, systemCount());

    for (const QJsonValue& value : array) {
        if (!value.isString()) {
            continue;
        }
        QString entry = value.toString();
        QStringList row;
        // Apply the same default rule to all systems
        for (int i = 0; i < cols; ++i) {
            row << entry;
        }
        rows.append(row);
    }
    return rows;
}
