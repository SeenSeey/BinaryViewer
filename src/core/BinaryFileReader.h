#pragma once

#include <QByteArray>
#include <QFile>
#include <QString>

#include <optional>

class BinaryFileReader
{
public:
    [[nodiscard]] bool open(const QString& path);
    void close();

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] qint64 fileSize() const;
    [[nodiscard]] QString errorString() const;

    [[nodiscard]] std::optional<QByteArray> readChunk(qint64 offset, qint64 size);

private:
    QFile file_;
    QString errorString_;
};
