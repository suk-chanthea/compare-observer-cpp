#include "file_diff_dialog.h"
#include "../widgets/custom_text_edit.h"
#include "../../config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QColor>
#include <QTimer>
#include <QFile>
#include <QDateTime>
#include <QMap>
#include <QScrollBar>
#include <QBrush>
#include <QDebug>

FileDiffDialog::FileDiffDialog(QWidget* parent)
    : QDialog(parent),
      m_oldContentEdit(new CustomTextEdit()),
      m_newContentEdit(new CustomTextEdit()),
      m_splitter(new QSplitter(Qt::Horizontal)),
      m_refreshTimer(new QTimer(this)),
      m_statusLabel(new QLabel("")),
      m_syncingScroll(false)
{
    setWindowTitle("File Diff Viewer - Live");
    setGeometry(100, 100, 1000, 600);

    m_oldContentEdit->setReadOnly(true);
    m_newContentEdit->setReadOnly(true);
    
    // Plain styling - dark theme
    QString plainTextStyle = 
        "QPlainTextEdit {"
        "   background-color: #1E1E1E;"
        "   color: #D4D4D4;"
        "   border: 1px solid #3E3E3E;"
        "   font-family: 'Consolas', 'Courier New', monospace;"
        "   font-size: 10pt;"
        "   selection-background-color: #264F78;"
        "   selection-color: #FFFFFF;"
        "}";
    
    m_oldContentEdit->setStyleSheet(plainTextStyle);
    m_newContentEdit->setStyleSheet(plainTextStyle);

    QWidget* oldWidget = new QWidget(this);
    QVBoxLayout* oldLayout = new QVBoxLayout(oldWidget);
    QLabel* oldLabel = new QLabel("Old Content (Baseline) - 🔴 Removed, 🟠 Modified", oldWidget);
    oldLabel->setStyleSheet("font-weight: bold; color: #CCCCCC; font-size: 9pt;");
    oldLayout->addWidget(oldLabel);
    oldLayout->addWidget(m_oldContentEdit);

    QWidget* newWidget = new QWidget(this);
    QVBoxLayout* newLayout = new QVBoxLayout(newWidget);
    QLabel* newLabel = new QLabel("New Content (Live) - 🟢 Added, 🟠 Modified", newWidget);
    newLabel->setStyleSheet("font-weight: bold; color: #CCCCCC; font-size: 9pt;");
    newLayout->addWidget(newLabel);
    newLayout->addWidget(m_newContentEdit);

    m_splitter->addWidget(oldWidget);
    m_splitter->addWidget(newWidget);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    
    QList<int> sizes;
    sizes << 500 << 500;
    m_splitter->setSizes(sizes);

    m_statusLabel->setStyleSheet("color: #888888; font-size: 9pt;");
    m_statusLabel->setText("🔄 Auto-refresh enabled");

    QPushButton* closeButton = new QPushButton("Close", this);
    closeButton->setStyleSheet("background-color: #0B57D0; color: white; padding: 6px 16px; border-radius: 4px;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_splitter);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_statusLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    
    // Setup synchronized scrolling
    connect(m_oldContentEdit->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &FileDiffDialog::syncOldToNew);
    connect(m_newContentEdit->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &FileDiffDialog::syncNewToOld);
    
    // Use configurable refresh interval
    int refreshInterval = AppConfig::instance().autoRefreshInterval();
    m_refreshTimer->setInterval(refreshInterval);
    connect(m_refreshTimer, &QTimer::timeout, this, &FileDiffDialog::refreshContent);
}

void FileDiffDialog::setFiles(const QString& oldFilePath, const QString& newFilePath)
{
    setWindowTitle(QString("Diff: %1 vs %2").arg(oldFilePath, newFilePath));
}

void FileDiffDialog::setContent(const QString& oldContent, const QString& newContent)
{
    // highlightDifferences will set the content and add padding
    highlightDifferences(oldContent, newContent);
}

void FileDiffDialog::setLiveFile(const QString& filePath, const QString& oldContent)
{
    m_filePath = filePath;
    m_baselineContent = oldContent;
    m_lastContent = readFileContent(filePath);
    
    // highlightDifferences will set the content and add padding
    highlightDifferences(m_baselineContent, m_lastContent);
    
    // Start auto-refresh
    m_refreshTimer->start();
    m_statusLabel->setText("🔄 Auto-refresh: Active");
}

void FileDiffDialog::refreshContent()
{
    if (m_filePath.isEmpty()) {
        return;
    }
    
    QString currentContent = readFileContent(m_filePath);
    
    // Quick size check before expensive string comparison
    if (currentContent.size() == m_lastContent.size()) {
        // Only do full comparison if sizes match
        if (currentContent == m_lastContent) {
            return;
        }
    }
    
    m_lastContent = currentContent;
    highlightDifferences(m_baselineContent, currentContent);
    
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_statusLabel->setText(QString("🔄 Updated: %1").arg(timestamp));
}

void FileDiffDialog::highlightDifferences(const QString& oldContent, const QString& newContent)
{
    // Quick check - if identical, no need to process
    if (oldContent == newContent) {
        m_oldContentEdit->setPlainText(oldContent);
        m_newContentEdit->setPlainText(newContent);
        return;
    }
    
    QStringList oldLines = oldContent.split('\n');
    QStringList newLines = newContent.split('\n');
    
    // Build aligned versions with empty lines inserted where needed
    QStringList alignedOld;
    QStringList alignedNew;
    
    int oldIdx = 0;
    int newIdx = 0;
    
    // Alignment algorithm (same as before but with better comments)
    while (oldIdx < oldLines.size() || newIdx < newLines.size()) {
        if (oldIdx < oldLines.size() && newIdx < newLines.size() && 
            oldLines[oldIdx] == newLines[newIdx]) {
            // Lines match - add both
            alignedOld.append(oldLines[oldIdx]);
            alignedNew.append(newLines[newIdx]);
            oldIdx++;
            newIdx++;
        }
        else if (oldIdx >= oldLines.size() && newIdx < newLines.size()) {
            // Old exhausted - add remaining new lines
            alignedOld.append("");
            alignedNew.append(newLines[newIdx]);
            newIdx++;
        }
        else if (newIdx >= newLines.size() && oldIdx < oldLines.size()) {
            // New exhausted - add remaining old lines
            alignedOld.append(oldLines[oldIdx]);
            alignedNew.append("");
            oldIdx++;
        }
        else {
            // Both have content but don't match - look ahead
            int oldLookAhead = -1;
            int newLookAhead = -1;
            
            // Search next 5 lines for a match
            for (int i = 1; i <= 5 && oldIdx + i < oldLines.size(); i++) {
                for (int j = 0; j <= 5 && newIdx + j < newLines.size(); j++) {
                    if (oldLines[oldIdx + i] == newLines[newIdx + j]) {
                        oldLookAhead = i;
                        newLookAhead = j;
                        break;
                    }
                }
                if (oldLookAhead >= 0) break;
            }
            
            if (oldLookAhead >= 0 && newLookAhead >= 0) {
                // Found match - insert padding
                if (oldLookAhead < newLookAhead) {
                    for (int i = 0; i < newLookAhead; i++) {
                        if (i < oldLookAhead) {
                            alignedOld.append(oldLines[oldIdx++]);
                        } else {
                            alignedOld.append("");
                        }
                        if (newIdx < newLines.size()) {
                            alignedNew.append(newLines[newIdx++]);
                        }
                    }
                } else {
                    for (int i = 0; i < oldLookAhead; i++) {
                        if (oldIdx < oldLines.size()) {
                            alignedOld.append(oldLines[oldIdx++]);
                        }
                        if (i < newLookAhead) {
                            alignedNew.append(newLines[newIdx++]);
                        } else {
                            alignedNew.append("");
                        }
                    }
                }
            } else {
                // No match found - treat as modification or separate add/remove
                bool oldHasMore = (oldIdx < oldLines.size());
                bool newHasMore = (newIdx < newLines.size());
                
                if (oldHasMore && newHasMore) {
                    QString oldLine = oldLines[oldIdx].trimmed();
                    QString newLine = newLines[newIdx].trimmed();
                    
                    if (oldLine.isEmpty() || newLine.isEmpty() || 
                        (oldLine != newLine && oldLine.length() > 3 && newLine.length() > 3)) {
                        // Separate rows
                        alignedOld.append(oldLines[oldIdx++]);
                        alignedNew.append("");
                    } else {
                        // Same row (modified)
                        alignedOld.append(oldLines[oldIdx++]);
                        alignedNew.append(newLines[newIdx++]);
                    }
                } else if (oldHasMore) {
                    alignedOld.append(oldLines[oldIdx++]);
                    alignedNew.append("");
                } else if (newHasMore) {
                    alignedOld.append("");
                    alignedNew.append(newLines[newIdx++]);
                } else {
                    break;
                }
            }
        }
    }
    
    // Ensure alignment
    while (alignedOld.size() < alignedNew.size()) {
        alignedOld.append("");
    }
    while (alignedNew.size() < alignedOld.size()) {
        alignedNew.append("");
    }
    
    // Batch update for performance
    m_oldContentEdit->setUpdatesEnabled(false);
    m_newContentEdit->setUpdatesEnabled(false);
    
    m_oldContentEdit->setPlainText(alignedOld.join('\n'));
    m_newContentEdit->setPlainText(alignedNew.join('\n'));
    
    // Apply highlighting
    applyHighlighting(alignedOld, alignedNew);
    
    m_oldContentEdit->setUpdatesEnabled(true);
    m_newContentEdit->setUpdatesEnabled(true);
}

void FileDiffDialog::applyHighlighting(const QStringList& oldLines, const QStringList& newLines)
{
    QColor redBg(220, 38, 38, 60);
    QColor greenBg(34, 197, 94, 60);
    QColor orangeBg(251, 140, 0, 60);
    QColor darkBg(30, 30, 30);
    
    QTextCursor oldCursor(m_oldContentEdit->document());
    QTextCursor newCursor(m_newContentEdit->document());
    
    oldCursor.movePosition(QTextCursor::Start);
    newCursor.movePosition(QTextCursor::Start);
    
    for (int i = 0; i < oldLines.size(); ++i) {
        QString oldLine = oldLines[i];
        QString newLine = i < newLines.size() ? newLines[i] : "";
        
        bool oldEmpty = oldLine.isEmpty();
        bool newEmpty = newLine.isEmpty();
        bool linesMatch = (oldLine == newLine);
        
        // Highlight old content line
        oldCursor.select(QTextCursor::LineUnderCursor);
        QTextCharFormat oldFormat;
        
        if (oldEmpty && !newEmpty) {
            oldFormat.setBackground(QBrush(redBg));
        } else if (!oldEmpty && newEmpty) {
            oldFormat.setBackground(QBrush(redBg));
        } else if (linesMatch) {
            oldFormat.setBackground(QBrush(darkBg));
        } else if (!oldEmpty && !newEmpty) {
            oldFormat.setBackground(QBrush(orangeBg));
        } else {
            oldFormat.setBackground(QBrush(darkBg));
        }
        
        oldCursor.mergeCharFormat(oldFormat);
        oldCursor.clearSelection();
        oldCursor.movePosition(QTextCursor::NextBlock);
        
        // Highlight new content line
        if (i < newLines.size()) {
            newCursor.select(QTextCursor::LineUnderCursor);
            QTextCharFormat newFormat;
            
            if (newEmpty && !oldEmpty) {
                newFormat.setBackground(QBrush(greenBg));
            } else if (!newEmpty && oldEmpty) {
                newFormat.setBackground(QBrush(greenBg));
            } else if (linesMatch) {
                newFormat.setBackground(QBrush(darkBg));
            } else if (!oldEmpty && !newEmpty) {
                newFormat.setBackground(QBrush(orangeBg));
            } else {
                newFormat.setBackground(QBrush(darkBg));
            }
            
            newCursor.mergeCharFormat(newFormat);
            newCursor.clearSelection();
            newCursor.movePosition(QTextCursor::NextBlock);
        }
    }
}

QString FileDiffDialog::readFileContent(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QByteArray data = file.readAll();
    file.close();
    return QString::fromUtf8(data);
}

void FileDiffDialog::syncOldToNew()
{
    if (m_syncingScroll) return;
    m_syncingScroll = true;
    
    QScrollBar* oldBar = m_oldContentEdit->verticalScrollBar();
    QScrollBar* newBar = m_newContentEdit->verticalScrollBar();
    
    // Calculate the scroll percentage from old scrollbar
    double scrollPercentage = 0.0;
    if (oldBar->maximum() > 0) {
        scrollPercentage = static_cast<double>(oldBar->value()) / static_cast<double>(oldBar->maximum());
    }
    
    // Apply the same percentage to new scrollbar
    int newValue = static_cast<int>(scrollPercentage * newBar->maximum());
    newBar->setValue(newValue);
    
    m_syncingScroll = false;
}

void FileDiffDialog::syncNewToOld()
{
    if (m_syncingScroll) return;
    m_syncingScroll = true;
    
    QScrollBar* oldBar = m_oldContentEdit->verticalScrollBar();
    QScrollBar* newBar = m_newContentEdit->verticalScrollBar();
    
    // Calculate the scroll percentage from new scrollbar
    double scrollPercentage = 0.0;
    if (newBar->maximum() > 0) {
        scrollPercentage = static_cast<double>(newBar->value()) / static_cast<double>(newBar->maximum());
    }
    
    // Apply the same percentage to old scrollbar
    int oldValue = static_cast<int>(scrollPercentage * oldBar->maximum());
    oldBar->setValue(oldValue);
    
    m_syncingScroll = false;
}
