#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QSettings>

/**
 * @brief Application configuration manager
 * Singleton pattern for centralized configuration
 */
class AppConfig {
public:
    // Get singleton instance
    static AppConfig& instance() {
        static AppConfig config;
        return config;
    }

    // Delete copy constructor and assignment
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    // API Configuration
    QString apiUrl() const { return m_apiUrl; }
    void setApiUrl(const QString& url) { m_apiUrl = url; save(); }

    // Debug Mode
    bool isDebugMode() const { return m_debugMode; }
    void setDebugMode(bool debug) { m_debugMode = debug; save(); }

    // File Watcher Settings
    qint64 duplicateEventThreshold() const { return m_duplicateEventThreshold; }
    void setDuplicateEventThreshold(qint64 ms) { m_duplicateEventThreshold = ms; save(); }

    int autoRefreshInterval() const { return m_autoRefreshInterval; }
    void setAutoRefreshInterval(int ms) { m_autoRefreshInterval = ms; save(); }

    // Load/Save
    void load();
    void save();

private:
    AppConfig();

    QString m_apiUrl;
    bool m_debugMode;
    qint64 m_duplicateEventThreshold;
    int m_autoRefreshInterval;
    
    QSettings m_settings;
};

// Legacy namespace for backward compatibility
namespace Config {
    inline QString API_URL() { return AppConfig::instance().apiUrl(); }
    inline bool DEBUG() { return AppConfig::instance().isDebugMode(); }
}

#endif // CONFIG_H