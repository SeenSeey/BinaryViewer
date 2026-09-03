#include "core/BinaryConverter.h"

#include <QStringList>

#include <algorithm>

namespace
{
QString byteToBinary(const unsigned char byte)
{
    QString result(8, QLatin1Char('0'));
    for (int bit = 7; bit >= 0; --bit) {
        if ((byte & (1U << bit)) != 0U) {
            result[7 - bit] = QLatin1Char('1');
        }
    }
    return result;
}

QString bytesToBinary(const QByteArray& bytes)
{
    QStringList parts;
    parts.reserve(bytes.size());
    for (const char value : bytes) {
        parts.append(byteToBinary(static_cast<unsigned char>(value)));
    }
    return parts.join(QLatin1Char(' '));
}
} // namespace

ConversionResult BinaryConverter::convert(
    const QByteArray& bytes,
    const ByteOrder byteOrder,
    const WordSize wordSize)
{
    ConversionResult result;
    if (bytes.isEmpty()) {
        return result;
    }

    const int wordBytes = static_cast<int>(wordSize) / 8;
    result.words.reserve((bytes.size() + wordBytes - 1) / wordBytes);

    for (int offset = 0; offset < bytes.size(); offset += wordBytes) {
        BinaryWord word;
        word.bytes = bytes.mid(offset, wordBytes);
        word.complete = word.bytes.size() == wordBytes;

        word.displayedBytes = word.bytes;
        if (word.complete && byteOrder == ByteOrder::LittleEndian) {
            std::reverse(word.displayedBytes.begin(), word.displayedBytes.end());
        }
        word.binary = bytesToBinary(word.displayedBytes);
        result.words.append(std::move(word));
    }

    return result;
}
