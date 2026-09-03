#pragma once

#include "core/BinaryFileReader.h"

#include <QByteArray>
#include <QString>

class ViewerSession final
{
public:
    static constexpr qint64 DefaultChunkSize = 4096;

    [[nodiscard]] bool open(const QString& path);

    [[nodiscard]] bool loadPreviousChunk();
    [[nodiscard]] bool loadNextChunk();
    [[nodiscard]] bool setChunkSize(qint64 chunkSize);
    void setSelection(int start, int length);

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool canLoadPreviousChunk() const;
    [[nodiscard]] bool canLoadNextChunk() const;
    [[nodiscard]] const QString& filePath() const;
    [[nodiscard]] qint64 fileSize() const;
    [[nodiscard]] qint64 chunkSize() const;
    [[nodiscard]] qint64 chunkIndex() const;
    [[nodiscard]] qint64 currentOffset() const;
    [[nodiscard]] qint64 totalChunks() const;
    [[nodiscard]] const QByteArray& currentChunk() const;
    [[nodiscard]] int selectionStart() const;
    [[nodiscard]] int selectionLength() const;
    [[nodiscard]] QByteArray selectedBytes() const;
    [[nodiscard]] const QString& errorString() const;

private:
    [[nodiscard]] bool loadChunk(qint64 chunkIndex);
    void resetDocumentState();
    void clearSelection();

    BinaryFileReader reader_;
    QByteArray currentChunk_;
    QString filePath_;
    QString errorString_;
    qint64 fileSize_ = 0;
    qint64 chunkSize_ = DefaultChunkSize;
    qint64 chunkIndex_ = 0;
    int selectionStart_ = -1;
    int selectionLength_ = 0;
};
