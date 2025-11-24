#ifndef GIT_COMPARE_DIALOG_H
#define GIT_COMPARE_DIALOG_H

#include <QDialog>
#include <QString>

class QTableWidget;
class QPushButton;

/**
 * @brief Dialog for comparing git repository with source files
 */
class GitSourceCompareDialog : public QDialog {
    Q_OBJECT

public:
    explicit GitSourceCompareDialog(QWidget* parent = nullptr);

    /**
     * @brief Adds a comparison entry
     */
    void addComparisonEntry(const QString& filePath, const QString& status);

    /**
     * @brief Clears all entries
     */
    void clearEntries();

private:
    QTableWidget* m_compareTable;
    QPushButton* m_syncButton;
    QPushButton* m_cancelButton;
};

#endif // GIT_COMPARE_DIALOG_H
