#ifndef LOG_DIALOG_H
#define LOG_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>

/**
 * @brief Dialog for displaying application logs
 */
class LogDialog : public QDialog {
    Q_OBJECT

public:
    explicit LogDialog(QWidget* parent = nullptr);

    /**
     * @brief Adds a log entry
     */
    void addLog(const QString& message);

    /**
     * @brief Clears all logs
     */
    void clearLogs();

signals:
    void addLogSignal(const QString& message);

private:
    QTableWidget* m_logTable;
    QPushButton* m_clearButton;
    QPushButton* m_closeButton;
};

#endif // LOG_DIALOG_H
