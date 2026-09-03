#include "ui/HexView.h"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

#include <algorithm>

HexView::HexView(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setFocusPolicy(Qt::StrongFocus);
    setFrameShape(QFrame::NoFrame);
    verticalScrollBar()->setSingleStep(1);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    updateMetrics();
}

void HexView::setData(QByteArray data, const qint64 baseOffset)
{
    data_ = std::move(data);
    baseOffset_ = baseOffset;
    selectionAnchor_ = -1;
    selectionStart_ = -1;
    selectionEnd_ = -1;
    verticalScrollBar()->setValue(0);
    updateScrollBar();
    viewport()->update();
    emit byteSelectionChanged(-1, 0);
}

QSize HexView::sizeHint() const
{
    return QSize(offsetWidth_ + BytesPerRow * byteCellWidth_ + 24, 500);
}

void HexView::paintEvent(QPaintEvent*)
{
    QPainter painter(viewport());
    painter.setFont(font());
    painter.fillRect(viewport()->rect(), palette().base());

    const int contentX = -horizontalScrollBar()->value();
    const int firstRow = verticalScrollBar()->value();
    const int visibleRows = std::max(0, (viewport()->height() - headerHeight_) / lineHeight_ + 1);
    const QColor muted = QColor(QStringLiteral("#6c6a64"));
    const QColor hairline = QColor(QStringLiteral("#e6dfd8"));

    painter.fillRect(0, 0, viewport()->width(), headerHeight_, QColor(QStringLiteral("#efe9de")));
    painter.setPen(muted);
    painter.drawText(QRect(contentX + 12, 0, offsetWidth_ - 18, headerHeight_),
                     Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("OFFSET"));
    for (int column = 0; column < BytesPerRow; ++column) {
        const QRect headerCell(contentX + offsetWidth_ + column * byteCellWidth_, 0,
                               byteCellWidth_, headerHeight_);
        painter.drawText(headerCell, Qt::AlignCenter,
                         QStringLiteral("%1").arg(column, 2, 16, QLatin1Char('0')).toUpper());
    }
    painter.setPen(hairline);
    painter.drawLine(0, headerHeight_ - 1, viewport()->width(), headerHeight_ - 1);
    painter.drawLine(contentX + offsetWidth_ - 1, 0,
                     contentX + offsetWidth_ - 1, viewport()->height());

    const int rowCount = (data_.size() + BytesPerRow - 1) / BytesPerRow;
    for (int visibleRow = 0; visibleRow < visibleRows; ++visibleRow) {
        const int row = firstRow + visibleRow;
        if (row >= rowCount) {
            break;
        }
        const int y = headerHeight_ + visibleRow * lineHeight_;
        const qint64 absoluteOffset = baseOffset_ + static_cast<qint64>(row) * BytesPerRow;
        painter.setPen(muted);
        painter.drawText(QRect(contentX + 12, y, offsetWidth_ - 24, lineHeight_),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         QStringLiteral("%1").arg(static_cast<qulonglong>(absoluteOffset),
                                                   16, 16, QLatin1Char('0')).toUpper());

        for (int column = 0; column < BytesPerRow; ++column) {
            const int byteIndex = row * BytesPerRow + column;
            if (byteIndex >= data_.size()) {
                break;
            }
            const QRect cell(contentX + offsetWidth_ + column * byteCellWidth_, y,
                             byteCellWidth_, lineHeight_);
            const bool selected = selectionStart_ >= 0
                && byteIndex >= selectionStart_ && byteIndex <= selectionEnd_;
            if (selected) {
                painter.fillRect(cell.adjusted(1, 1, -1, -1), palette().highlight());
                painter.setPen(palette().highlightedText().color());
            } else {
                painter.setPen(palette().text().color());
            }
            const auto value = static_cast<unsigned char>(data_.at(byteIndex));
            painter.drawText(cell, Qt::AlignCenter,
                             QStringLiteral("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper());
        }
    }
}

void HexView::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollBar();
}

void HexView::scrollContentsBy(int, int)
{
    viewport()->update();
}

void HexView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QAbstractScrollArea::mousePressEvent(event);
        return;
    }
    const int byteIndex = byteIndexAt(event->pos());
    if (byteIndex < 0) {
        return;
    }
    selectionAnchor_ = byteIndex;
    setSelectionEnd(byteIndex);
    setFocus(Qt::MouseFocusReason);
}

void HexView::mouseMoveEvent(QMouseEvent* event)
{
    if ((event->buttons() & Qt::LeftButton) == 0 || selectionAnchor_ < 0) {
        QAbstractScrollArea::mouseMoveEvent(event);
        return;
    }
    const int byteIndex = byteIndexAt(event->pos());
    if (byteIndex >= 0) {
        setSelectionEnd(byteIndex);
    }
}

void HexView::updateMetrics()
{
    const QFontMetrics metrics(font());
    characterWidth_ = metrics.horizontalAdvance(QLatin1Char('0'));
    lineHeight_ = std::max(20, metrics.height() + 4);
    headerHeight_ = lineHeight_ + 6;
    offsetWidth_ = characterWidth_ * 16 + 24;
    const int naturalCellWidth = characterWidth_ * 3;
    const int availableForBytes = std::max(0, viewport()->width() - offsetWidth_);
    byteCellWidth_ = std::max(naturalCellWidth, availableForBytes / BytesPerRow);
    horizontalScrollBar()->setRange(
        0, std::max(0, offsetWidth_ + BytesPerRow * byteCellWidth_ - viewport()->width()));
}

void HexView::updateScrollBar()
{
    updateMetrics();
    const int rowCount = (data_.size() + BytesPerRow - 1) / BytesPerRow;
    const int visibleRows = std::max(1, (viewport()->height() - headerHeight_) / lineHeight_);
    verticalScrollBar()->setPageStep(visibleRows);
    verticalScrollBar()->setRange(0, std::max(0, rowCount - visibleRows));
}

int HexView::byteIndexAt(const QPoint& position) const
{
    if (position.y() < headerHeight_) {
        return -1;
    }
    const int contentX = position.x() + horizontalScrollBar()->value();
    if (contentX < offsetWidth_) {
        return -1;
    }
    const int column = (contentX - offsetWidth_) / byteCellWidth_;
    if (column < 0 || column >= BytesPerRow) {
        return -1;
    }
    const int row = verticalScrollBar()->value() + (position.y() - headerHeight_) / lineHeight_;
    const int byteIndex = row * BytesPerRow + column;
    return byteIndex >= 0 && byteIndex < data_.size() ? byteIndex : -1;
}

void HexView::setSelectionEnd(const int byteIndex)
{
    const int newStart = std::min(selectionAnchor_, byteIndex);
    const int newEnd = std::max(selectionAnchor_, byteIndex);
    if (selectionStart_ == newStart && selectionEnd_ == newEnd) {
        return;
    }
    selectionStart_ = newStart;
    selectionEnd_ = newEnd;
    viewport()->update();
    emit byteSelectionChanged(selectionStart_, selectionEnd_ - selectionStart_ + 1);
}
