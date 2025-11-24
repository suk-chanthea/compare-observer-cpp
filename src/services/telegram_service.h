#ifndef TELEGRAM_SERVICE_H
#define TELEGRAM_SERVICE_H

#include <QString>
#include <QObject>

/**
 * @brief TelegramService handles sending notifications to Telegram
 */
class TelegramService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs a TelegramService
     * @param token Telegram bot token
     * @param chatId Telegram chat ID
     */
    TelegramService(const QString& token, const QString& chatId);

    /**
     * @brief Sends a message to Telegram
     * @param username Username for the notification
     * @param description Description of the changes
     * @param fileList List of files changed
     * @return true if message was sent successfully
     */
    bool sendMessage(const QString& username, 
                    const QString& description, 
                    const QString& fileList);

signals:
    void messageSent(bool success);
    void error(const QString& message);

private:
    QString m_token;
    QString m_chatId;
    QString m_url;
};

#endif // TELEGRAM_SERVICE_H
