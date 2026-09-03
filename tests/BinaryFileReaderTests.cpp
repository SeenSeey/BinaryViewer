#include "core/BinaryFileReader.h"

#include <QDir>
#include <QTemporaryFile>
#include <QtTest>

namespace
{
QString temporaryFileTemplate()
{
    return QDir::tempPath() + QStringLiteral("/BinaryViewer-XXXXXX.bin");
}

QString readableTemporaryFilePath(const QTemporaryFile& file)
{
    if (!file.fileName().isEmpty()) {
        return file.fileName();
    }
#ifdef Q_OS_LINUX
    // Some restricted filesystems leave QTemporaryFile as an unnamed O_TMPFILE.
    return QStringLiteral("/proc/self/fd/%1").arg(file.handle());
#else
    return {};
#endif
}
} // namespace

class BinaryFileReaderTests final : public QObject
{
    Q_OBJECT

private slots:
    void chunksAndEof();
    void emptyFile();
    void invalidRanges();
    void missingFile();
};

void BinaryFileReaderTests::chunksAndEof()
{
    QTemporaryFile file(temporaryFileTemplate());
    QVERIFY(file.open());
    QCOMPARE(file.write(QByteArray::fromHex("00010203040506070809")), qint64(10));
    QVERIFY(file.flush());

    BinaryFileReader reader;
    QVERIFY(reader.open(readableTemporaryFilePath(file)));
    QCOMPARE(reader.fileSize(), qint64(10));

    const auto first = reader.readChunk(0, 4);
    QVERIFY(first.has_value());
    QCOMPARE(*first, QByteArray::fromHex("00010203"));

    const auto middle = reader.readChunk(4, 4);
    QVERIFY(middle.has_value());
    QCOMPARE(*middle, QByteArray::fromHex("04050607"));

    const auto last = reader.readChunk(8, 4);
    QVERIFY(last.has_value());
    QCOMPARE(*last, QByteArray::fromHex("0809"));

    const auto beyond = reader.readChunk(100, 4);
    QVERIFY(beyond.has_value());
    QVERIFY(beyond->isEmpty());
}

void BinaryFileReaderTests::emptyFile()
{
    QTemporaryFile file(temporaryFileTemplate());
    QVERIFY(file.open());
    QVERIFY(file.flush());

    BinaryFileReader reader;
    QVERIFY(reader.open(readableTemporaryFilePath(file)));
    QCOMPARE(reader.fileSize(), qint64(0));
    const auto data = reader.readChunk(0, 1024);
    QVERIFY(data.has_value());
    QVERIFY(data->isEmpty());
}

void BinaryFileReaderTests::invalidRanges()
{
    QTemporaryFile file(temporaryFileTemplate());
    QVERIFY(file.open());

    BinaryFileReader reader;
    QVERIFY(reader.open(readableTemporaryFilePath(file)));
    QVERIFY(!reader.readChunk(-1, 1).has_value());
    QVERIFY(!reader.readChunk(0, -1).has_value());
}

void BinaryFileReaderTests::missingFile()
{
    BinaryFileReader reader;
    QVERIFY(!reader.open(QStringLiteral("/path/that/does/not/exist.bin")));
    QVERIFY(!reader.errorString().isEmpty());
}

QTEST_GUILESS_MAIN(BinaryFileReaderTests)
#include "BinaryFileReaderTests.moc"
