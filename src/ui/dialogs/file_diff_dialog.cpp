#include "file_diff_dialog.h"
#include "../widgets/custom_text_edit.h"
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
    
    // Set plain styling with no color highlighting - just dark theme
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

    QWidget* oldWidget = new QWidget();
    QVBoxLayout* oldLayout = new QVBoxLayout();
    QLabel* oldLabel = new QLabel("Old Content (Baseline) - 🔴 Removed, 🟠 Modified");
    oldLabel->setStyleSheet("font-weight: bold; color: #CCCCCC; font-size: 9pt;");
    oldLayout->addWidget(oldLabel);
    oldLayout->addWidget(m_oldContentEdit);
    oldWidget->setLayout(oldLayout);

    QWidget* newWidget = new QWidget();
    QVBoxLayout* newLayout = new QVBoxLayout();
    QLabel* newLabel = new QLabel("New Content (Live) - 🟢 Added, 🟠 Modified");
    newLabel->setStyleSheet("font-weight: bold; color: #CCCCCC; font-size: 9pt;");
    newLayout->addWidget(newLabel);
    newLayout->addWidget(m_newContentEdit);
    newWidget->setLayout(newLayout);

    m_splitter->addWidget(oldWidget);
    m_splitter->addWidget(newWidget);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    
    // Set equal sizes for both panels
    QList<int> sizes;
    sizes << 500 << 500;  // Equal width
    m_splitter->setSizes(sizes);

    // Status label for auto-refresh
    m_statusLabel->setStyleSheet("color: #888888; font-size: 9pt;");
    m_statusLabel->setText("🔄 Auto-refresh enabled");

    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setStyleSheet("background-color: #0B57D0; color: white; padding: 6px 16px; border-radius: 4px;");

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(m_splitter);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_statusLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    
    // Setup synchronized scrolling with line-based synchronization
    connect(m_oldContentEdit->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &FileDiffDialog::syncOldToNew);
    connect(m_newContentEdit->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &FileDiffDialog::syncNewToOld);
    
    // Setup auto-refresh timer (every 2 seconds)
    m_refreshTimer->setInterval(2000);
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
    
    // Only update if content changed
    if (currentContent != m_lastContent) {
        m_lastContent = currentContent;
        // highlightDifferences will set the content and add padding
        highlightDifferences(m_baselineContent, currentContent);
        
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_statusLabel->setText(QString("🔄 Updated: %1").arg(timestamp));
    }
}

void FileDiffDialog::highlightDifferences(const QString& oldContent, const QString& newContent)
{
    QStringList oldLines = oldContent.split('\n');
    QStringList newLines = newContent.split('\n');
    
    // Build aligned versions with empty lines inserted where needed
    QStringList alignedOld;
    QStringList alignedNew;
    
    int oldIdx = 0;
    int newIdx = 0;
    
    while (oldIdx < oldLines.size() || newIdx < newLines.size()) {
        // If both have lines and they match, add them both
        if (oldIdx < oldLines.size() && newIdx < newLines.size() && 
            oldLines[oldIdx] == newLines[newIdx]) {
            alignedOld.append(oldLines[oldIdx]);
            alignedNew.append(newLines[newIdx]);
            oldIdx++;
            newIdx++;
        }
        // If old is done, add remaining new lines with empty old lines
        else if (oldIdx >= oldLines.size() && newIdx < newLines.size()) {
            alignedOld.append("");  // Empty line in old
            alignedNew.append(newLines[newIdx]);
            newIdx++;
        }
        // If new is done, add remaining old lines with empty new lines
        else if (newIdx >= newLines.size() && oldIdx < oldLines.size()) {
            alignedOld.append(oldLines[oldIdx]);
            alignedNew.append("");  // Empty line in new
            oldIdx++;
        }
        // Both have content but don't match - check which side to advance
        else {
            // Look ahead to find matching lines
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
                // Insert empty lines on the side that has fewer lines until match
                if (oldLookAhead < newLookAhead) {
                    // More lines added in new - insert empties in old
                    for (int i = 0; i < newLookAhead; i++) {
                        if (i < oldLookAhead) {
                            alignedOld.append(oldLines[oldIdx++]);
                        } else {
                            alignedOld.append("");  // Empty line in old
                        }
                        if (newIdx < newLines.size()) {
                            alignedNew.append(newLines[newIdx++]);
                        }
                    }
                } else {
                    // More lines removed from old - insert empties in new
                    for (int i = 0; i < oldLookAhead; i++) {
                        if (oldIdx < oldLines.size()) {
                            alignedOld.append(oldLines[oldIdx++]);
                        }
                        if (i < newLookAhead) {
                            alignedNew.append(newLines[newIdx++]);
                        } else {
                            alignedNew.append("");  // Empty line in new
                        }
                    }
                }
            } else {
                // No match found within lookahead window
                // Check if one side ran out of lines
                bool oldHasMore = (oldIdx < oldLines.size());
                bool newHasMore = (newIdx < newLines.size());
                
                if (oldHasMore && newHasMore) {
                    // Both have lines - check if they're similar enough to be "modified"
                    QString oldLine = oldLines[oldIdx].trimmed();
                    QString newLine = newLines[newIdx].trimmed();
                    
                    // If lines are very different or one is empty, treat as separate add/remove
                    if (oldLine.isEmpty() || newLine.isEmpty() || 
                        (oldLine != newLine && oldLine.length() > 3 && newLine.length() > 3)) {
                        // Treat as removal + addition (separate rows)
                        alignedOld.append(oldLines[oldIdx++]);
                        alignedNew.append("");  // Empty on new side
                        
                        // Don't advance newIdx yet, handle in next iteration
                    } else {
                        // Similar enough - treat as modified (same row)
                        alignedOld.append(oldLines[oldIdx++]);
                        alignedNew.append(newLines[newIdx++]);
                    }
                } else if (oldHasMore) {
                    // Only old has more - removal
                    alignedOld.append(oldLines[oldIdx++]);
                    alignedNew.append("");
                } else if (newHasMore) {
                    // Only new has more - addition
                    alignedOld.append("");
                    alignedNew.append(newLines[newIdx++]);
                } else {
                    // Both ran out (shouldn't happen but handle it)
                    break;
                }
            }
        }
    }
    
    // Verify alignment - both should have same number of lines
    if (alignedOld.size() != alignedNew.size()) {
        // Pad the shorter one with empty lines
        while (alignedOld.size() < alignedNew.size()) {
            alignedOld.append("");
        }
        while (alignedNew.size() < alignedOld.size()) {
            alignedNew.append("");
        }
    }
    
    // Update display with aligned content
    m_oldContentEdit->setPlainText(alignedOld.join('\n'));
    m_newContentEdit->setPlainText(alignedNew.join('\n'));
    
    // Now work with the aligned versions
    oldLines = alignedOld;
    newLines = alignedNew;
    
    // Define highlight colors for different types of changes
    QColor redBg(220, 38, 38, 60);      // Red - line removed or empty on old side
    QColor greenBg(34, 197, 94, 60);    // Green - line added or empty on new side
    QColor orangeBg(251, 140, 0, 60);   // Orange - line modified
    QColor darkBg(30, 30, 30);          // Dark - unchanged
    
    // Since content is now aligned, we can compare line-by-line
    QTextCursor oldCursor(m_oldContentEdit->document());
    QTextCursor newCursor(m_newContentEdit->document());
    
    oldCursor.movePosition(QTextCursor::Start);
    newCursor.movePosition(QTextCursor::Start);
    
    for (int i = 0; i < qMax(oldLines.size(), newLines.size()); ++i) {
        QString oldLine = (i < oldLines.size()) ? oldLines[i] : "";
        QString newLine = (i < newLines.size()) ? newLines[i] : "";
        
        // Determine the type of difference
        bool oldEmpty = oldLine.isEmpty();
        bool newEmpty = newLine.isEmpty();
        bool linesMatch = (oldLine == newLine);
        
        // Highlight old content line
        if (i < oldLines.size()) {
            oldCursor.select(QTextCursor::LineUnderCursor);
            QTextCharFormat oldFormat;
            
            if (oldEmpty && !newEmpty) {
                // Empty on old side, content on new side = RED empty placeholder
                oldFormat.setBackground(QBrush(redBg));
            } else if (!oldEmpty && newEmpty) {
                // Content on old side, empty on new side = RED removed
                oldFormat.setBackground(QBrush(redBg));
            } else if (linesMatch) {
                // Lines match exactly = unchanged
                oldFormat.setBackground(QBrush(darkBg));
            } else if (!oldEmpty && !newEmpty) {
                // Both have content but different = modified ORANGE
                oldFormat.setBackground(QBrush(orangeBg));
            } else {
                // Both empty - unchanged
                oldFormat.setBackground(QBrush(darkBg));
            }
            
            oldCursor.mergeCharFormat(oldFormat);
            oldCursor.clearSelection();
            oldCursor.movePosition(QTextCursor::NextBlock);
        }
        
        // Highlight new content line
        if (i < newLines.size()) {
            newCursor.select(QTextCursor::LineUnderCursor);
            QTextCharFormat newFormat;
            
            if (newEmpty && !oldEmpty) {
                // Empty on new side, content on old side = GREEN empty placeholder
                newFormat.setBackground(QBrush(greenBg));
            } else if (!newEmpty && oldEmpty) {
                // Content on new side, empty on old side = GREEN added
                newFormat.setBackground(QBrush(greenBg));
            } else if (linesMatch) {
                // Lines match exactly = unchanged
                newFormat.setBackground(QBrush(darkBg));
            } else if (!oldEmpty && !newEmpty) {
                // Both have content but different = modified ORANGE
                newFormat.setBackground(QBrush(orangeBg));
            } else {
                // Both empty - unchanged
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
