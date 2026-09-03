#pragma once

#include <QAbstractScrollArea>
#include <QByteArray>

class HexView final : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit HexView(QWidget* parent = nullptr);

    void setData(QByteArray data, qint64 baseOffset);

    [[nodiscard]] QSize sizeHint() const override;

signals:
    void byteSelectionChanged(int start, int length);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    static constexpr int BytesPerRow = 16;

    void updateMetrics();
    void updateScrollBar();
    [[nodiscard]] int byteIndexAt(const QPoint& position) const;
    void setSelectionEnd(int byteIndex);

    QByteArray data_;
    qint64 baseOffset_ = 0;
    int selectionAnchor_ = -1;
    int selectionStart_ = -1;
    int selectionEnd_ = -1;
    int characterWidth_ = 8;
    int lineHeight_ = 18;
    int headerHeight_ = 26;
    int offsetWidth_ = 152;
    int byteCellWidth_ = 24;
};
