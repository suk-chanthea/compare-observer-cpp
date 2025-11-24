#ifndef CUSTOM_TEXT_EDIT_H
#define CUSTOM_TEXT_EDIT_H

#include <QPlainTextEdit>

/**
 * @brief Custom text editor with line numbers and syntax highlighting
 */
class CustomTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CustomTextEdit(QWidget* parent = nullptr);

    /**
     * @brief Gets the line number area width
     */
    int lineNumberAreaWidth() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect& rect, int dy);

private:
    void drawLineNumbers(QPaintEvent* event);

    class LineNumberArea;
    LineNumberArea* m_lineNumberArea;
};

#endif // CUSTOM_TEXT_EDIT_H
