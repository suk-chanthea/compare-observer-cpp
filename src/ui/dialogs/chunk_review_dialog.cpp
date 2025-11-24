#include "chunk_review_dialog.h"
#include "../widgets/custom_text_edit.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

ChunkReviewDialog::ChunkReviewDialog(QWidget* parent)
    : QDialog(parent),
      m_oldChunkEdit(new CustomTextEdit()),
      m_newChunkEdit(new CustomTextEdit()),
      m_approveButton(new QPushButton("Approve")),
      m_rejectButton(new QPushButton("Reject")),
      m_approved(false)
{
    setWindowTitle("Review Code Chunk");
    setGeometry(100, 100, 800, 400);

    m_oldChunkEdit->setReadOnly(true);
    m_newChunkEdit->setReadOnly(true);

    QHBoxLayout* mainLayout = new QHBoxLayout();

    QVBoxLayout* oldLayout = new QVBoxLayout();
    oldLayout->addWidget(new QLabel("Old Code:"));
    oldLayout->addWidget(m_oldChunkEdit);

    QVBoxLayout* newLayout = new QVBoxLayout();
    newLayout->addWidget(new QLabel("New Code:"));
    newLayout->addWidget(m_newChunkEdit);

    QVBoxLayout* leftVbox = new QVBoxLayout();
    leftVbox->addLayout(oldLayout);
    leftVbox->addLayout(newLayout);
    mainLayout->addLayout(leftVbox);

    QVBoxLayout* buttonLayout = new QVBoxLayout();
    buttonLayout->addWidget(m_approveButton);
    buttonLayout->addWidget(m_rejectButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(m_approveButton, &QPushButton::clicked, this, [this]() {
        m_approved = true;
        accept();
    });

    connect(m_rejectButton, &QPushButton::clicked, this, &QDialog::reject);
}

void ChunkReviewDialog::setChunkContent(const QString& oldChunk, const QString& newChunk)
{
    m_oldChunkEdit->setPlainText(oldChunk);
    m_newChunkEdit->setPlainText(newChunk);
}
