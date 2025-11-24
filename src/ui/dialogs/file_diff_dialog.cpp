#include "file_diff_dialog.h"
#include "../widgets/custom_text_edit.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>

FileDiffDialog::FileDiffDialog(QWidget* parent)
    : QDialog(parent),
      m_oldContentEdit(new CustomTextEdit()),
      m_newContentEdit(new CustomTextEdit()),
      m_splitter(new QSplitter(Qt::Horizontal))
{
    setWindowTitle("File Diff Viewer");
    setGeometry(100, 100, 1000, 600);

    m_oldContentEdit->setReadOnly(true);
    m_newContentEdit->setReadOnly(true);

    QWidget* oldWidget = new QWidget();
    QVBoxLayout* oldLayout = new QVBoxLayout();
    oldLayout->addWidget(new QLabel("Old Content:"));
    oldLayout->addWidget(m_oldContentEdit);
    oldWidget->setLayout(oldLayout);

    QWidget* newWidget = new QWidget();
    QVBoxLayout* newLayout = new QVBoxLayout();
    newLayout->addWidget(new QLabel("New Content:"));
    newLayout->addWidget(m_newContentEdit);
    newWidget->setLayout(newLayout);

    m_splitter->addWidget(oldWidget);
    m_splitter->addWidget(newWidget);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    QPushButton* closeButton = new QPushButton("Close");

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_splitter);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void FileDiffDialog::setFiles(const QString& oldFilePath, const QString& newFilePath)
{
    setWindowTitle(QString("Diff: %1 vs %2").arg(oldFilePath, newFilePath));
}

void FileDiffDialog::setContent(const QString& oldContent, const QString& newContent)
{
    m_oldContentEdit->setPlainText(oldContent);
    m_newContentEdit->setPlainText(newContent);
}
