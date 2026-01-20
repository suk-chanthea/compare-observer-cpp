#include "telegram_service.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "../utils/helpers.h"

TelegramService::TelegramService(const QString& token, const QString& chatId)
    : m_token(token),
      m_chatId(chatId),
      m_url("https://api.telegram.org/bot" + token + "/sendMessage")
{
}

bool TelegramService::sendMessage(const QString& username,
                                 const QString& description,
                                 const QString& fileList)
{
    Q_UNUSED(username);
    Q_UNUSED(fileList);
    
    // The 'description' parameter contains the complete pre-formatted message
    QString message = description;
    
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    
    QUrl url(m_url);
    QUrlQuery query;
    query.addQueryItem("chat_id", m_chatId);
    query.addQueryItem("text", message);
    // Don't use parse_mode to avoid markdown formatting issues
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            emit messageSent(true);
            qDebug() << "✓ Telegram message sent successfully!";
            qDebug() << "Response:" << response;
        } else {
            QByteArray response = reply->readAll();
            QString errorMsg = QString("Failed to send Telegram message: %1\nResponse: %2")
                              .arg(reply->errorString())
                              .arg(QString::fromUtf8(response));
            emit messageSent(false);
            emit error(errorMsg);
            qDebug() << "✗ Telegram send failed:" << errorMsg;
        }
        reply->deleteLater();
    });
    
    qDebug() << "Sending to Telegram:" << m_chatId;
    qDebug() << "Message:" << message;
    
    return true;
}
