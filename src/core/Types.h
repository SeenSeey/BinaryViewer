#pragma once

#include <QByteArray>
#include <QString>

enum class ByteOrder
{
    LittleEndian,
    BigEndian
};

enum class WordSize
{
    Bits16 = 16,
    Bits32 = 32,
    Bits64 = 64,
    Bits128 = 128
};

struct BinaryWord
{
    QByteArray displayedBytes;
    QString binary;
    bool complete = false;
};
