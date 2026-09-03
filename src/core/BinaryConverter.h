#pragma once

#include "core/Types.h"

class BinaryConverter
{
public:
    [[nodiscard]] static ConversionResult convert(
        const QByteArray& bytes,
        ByteOrder byteOrder,
        WordSize wordSize);
};
