#include "models.h"
#include <QFileInfo>

FileChangeEntry::FileChangeEntry(const QString& filePath,
                                const QString& oldContent,
                                const QString& newContent,
                                const QString& sourceRoot)
    : m_filePath(filePath),
      m_oldContent(oldContent),
      m_newContent(newContent),
      m_sourceRoot(sourceRoot),
      m_isSelected(true)
{
    // Calculate relative path
    QFileInfo fileInfo(filePath);
    m_relativePath = QFileInfo(sourceRoot).absoluteFilePath();
    if (filePath.startsWith(m_relativePath)) {
        m_relativePath = filePath.mid(m_relativePath.length() + 1);
    } else {
        m_relativePath = filePath;
    }
}

QVector<QString> FileChangeEntry::getDiffLines() const
{
    QVector<QString> diffLines;
    
    if (m_oldContent.isNull() || m_oldContent.isEmpty()) {
        // File was created
        for (const auto& line : m_newContent.split('\n')) {
            diffLines.append("+ " + line);
        }
    } else if (m_newContent.isNull() || m_newContent.isEmpty()) {
        // File was deleted
        for (const auto& line : m_oldContent.split('\n')) {
            diffLines.append("- " + line);
        }
    } else {
        // File was modified - compute unified diff
        QStringList oldLines = m_oldContent.split('\n');
        QStringList newLines = m_newContent.split('\n');
        
        // Simple diff: mark lines as added/removed/context
        // For a more sophisticated diff algorithm, consider using a library
        int maxLines = qMax(oldLines.size(), newLines.size());
        for (int i = 0; i < maxLines; ++i) {
            QString oldLine = i < oldLines.size() ? oldLines[i] : "";
            QString newLine = i < newLines.size() ? newLines[i] : "";
            
            if (oldLine == newLine) {
                diffLines.append("  " + oldLine); // Context line
            } else {
                if (!oldLine.isEmpty()) {
                    diffLines.append("- " + oldLine);
                }
                if (!newLine.isEmpty()) {
                    diffLines.append("+ " + newLine);
                }
            }
        }
    }
    
    return diffLines;
}
