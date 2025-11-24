#ifndef EVENTS_H
#define EVENTS_H

#include <QEvent>
#include <QString>

/**
 * @brief Custom Qt event for file update notification
 */
class FileUpdateEvent : public QEvent {
public:
    static const QEvent::Type EventType;

    FileUpdateEvent(const QString& filePath);
    
    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
};

/**
 * @brief Custom Qt event for file creation notification
 */
class FileCreateEvent : public QEvent {
public:
    static const QEvent::Type EventType;

    FileCreateEvent(const QString& filePath);
    
    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
};

/**
 * @brief Custom Qt event for file deletion notification
 */
class FileDeleteEvent : public QEvent {
public:
    static const QEvent::Type EventType;

    FileDeleteEvent(const QString& filePath);
    
    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
};

#endif // EVENTS_H
