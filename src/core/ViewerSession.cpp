#include "core/ViewerSession.h"

#include <algorithm>
#include <limits>

bool ViewerSession::open(const QString& path)
{
    if (!reader_.open(path)) {
        errorString_ = reader_.errorString();
        resetDocumentState();
        return false;
    }

    filePath_ = path;
    fileSize_ = reader_.fileSize();
    chunkIndex_ = 0;
    if (!loadChunk(0)) {
        reader_.close();
        resetDocumentState();
        return false;
    }
    return true;
}

bool ViewerSession::loadChunk(const qint64 chunkIndex)
{
    if (!reader_.isOpen() || chunkIndex < 0 || chunkSize_ <= 0
        || (chunkIndex > 0
            && chunkIndex > std::numeric_limits<qint64>::max() / chunkSize_)) {
        errorString_ = QStringLiteral("Invalid chunk index.");
        return false;
    }

    const qint64 offset = chunkIndex * chunkSize_;
    if ((fileSize_ == 0 && chunkIndex != 0)
        || (fileSize_ > 0 && offset >= fileSize_)) {
        errorString_ = QStringLiteral("Chunk index is outside the file.");
        return false;
    }
    const auto data = reader_.readChunk(offset, chunkSize_);
    if (!data.has_value()) {
        errorString_ = reader_.errorString();
        return false;
    }

    chunkIndex_ = chunkIndex;
    currentChunk_ = *data;
    clearSelection();
    errorString_.clear();
    return true;
}

bool ViewerSession::loadPreviousChunk()
{
    return canLoadPreviousChunk() && loadChunk(chunkIndex_ - 1);
}

bool ViewerSession::loadNextChunk()
{
    return canLoadNextChunk() && loadChunk(chunkIndex_ + 1);
}

bool ViewerSession::setChunkSize(const qint64 chunkSize)
{
    if (chunkSize <= 0) {
        errorString_ = QStringLiteral("Chunk size must be positive.");
        return false;
    }
    if (chunkSize == chunkSize_) {
        return true;
    }
    if (!reader_.isOpen()) {
        chunkSize_ = chunkSize;
        errorString_.clear();
        return true;
    }

    const qint64 previousChunkSize = chunkSize_;
    const qint64 previousOffset = currentOffset();
    chunkSize_ = chunkSize;
    if (loadChunk(previousOffset / chunkSize_)) {
        return true;
    }

    chunkSize_ = previousChunkSize;
    return false;
}

void ViewerSession::setSelection(const int start, const int length)
{
    if (start < 0 || length <= 0 || start >= currentChunk_.size()) {
        clearSelection();
        return;
    }

    selectionStart_ = start;
    selectionLength_ = std::min(length, currentChunk_.size() - start);
}

bool ViewerSession::isOpen() const
{
    return reader_.isOpen();
}

bool ViewerSession::canLoadPreviousChunk() const
{
    return reader_.isOpen() && chunkIndex_ > 0;
}

bool ViewerSession::canLoadNextChunk() const
{
    return reader_.isOpen() && currentOffset() < fileSize_
        && chunkSize_ < fileSize_ - currentOffset();
}

const QString& ViewerSession::filePath() const
{
    return filePath_;
}

qint64 ViewerSession::fileSize() const
{
    return fileSize_;
}

qint64 ViewerSession::chunkSize() const
{
    return chunkSize_;
}

qint64 ViewerSession::chunkIndex() const
{
    return chunkIndex_;
}

qint64 ViewerSession::currentOffset() const
{
    if (chunkIndex_ > 0
        && chunkIndex_ > std::numeric_limits<qint64>::max() / chunkSize_) {
        return std::numeric_limits<qint64>::max();
    }
    return chunkIndex_ * chunkSize_;
}

qint64 ViewerSession::totalChunks() const
{
    if (fileSize_ <= 0) {
        return 0;
    }
    return fileSize_ / chunkSize_ + (fileSize_ % chunkSize_ != 0 ? 1 : 0);
}

const QByteArray& ViewerSession::currentChunk() const
{
    return currentChunk_;
}

int ViewerSession::selectionStart() const
{
    return selectionStart_;
}

int ViewerSession::selectionLength() const
{
    return selectionLength_;
}

QByteArray ViewerSession::selectedBytes() const
{
    if (selectionStart_ < 0 || selectionLength_ <= 0) {
        return {};
    }
    return currentChunk_.mid(selectionStart_, selectionLength_);
}

const QString& ViewerSession::errorString() const
{
    return errorString_;
}

void ViewerSession::resetDocumentState()
{
    currentChunk_.clear();
    filePath_.clear();
    fileSize_ = 0;
    chunkIndex_ = 0;
    clearSelection();
}

void ViewerSession::clearSelection()
{
    selectionStart_ = -1;
    selectionLength_ = 0;
}
