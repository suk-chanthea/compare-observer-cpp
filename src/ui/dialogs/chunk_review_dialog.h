#ifndef CHUNK_REVIEW_DIALOG_H
#define CHUNK_REVIEW_DIALOG_H

#include <QDialog>
#include <QString>

class CustomTextEdit;
class QPushButton;

/**
 * @brief Dialog for reviewing individual code chunks
 */
class ChunkReviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChunkReviewDialog(QWidget* parent = nullptr);

    /**
     * @brief Sets the chunk content for review
     */
    void setChunkContent(const QString& oldChunk, const QString& newChunk);

    /**
     * @brief Gets whether the chunk was approved
     */
    bool isApproved() const { return m_approved; }

private:
    CustomTextEdit* m_oldChunkEdit;
    CustomTextEdit* m_newChunkEdit;
    QPushButton* m_approveButton;
    QPushButton* m_rejectButton;
    bool m_approved;
};

#endif // CHUNK_REVIEW_DIALOG_H
