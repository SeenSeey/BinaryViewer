#include "presentation/BinaryTextFormatter.h"

#include <QtTest>

class BinaryTextFormatterTests final : public QObject
{
    Q_OBJECT

private slots:
    void formatsCompleteAndPartialWords();
    void limitsLargePreview();
    void emptyInput();
};

void BinaryTextFormatterTests::formatsCompleteAndPartialWords()
{
    const QString text = BinaryTextFormatter::format(
        QByteArray::fromHex("123456"), 0x20, ByteOrder::LittleEndian, WordSize::Bits16);

    QVERIFY(text.contains(QStringLiteral("Offset: 0x0000000000000020")));
    QVERIFY(text.contains(QStringLiteral("Length: 3 bytes")));
    QVERIFY(text.contains(QStringLiteral("[0] 00110100 00010010")));
    QVERIFY(text.contains(QStringLiteral("Partial word (8/16 bit): 01010110")));
}

void BinaryTextFormatterTests::limitsLargePreview()
{
    const QByteArray bytes(BinaryTextFormatter::MaximumPreviewBytes + 1, '\0');
    const QString text = BinaryTextFormatter::format(
        bytes, 0, ByteOrder::BigEndian, WordSize::Bits32);

    QVERIFY(text.contains(QStringLiteral("Binary preview is limited")));
    QVERIFY(!text.contains(QStringLiteral("[0]")));
}

void BinaryTextFormatterTests::emptyInput()
{
    QVERIFY(BinaryTextFormatter::format(
        {}, 0, ByteOrder::BigEndian, WordSize::Bits32).isEmpty());
}

QTEST_APPLESS_MAIN(BinaryTextFormatterTests)
#include "BinaryTextFormatterTests.moc"
