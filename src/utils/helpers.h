#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <QPixmap>

/**
 * @brief Utility helper functions
 */
namespace Helpers {
    /**
     * @brief Converts a Base64 string to QPixmap
     * @param base64String Base64 encoded image string
     * @return QPixmap object
     */
    QPixmap getPixmapFromBase64(const QString& base64String);

    /**
     * @brief Escapes special characters for Telegram MarkdownV2 format
     * @param text Text to escape
     * @return Escaped text safe for Telegram MarkdownV2
     */
    QString escapeMarkdownV2(const QString& text);

    /**
     * @brief Reads file content as string
     * @param filePath Path to the file
     * @return File content or empty string on error
     */
    QString readFileContent(const QString& filePath);

    /**
     * @brief Writes content to file
     * @param filePath Path to the file
     * @param content Content to write
     * @return true if successful
     */
    bool writeFileContent(const QString& filePath, const QString& content);

    /**
     * @brief Calculates MD5 hash of a file
     * @param filePath Path to the file
     * @return MD5 hash string
     */
    QString calculateMd5Hash(const QString& filePath);
}

#endif // HELPERS_H
