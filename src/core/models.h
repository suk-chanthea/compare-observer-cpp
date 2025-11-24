#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QVector>

/**
 * @brief Represents a tracked file change.
 */
class FileChangeEntry
{
public:
    FileChangeEntry(const QString& filePath,
                    const QString& oldContent,
                    const QString& newContent,
                    const QString& sourceRoot);

    const QString& filePath() const { return m_filePath; }
    const QString& oldContent() const { return m_oldContent; }
    const QString& newContent() const { return m_newContent; }
    const QString& sourceRoot() const { return m_sourceRoot; }
    const QString& relativePath() const { return m_relativePath; }

    bool isSelected() const { return m_isSelected; }
    void setSelected(bool selected) { m_isSelected = selected; }

    QVector<QString> getDiffLines() const;

private:
    QString m_filePath;
    QString m_oldContent;
    QString m_newContent;
    QString m_sourceRoot;
    QString m_relativePath;
    bool m_isSelected;
};

#endif // MODELS_H

