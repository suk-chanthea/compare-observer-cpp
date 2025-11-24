#include "change_review_dialog.h"
#include "../widgets/custom_text_edit.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QSplitter>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QItemSelectionModel>

ChangeReviewDialog::ChangeReviewDialog(QWidget* parent)
    : QDialog(parent),
      m_changesTable(new QTableWidget()),
      m_diffPreview(new CustomTextEdit()),
      m_applyButton(new QPushButton("Apply Selected")),
      m_cancelButton(new QPushButton("Cancel")),
      m_splitter(new QSplitter(Qt::Vertical))
{
    setWindowTitle("Review Changes");
    setGeometry(100, 100, 900, 600);

    m_changesTable->setColumnCount(2);
    m_changesTable->setHorizontalHeaderLabels({"File Path", "Change Type"});
    m_changesTable->horizontalHeader()->setStretchLastSection(true);
    m_changesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_diffPreview->setReadOnly(true);

    m_splitter->addWidget(m_changesTable);
    m_splitter->addWidget(m_diffPreview);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_cancelButton);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_splitter);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(m_changesTable, &QTableWidget::cellClicked, this, &ChangeReviewDialog::onChangeSelected);
    connect(m_applyButton, &QPushButton::clicked, this, &ChangeReviewDialog::onApplyChanges);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::close);
}

void ChangeReviewDialog::addChange(const QString& filePath, const QString& changeType)
{
    int row = m_changesTable->rowCount();
    m_changesTable->insertRow(row);

    QTableWidgetItem* pathItem = new QTableWidgetItem(filePath);
    QTableWidgetItem* typeItem = new QTableWidgetItem(changeType);

    m_changesTable->setItem(row, 0, pathItem);
    m_changesTable->setItem(row, 1, typeItem);
}

QVector<QString> ChangeReviewDialog::getSelectedChanges() const
{
    QVector<QString> selected;
    const auto selectedRows = m_changesTable->selectionModel()->selectedRows();
    for (const QModelIndex& index : selectedRows) {
        selected.append(m_changesTable->item(index.row(), 0)->text());
    }
    return selected;
}

void ChangeReviewDialog::clearChanges()
{
    m_changesTable->setRowCount(0);
    m_diffPreview->clear();
}

void ChangeReviewDialog::onChangeSelected(int row)
{
    if (row >= 0 && row < m_changesTable->rowCount()) {
        QString filePath = m_changesTable->item(row, 0)->text();
        m_diffPreview->setPlainText("Diff preview for: " + filePath);
    }
}

void ChangeReviewDialog::onApplyChanges()
{
    accept();
}
