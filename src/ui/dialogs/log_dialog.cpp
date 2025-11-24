#include "log_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QHeaderView>

LogDialog::LogDialog(QWidget* parent)
    : QDialog(parent),
      m_logTable(new QTableWidget()),
      m_clearButton(new QPushButton("Clear")),
      m_closeButton(new QPushButton("Close"))
{
    setWindowTitle("Application Logs");
    setGeometry(100, 100, 800, 400);

    m_logTable->setColumnCount(2);
    m_logTable->setHorizontalHeaderLabels({"Timestamp", "Message"});
    m_logTable->horizontalHeader()->setStretchLastSection(true);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(m_closeButton);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_logTable);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    connect(m_clearButton, &QPushButton::clicked, this, &LogDialog::clearLogs);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(this, &LogDialog::addLogSignal, this, &LogDialog::addLog);
}

void LogDialog::addLog(const QString& message)
{
    int row = m_logTable->rowCount();
    m_logTable->insertRow(row);

    QTableWidgetItem* timeItem = new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    QTableWidgetItem* messageItem = new QTableWidgetItem(message);

    m_logTable->setItem(row, 0, timeItem);
    m_logTable->setItem(row, 1, messageItem);
    
    m_logTable->scrollToBottom();
}

void LogDialog::clearLogs()
{
    m_logTable->setRowCount(0);
}
