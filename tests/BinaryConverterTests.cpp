#include "core/BinaryConverter.h"

#include <QtTest>

#include <algorithm>

class BinaryConverterTests final : public QObject
{
    Q_OBJECT

private slots:
    void completeWords_data();
    void completeWords();
    void partialWordIsNotReversed();
    void endianIsAppliedPerWord();
    void emptySelection();
};

void BinaryConverterTests::completeWords_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<int>("wordSize");
    QTest::addColumn<int>("byteOrder");
    QTest::addColumn<QString>("expected");

    QTest::newRow("16-bit big")
        << QByteArray::fromHex("1234") << 16 << static_cast<int>(ByteOrder::BigEndian)
        << QStringLiteral("00010010 00110100");
    QTest::newRow("16-bit little")
        << QByteArray::fromHex("1234") << 16 << static_cast<int>(ByteOrder::LittleEndian)
        << QStringLiteral("00110100 00010010");
    QTest::newRow("32-bit big")
        << QByteArray::fromHex("12345678") << 32 << static_cast<int>(ByteOrder::BigEndian)
        << QStringLiteral("00010010 00110100 01010110 01111000");
    QTest::newRow("32-bit little")
        << QByteArray::fromHex("12345678") << 32 << static_cast<int>(ByteOrder::LittleEndian)
        << QStringLiteral("01111000 01010110 00110100 00010010");
    QTest::newRow("64-bit little")
        << QByteArray::fromHex("0102030405060708") << 64 << static_cast<int>(ByteOrder::LittleEndian)
        << QStringLiteral("00001000 00000111 00000110 00000101 00000100 00000011 00000010 00000001");
    QTest::newRow("128-bit big")
        << QByteArray::fromHex("000102030405060708090a0b0c0d0e0f") << 128
        << static_cast<int>(ByteOrder::BigEndian)
        << QStringLiteral("00000000 00000001 00000010 00000011 00000100 00000101 00000110 00000111 "
                          "00001000 00001001 00001010 00001011 00001100 00001101 00001110 00001111");
}

void BinaryConverterTests::completeWords()
{
    QFETCH(QByteArray, input);
    QFETCH(int, wordSize);
    QFETCH(int, byteOrder);
    QFETCH(QString, expected);

    const auto result = BinaryConverter::convert(
        input, static_cast<ByteOrder>(byteOrder), static_cast<WordSize>(wordSize));
    QCOMPARE(result.words.size(), 1);
    QVERIFY(result.words.first().complete);
    QCOMPARE(result.words.first().binary, expected);
    QByteArray expectedDisplayedBytes = input;
    if (static_cast<ByteOrder>(byteOrder) == ByteOrder::LittleEndian) {
        std::reverse(expectedDisplayedBytes.begin(), expectedDisplayedBytes.end());
    }
    QCOMPARE(result.words.first().displayedBytes, expectedDisplayedBytes);
}

void BinaryConverterTests::partialWordIsNotReversed()
{
    const auto result = BinaryConverter::convert(
        QByteArray::fromHex("123456"), ByteOrder::LittleEndian, WordSize::Bits16);
    QCOMPARE(result.words.size(), 2);
    QCOMPARE(result.words.at(0).binary, QStringLiteral("00110100 00010010"));
    QVERIFY(result.words.at(0).complete);
    QCOMPARE(result.words.at(1).binary, QStringLiteral("01010110"));
    QCOMPARE(result.words.at(1).displayedBytes, QByteArray::fromHex("56"));
    QVERIFY(!result.words.at(1).complete);
}

void BinaryConverterTests::endianIsAppliedPerWord()
{
    const auto result = BinaryConverter::convert(
        QByteArray::fromHex("1234aabb"), ByteOrder::LittleEndian, WordSize::Bits16);
    QCOMPARE(result.words.size(), 2);
    QCOMPARE(result.words.at(0).binary, QStringLiteral("00110100 00010010"));
    QCOMPARE(result.words.at(1).binary, QStringLiteral("10111011 10101010"));
}

void BinaryConverterTests::emptySelection()
{
    const auto result = BinaryConverter::convert({}, ByteOrder::BigEndian, WordSize::Bits32);
    QVERIFY(result.words.isEmpty());
}

QTEST_APPLESS_MAIN(BinaryConverterTests)
#include "BinaryConverterTests.moc"
