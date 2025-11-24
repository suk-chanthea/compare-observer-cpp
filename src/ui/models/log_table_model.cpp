#include "log_table_model.h"
#include <QDateTime>

LogTableModel::LogTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int LogTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_logs.size();
}

int LogTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 3;
}

QVariant LogTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_logs.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const LogEntry& entry = m_logs[index.row()];
        switch (index.column()) {
            case 0: return entry.timestamp;
            case 1: return entry.filePath;
            case 2: return entry.action;
            default: return QVariant();
        }
    }

    return QVariant();
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "Timestamp";
            case 1: return "File Path";
            case 2: return "Action";
            default: return QVariant();
        }
    }

    return QVariant();
}

void LogTableModel::addLog(const QString& timestamp, const QString& filePath, const QString& action)
{
    beginInsertRows(QModelIndex(), m_logs.size(), m_logs.size());
    m_logs.append({timestamp, filePath, action});
    endInsertRows();
}

void LogTableModel::clear()
{
    beginResetModel();
    m_logs.clear();
    endResetModel();
}
