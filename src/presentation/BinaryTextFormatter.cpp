#include "presentation/BinaryTextFormatter.h"

#include "core/BinaryConverter.h"

#include <QCoreApplication>
#include <QStringList>

namespace
{
QString translated(const char* text)
{
    return QCoreApplication::translate("BinaryTextFormatter", text);
}

QString hexByteLabels(const QByteArray& bytes)
{
    QStringList labels;
    labels.reserve(bytes.size());
    for (const char byte : bytes) {
        const QString hex = QStringLiteral("%1")
            .arg(static_cast<unsigned char>(byte), 2, 16, QLatin1Char('0')).toUpper();
        labels.append(QStringLiteral("   %1   ").arg(hex));
    }
    return labels.join(QLatin1Char(' '));
}
} // namespace

QString BinaryTextFormatter::format(
    const QByteArray& bytes,
    const qint64 absoluteOffset,
    const ByteOrder byteOrder,
    const WordSize wordSize)
{
    if (bytes.isEmpty()) {
        return {};
    }

    const int wordBits = static_cast<int>(wordSize);
    const QString offsetText = QStringLiteral("%1")
        .arg(static_cast<qulonglong>(absoluteOffset), 16, 16, QLatin1Char('0')).toUpper();
    QStringList lines;
    lines << translated("Offset: 0x%1").arg(offsetText)
          << translated("Length: %1 bytes").arg(bytes.size())
          << translated("Word: %1 bit").arg(wordBits)
          << translated("Endian: %1").arg(byteOrder == ByteOrder::LittleEndian
                                                ? translated("Little Endian")
                                                : translated("Big Endian"))
          << QString();

    if (bytes.size() > MaximumPreviewBytes) {
        lines << translated("Binary preview is limited to %1 KB.")
                     .arg(MaximumPreviewBytes / 1024)
              << translated("Reduce the selection to display binary data.");
        return lines.join(QLatin1Char('\n'));
    }

    const QList<BinaryWord> words = BinaryConverter::convert(bytes, byteOrder, wordSize);
    int completeIndex = 0;
    for (const BinaryWord& word : words) {
        QString prefix;
        if (word.complete) {
            prefix = QStringLiteral("[%1] ").arg(completeIndex++);
        } else {
            prefix = translated("Partial word (%1/%2 bit): ")
                         .arg(word.displayedBytes.size() * 8).arg(wordBits);
        }
        lines << QString(prefix.size(), QLatin1Char(' ')) + hexByteLabels(word.displayedBytes)
              << prefix + word.binary;
    }
    return lines.join(QLatin1Char('\n'));
}
