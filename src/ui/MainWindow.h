#pragma once

#include "core/BinaryFileReader.h"
#include "core/Types.h"

#include <QByteArray>
#include <QMainWindow>

class QAction;
class QButtonGroup;
class QLabel;
class QPlainTextEdit;
class QSpinBox;
class HexView;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openFile();
    void previousChunk();
    void nextChunk();
    void chunkSizeChanged(int sizeInBytes);
    void conversionSettingsChanged();
    void selectionChanged(int start, int length);

private:
    static constexpr int MaxBinaryPreviewBytes = 256 * 1024;

    void createInterface();
    void applyTheme();
    [[nodiscard]] bool loadChunk(qint64 chunkIndex);
    void updateNavigation();
    void updateBinaryView();
    void updateStatusBar();
    void showFileError(const QString& message);
    [[nodiscard]] qint64 currentOffset() const;
    [[nodiscard]] ByteOrder byteOrder() const;
    [[nodiscard]] WordSize wordSize() const;
    [[nodiscard]] QString formattedFileSize(qint64 bytes) const;

    BinaryFileReader reader_;
    QByteArray currentChunk_;
    QString filePath_;
    qint64 fileSize_ = 0;
    qint64 chunkSize_ = 4096;
    qint64 chunkIndex_ = 0;
    int selectionStart_ = -1;
    int selectionLength_ = 0;

    HexView* hexView_ = nullptr;
    QPlainTextEdit* binaryView_ = nullptr;
    QLabel* statusFileValue_ = nullptr;
    QLabel* statusSizeValue_ = nullptr;
    QLabel* statusOffsetValue_ = nullptr;
    QLabel* statusChunkValue_ = nullptr;
    QLabel* statusSelectionValue_ = nullptr;
    QSpinBox* chunkSizeSpinBox_ = nullptr;
    QButtonGroup* byteOrderButtonGroup_ = nullptr;
    QButtonGroup* wordSizeButtonGroup_ = nullptr;
    QAction* previousAction_ = nullptr;
    QAction* nextAction_ = nullptr;
};
