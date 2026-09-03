#include "core/BinaryFileReader.h"

bool BinaryFileReader::open(const QString& path)
{
    close();
    file_.setFileName(path);
    if (!file_.open(QIODevice::ReadOnly)) {
        errorString_ = file_.errorString();
        return false;
    }
    errorString_.clear();
    return true;
}

void BinaryFileReader::close()
{
    if (file_.isOpen()) {
        file_.close();
    }
    file_.setFileName(QString());
    errorString_.clear();
}

bool BinaryFileReader::isOpen() const
{
    return file_.isOpen();
}

qint64 BinaryFileReader::fileSize() const
{
    return file_.isOpen() ? file_.size() : 0;
}

QString BinaryFileReader::errorString() const
{
    return errorString_;
}

std::optional<QByteArray> BinaryFileReader::readChunk(const qint64 offset, const qint64 size)
{
    if (!file_.isOpen()) {
        errorString_ = QStringLiteral("No file is open.");
        return std::nullopt;
    }
    if (offset < 0 || size < 0) {
        errorString_ = QStringLiteral("Invalid chunk range.");
        return std::nullopt;
    }
    if (offset >= file_.size()) {
        errorString_.clear();
        return QByteArray();
    }
    if (!file_.seek(offset)) {
        errorString_ = file_.errorString();
        return std::nullopt;
    }

    QByteArray data = file_.read(size);
    if (data.isNull() && file_.error() != QFileDevice::NoError) {
        errorString_ = file_.errorString();
        return std::nullopt;
    }

    errorString_.clear();
    return data;
}
