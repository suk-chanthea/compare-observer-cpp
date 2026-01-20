#ifndef FILE_DIFF_DIALOG_H
#define FILE_DIFF_DIALOG_H

#include <QDialog>
#include <QString>

class CustomTextEdit;
class QSplitter;
class QTimer;
class QLabel;

/**
 * @brief Dialog for comparing file differences with auto-refresh
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
     * @brief Sets the old and new content for comparison with auto-refresh
     */
    void setContent(const QString& oldContent, const QString& newContent);
    
    /**
     * @brief Sets the file path for live monitoring
     */
    void setLiveFile(const QString& filePath, const QString& oldContent);

private slots:
    void refreshContent();

private:
    void highlightDifferences(const QString& oldContent, const QString& newContent);
    QString readFileContent(const QString& filePath);

    CustomTextEdit* m_oldContentEdit;
    CustomTextEdit* m_newContentEdit;
    QSplitter* m_splitter;
    QTimer* m_refreshTimer;
    QLabel* m_statusLabel;
    
    QString m_filePath;
    QString m_baselineContent;
    QString m_lastContent;
};

#endif // FILE_DIFF_DIALOG_H
