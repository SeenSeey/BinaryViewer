#pragma once

#include "core/Types.h"

#include <QByteArray>
#include <QString>

class BinaryTextFormatter final
{
public:
    static constexpr int MaximumPreviewBytes = 256 * 1024;

    [[nodiscard]] static QString format(
        const QByteArray& bytes,
        qint64 absoluteOffset,
        ByteOrder byteOrder,
        WordSize wordSize);
};
