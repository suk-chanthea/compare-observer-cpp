#include "git_compare_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QHeaderView>

GitSourceCompareDialog::GitSourceCompareDialog(QWidget* parent)
    : QDialog(parent),
      m_compareTable(new QTableWidget()),
      m_syncButton(new QPushButton("Sync")),
      m_cancelButton(new QPushButton("Cancel"))
{
    setWindowTitle("Git Source Compare");
    setGeometry(100, 100, 700, 500);

    m_compareTable->setColumnCount(3);
    m_compareTable->setHorizontalHeaderLabels({"File Path", "Status", "Action"});
    m_compareTable->horizontalHeader()->setStretchLastSection(true);
    m_compareTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_syncButton);
    buttonLayout->addWidget(m_cancelButton);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_compareTable);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::close);
}

void GitSourceCompareDialog::addComparisonEntry(const QString& filePath, const QString& status)
{
    int row = m_compareTable->rowCount();
    m_compareTable->insertRow(row);

    QTableWidgetItem* pathItem = new QTableWidgetItem(filePath);
    QTableWidgetItem* statusItem = new QTableWidgetItem(status);
    QTableWidgetItem* actionItem = new QTableWidgetItem("Pending");

    m_compareTable->setItem(row, 0, pathItem);
    m_compareTable->setItem(row, 1, statusItem);
    m_compareTable->setItem(row, 2, actionItem);
}

void GitSourceCompareDialog::clearEntries()
{
    m_compareTable->setRowCount(0);
}
