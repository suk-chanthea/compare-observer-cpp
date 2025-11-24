#ifndef CHANGE_REVIEW_DIALOG_H
#define CHANGE_REVIEW_DIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

class QTableWidget;
class CustomTextEdit;
class QPushButton;
class QSplitter;

/**
 * @brief Dialog for reviewing file changes before applying
 */
class ChangeReviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChangeReviewDialog(QWidget* parent = nullptr);

    /**
     * @brief Adds a change entry for review
     */
    void addChange(const QString& filePath, const QString& changeType);

    /**
     * @brief Gets the selected changes
     */
    QVector<QString> getSelectedChanges() const;

    /**
     * @brief Clears all changes
     */
    void clearChanges();

private slots:
    void onChangeSelected(int row);
    void onApplyChanges();

private:
    QTableWidget* m_changesTable;
    CustomTextEdit* m_diffPreview;
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QSplitter* m_splitter;
};

#endif // CHANGE_REVIEW_DIALOG_H
