#ifndef LOG_TABLE_MODEL_H
#define LOG_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QString>

/**
 * @brief Custom table model for displaying logs
 */
class LogTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    LogTableModel(QObject* parent = nullptr);

    /**
     * @brief Gets the number of rows
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Gets the number of columns
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Gets data for a specific cell
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Gets header data
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Adds a log entry
     */
    void addLog(const QString& timestamp, const QString& filePath, const QString& action);

    /**
     * @brief Clears all logs
     */
    void clear();

private:
    struct LogEntry {
        QString timestamp;
        QString filePath;
        QString action;
    };

    QList<LogEntry> m_logs;
};

#endif // LOG_TABLE_MODEL_H
