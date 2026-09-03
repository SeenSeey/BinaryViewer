#include "core/ViewerSession.h"

#include <QDir>
#include <QTemporaryFile>
#include <QtTest>

namespace
{
QString temporaryFileTemplate()
{
    return QDir::tempPath() + QStringLiteral("/binary-viewer-session-XXXXXX.bin");
}

QString readableTemporaryFilePath(QTemporaryFile& file)
{
    if (!file.fileName().isEmpty()) {
        return file.fileName();
    }
#ifdef Q_OS_LINUX
    return QStringLiteral("/proc/self/fd/%1").arg(file.handle());
#else
    return {};
#endif
}
} // namespace

class ViewerSessionTests final : public QObject
{
    Q_OBJECT

private slots:
    void navigationAndSelection();
    void chunkResizeKeepsCurrentRegion();
    void invalidState();
};

void ViewerSessionTests::navigationAndSelection()
{
    QTemporaryFile file(temporaryFileTemplate());
    QVERIFY(file.open());
    QCOMPARE(file.write(QByteArray::fromHex("00010203040506070809")), qint64(10));
    QVERIFY(file.flush());

    ViewerSession session;
    QVERIFY(session.setChunkSize(4));
    QVERIFY(session.open(readableTemporaryFilePath(file)));
    QCOMPARE(session.fileSize(), qint64(10));
    QCOMPARE(session.currentChunk(), QByteArray::fromHex("00010203"));
    QCOMPARE(session.totalChunks(), qint64(3));
    QVERIFY(!session.canLoadPreviousChunk());
    QVERIFY(session.canLoadNextChunk());

    session.setSelection(1, 2);
    QCOMPARE(session.selectedBytes(), QByteArray::fromHex("0102"));
    QVERIFY(session.loadNextChunk());
    QCOMPARE(session.currentOffset(), qint64(4));
    QCOMPARE(session.currentChunk(), QByteArray::fromHex("04050607"));
    QCOMPARE(session.selectionStart(), -1);

    QVERIFY(session.loadNextChunk());
    QCOMPARE(session.currentChunk(), QByteArray::fromHex("0809"));
    QVERIFY(!session.canLoadNextChunk());
    QVERIFY(session.loadPreviousChunk());
    QCOMPARE(session.currentOffset(), qint64(4));
}

void ViewerSessionTests::chunkResizeKeepsCurrentRegion()
{
    QTemporaryFile file(temporaryFileTemplate());
    QVERIFY(file.open());
    QCOMPARE(file.write(QByteArray::fromHex("000102030405060708090a0b")), qint64(12));
    QVERIFY(file.flush());

    ViewerSession session;
    QVERIFY(session.setChunkSize(4));
    QVERIFY(session.open(readableTemporaryFilePath(file)));
    QVERIFY(session.loadNextChunk());
    QCOMPARE(session.currentOffset(), qint64(4));

    QVERIFY(session.setChunkSize(3));
    QCOMPARE(session.chunkSize(), qint64(3));
    QCOMPARE(session.currentOffset(), qint64(3));
    QCOMPARE(session.currentChunk(), QByteArray::fromHex("030405"));
}

void ViewerSessionTests::invalidState()
{
    ViewerSession session;
    QVERIFY(!session.setChunkSize(0));
    QVERIFY(!session.errorString().isEmpty());
    QVERIFY(!session.open(QStringLiteral("/path/that/does/not/exist.bin")));
    QVERIFY(!session.isOpen());
    QVERIFY(session.currentChunk().isEmpty());
    QVERIFY(!session.errorString().isEmpty());
}

QTEST_GUILESS_MAIN(ViewerSessionTests)
#include "ViewerSessionTests.moc"
