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
      m_url("https://api.telegram.org/bot" + token + "/sendMessage"),
      m_manager(new QNetworkAccessManager(this))
{
    qDebug() << "🔧 TelegramService initialized";
    qDebug() << "   Bot Token:" << token.left(8) + "..." + token.right(4) << "(length:" << token.length() << ")";
    qDebug() << "   Chat ID:" << chatId;
    qDebug() << "   API URL:" << m_url;
}

bool TelegramService::sendMessage(const QString& username,
                                 const QString& description,
                                 const QString& fileList)
{
    Q_UNUSED(username);
    Q_UNUSED(fileList);
    
    // The 'description' parameter contains the complete pre-formatted message
    QString message = description;
    
    // Use POST with JSON body (recommended by Telegram Bot API)
    QJsonObject json;
    json["chat_id"] = m_chatId;
    json["text"] = message;
    json["parse_mode"] = "HTML";
    
    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();
    
    QUrl url(m_url);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    qDebug() << "📤 Sending POST request to:" << m_url;
    qDebug() << "📦 JSON Payload:" << QString::fromUtf8(postData);
    
    QNetworkReply* reply = m_manager->post(request, postData);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, message]() {
        QByteArray response = reply->readAll();
        
        if (reply->error() == QNetworkReply::NoError) {
            emit messageSent(true);
            qDebug() << "✅ Telegram message sent successfully!";
            qDebug() << "Response:" << QString::fromUtf8(response);
        } else {
            QString errorMsg = QString("Failed to send Telegram message:\nHTTP Error: %1\nStatus Code: %2\nResponse: %3")
                              .arg(reply->errorString())
                              .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
                              .arg(QString::fromUtf8(response));
            emit messageSent(false);
            emit error(errorMsg);
            qDebug() << "❌ Telegram send failed:" << errorMsg;
        }
        reply->deleteLater();
    });
    
    qDebug() << "📤 Sending to Telegram Chat ID:" << m_chatId;
    qDebug() << "📝 Message:" << message;
    qDebug() << "🔗 URL:" << m_url;
    
    return true;
}
