#ifndef FILE_DIFF_DIALOG_H
#define FILE_DIFF_DIALOG_H

#include <QDialog>
#include <QString>

class CustomTextEdit;
class QSplitter;

/**
 * @brief Dialog for comparing file differences
 */
class FileDiffDialog : public QDialog {
    Q_OBJECT

public:
    explicit FileDiffDialog(QWidget* parent = nullptr);

    /**
     * @brief Sets the file paths for comparison
     */
    void setFiles(const QString& oldFilePath, const QString& newFilePath);

    /**
     * @brief Sets the old and new content for comparison
     */
    void setContent(const QString& oldContent, const QString& newContent);

private:
    CustomTextEdit* m_oldContentEdit;
    CustomTextEdit* m_newContentEdit;
    QSplitter* m_splitter;
};

#endif // FILE_DIFF_DIALOG_H
