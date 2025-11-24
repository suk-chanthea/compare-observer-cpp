#include "events.h"

const QEvent::Type FileUpdateEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
const QEvent::Type FileCreateEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
const QEvent::Type FileDeleteEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());

FileUpdateEvent::FileUpdateEvent(const QString& filePath)
    : QEvent(EventType), m_filePath(filePath) {}

FileCreateEvent::FileCreateEvent(const QString& filePath)
    : QEvent(EventType), m_filePath(filePath) {}

FileDeleteEvent::FileDeleteEvent(const QString& filePath)
    : QEvent(EventType), m_filePath(filePath) {}
