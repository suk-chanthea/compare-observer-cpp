#include "helpers.h"
#include <QPixmap>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QStringConverter>

namespace Helpers {

QPixmap getPixmapFromBase64(const QString& base64String)
{
    QByteArray imageData = QByteArray::fromBase64(base64String.toLatin1());
    QPixmap pixmap;
    pixmap.loadFromData(imageData);
    return pixmap;
}

QString escapeMarkdownV2(const QString& text)
{
    QString escaped = text;
    const QString specialChars = "_*[]()~`>#+-=|{}.!";
    
    for (QChar ch : specialChars) {
        escaped.replace(ch, "\\" + QString(ch));
    }
    
    return escaped;
}

QString readFileContent(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    file.close();
    
    return content;
}

bool writeFileContent(const QString& filePath, const QString& content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << content;
    file.close();
    
    return true;
}

QString calculateMd5Hash(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    while (!file.atEnd()) {
        hash.addData(file.read(8192));
    }
    file.close();
    
    return hash.result().toHex();
}

}
