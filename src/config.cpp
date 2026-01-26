#include "config.h"

// Default configuration values
namespace {
    constexpr const char* DEFAULT_API_URL = "https://khmergaming.436bet.com/app/";
    constexpr bool DEFAULT_DEBUG_MODE = false;
    constexpr qint64 DEFAULT_DUPLICATE_THRESHOLD = 500;
    constexpr int DEFAULT_REFRESH_INTERVAL = 2000;
}

AppConfig::AppConfig()
    : m_settings("CompareObserver", "Configuration")
{
    load();
}

void AppConfig::load()
{
    m_apiUrl = m_settings.value("apiUrl", DEFAULT_API_URL).toString();
    m_debugMode = m_settings.value("debugMode", DEFAULT_DEBUG_MODE).toBool();
    m_duplicateEventThreshold = m_settings.value("duplicateEventThreshold", DEFAULT_DUPLICATE_THRESHOLD).toLongLong();
    m_autoRefreshInterval = m_settings.value("autoRefreshInterval", DEFAULT_REFRESH_INTERVAL).toInt();
}

void AppConfig::save()
{
    m_settings.setValue("apiUrl", m_apiUrl);
    m_settings.setValue("debugMode", m_debugMode);
    m_settings.setValue("duplicateEventThreshold", m_duplicateEventThreshold);
    m_settings.setValue("autoRefreshInterval", m_autoRefreshInterval);
    m_settings.sync();
}