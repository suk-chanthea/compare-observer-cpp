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
    QString formattedDescription = username + ": " + description;
    QString escapedFileList = Helpers::escapeMarkdownV2(fileList);
    QString formattedMessage = "`" + formattedDescription + "`\n" + escapedFileList;
    
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    
    QUrl url(m_url);
    QUrlQuery query;
    query.addQueryItem("chat_id", m_chatId);
    query.addQueryItem("text", formattedMessage);
    query.addQueryItem("parse_mode", "MarkdownV2");
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->post(request, QByteArray());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit messageSent(true);
            qDebug() << "Message sent successfully!";
        } else {
            emit messageSent(false);
            emit error("Failed to send message: " + reply->errorString());
            qDebug() << "Failed to send message. Error:" << reply->errorString();
        }
        reply->deleteLater();
    });
    
    return true;
}
