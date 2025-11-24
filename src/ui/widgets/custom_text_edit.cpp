#include "custom_text_edit.h"
#include <QWidget>
#include <QPainter>
#include <QTextBlock>
#include <QResizeEvent>
#include <QPaintEvent>

class CustomTextEdit::LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CustomTextEdit* editor) : QWidget(editor), m_editor(editor) {}

    QSize sizeHint() const override {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        m_editor->drawLineNumbers(event);
    }

private:
    CustomTextEdit* m_editor;
};

CustomTextEdit::CustomTextEdit(QWidget* parent)
    : QPlainTextEdit(parent), m_lineNumberArea(new LineNumberArea(this))
{
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CustomTextEdit::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CustomTextEdit::updateLineNumberArea);
    
    updateLineNumberAreaWidth();
}

int CustomTextEdit::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, document()->blockCount());
    while (max >= 10) {
        ++digits;
        max /= 10;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CustomTextEdit::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CustomTextEdit::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void CustomTextEdit::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height());
}

void CustomTextEdit::paintEvent(QPaintEvent* event)
{
    QPlainTextEdit::paintEvent(event);
    m_lineNumberArea->update();
}

void CustomTextEdit::drawLineNumbers(QPaintEvent* event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, m_lineNumberArea->width(), fontMetrics().height(),
                            Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}
