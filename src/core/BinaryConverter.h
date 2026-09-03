#pragma once

#include "core/Types.h"

#include <QList>

class BinaryConverter
{
public:
    [[nodiscard]] static QList<BinaryWord> convert(
        const QByteArray& bytes,
        ByteOrder byteOrder,
        WordSize wordSize);
};
