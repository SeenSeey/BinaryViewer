#include "ui/MainWindow.h"

#include "presentation/BinaryTextFormatter.h"
#include "ui/HexView.h"
#include "ui/MainWindowUi.h"

#include <QAction>
#include <QButtonGroup>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSpinBox>

namespace
{
QString formattedFileSize(const qint64 bytes)
{
    if (bytes < 1024) {
        return QCoreApplication::translate("MainWindow", "%1 bytes").arg(bytes);
    }

    static const char* const units[] = {"KB", "MB", "GB", "TB", "PB"};
    double value = static_cast<double>(bytes);
    int unit = -1;
    do {
        value /= 1024.0;
        ++unit;
    } while (value >= 1024.0 && unit < 4);
    return QStringLiteral("%1 %2")
        .arg(QLocale().toString(value, 'f', value < 10.0 ? 1 : 0),
             QString::fromLatin1(units[unit]));
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(std::make_unique<MainWindowUi>())
{
    ui_->setup(this);
    ui_->applyTheme(this);

    connect(ui_->openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui_->previousAction, &QAction::triggered, this, &MainWindow::previousChunk);
    connect(ui_->nextAction, &QAction::triggered, this, &MainWindow::nextChunk);
    connect(ui_->chunkSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged),
            this, &MainWindow::chunkSizeChanged);
    connect(ui_->byteOrderButtonGroup, &QButtonGroup::idClicked,
            this, &MainWindow::conversionSettingsChanged);
    connect(ui_->wordSizeButtonGroup, &QButtonGroup::idClicked,
            this, &MainWindow::conversionSettingsChanged);
    connect(ui_->hexView, &HexView::byteSelectionChanged,
            this, &MainWindow::selectionChanged);

    updateNavigation();
    updateStatusBar();
    setWindowTitle(QStringLiteral("Binary Viewer"));
    resize(1180, 720);
}

MainWindow::~MainWindow() = default;

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open binary file"));
    if (path.isEmpty()) {
        return;
    }

    if (!session_.open(path)) {
        const QString error = session_.errorString();
        ui_->hexView->setData({}, 0);
        ui_->binaryView->clear();
        updateNavigation();
        updateStatusBar();
        showFileError(tr("Unable to open file.\n%1").arg(error));
        return;
    }

    showCurrentChunk();
}

void MainWindow::previousChunk()
{
    if (!session_.canLoadPreviousChunk()) {
        return;
    }
    if (!session_.loadPreviousChunk()) {
        showFileError(tr("Unable to read file chunk.\n%1").arg(session_.errorString()));
        return;
    }
    showCurrentChunk();
}

void MainWindow::nextChunk()
{
    if (!session_.canLoadNextChunk()) {
        return;
    }
    if (!session_.loadNextChunk()) {
        showFileError(tr("Unable to read file chunk.\n%1").arg(session_.errorString()));
        return;
    }
    showCurrentChunk();
}

void MainWindow::chunkSizeChanged(const int sizeInBytes)
{
    const qint64 previousChunkSize = session_.chunkSize();
    if (!session_.setChunkSize(sizeInBytes)) {
        const QSignalBlocker blocker(ui_->chunkSizeSpinBox);
        ui_->chunkSizeSpinBox->setValue(static_cast<int>(previousChunkSize));
        updateNavigation();
        updateStatusBar();
        showFileError(tr("Unable to read file chunk.\n%1").arg(session_.errorString()));
        return;
    }

    if (session_.isOpen()) {
        showCurrentChunk();
    } else {
        updateNavigation();
    }
}

void MainWindow::conversionSettingsChanged()
{
    updateBinaryView();
    updateStatusBar();
}

void MainWindow::selectionChanged(const int start, const int length)
{
    session_.setSelection(start, length);
    updateBinaryView();
    updateStatusBar();
}

void MainWindow::showCurrentChunk()
{
    ui_->hexView->setData(session_.currentChunk(), session_.currentOffset());
    ui_->binaryView->clear();
    updateNavigation();
    updateStatusBar();
}

void MainWindow::updateNavigation()
{
    ui_->previousAction->setEnabled(session_.canLoadPreviousChunk());
    ui_->nextAction->setEnabled(session_.canLoadNextChunk());
}

void MainWindow::updateBinaryView()
{
    if (session_.selectionStart() < 0 || session_.selectionLength() <= 0) {
        ui_->binaryView->clear();
        return;
    }

    ui_->binaryView->setPlainText(BinaryTextFormatter::format(
        session_.selectedBytes(),
        session_.currentOffset() + session_.selectionStart(),
        ui_->byteOrder(),
        ui_->wordSize()));
}

void MainWindow::updateStatusBar()
{
    if (!session_.isOpen()) {
        ui_->statusFileValue->setText(tr("No file opened"));
        ui_->statusFileValue->setToolTip({});
        ui_->statusSizeValue->setText(QStringLiteral("—"));
        ui_->statusOffsetValue->setText(QStringLiteral("—"));
        ui_->statusChunkValue->setText(QStringLiteral("—"));
        ui_->statusSelectionValue->setText(QStringLiteral("—"));
        return;
    }

    const QString offsetText = QStringLiteral("%1")
        .arg(static_cast<qulonglong>(session_.currentOffset()),
             16, 16, QLatin1Char('0')).toUpper();
    ui_->statusFileValue->setText(QFileInfo(session_.filePath()).fileName());
    ui_->statusFileValue->setToolTip(QFileInfo(session_.filePath()).absoluteFilePath());
    ui_->statusSizeValue->setText(formattedFileSize(session_.fileSize()));
    ui_->statusOffsetValue->setText(QStringLiteral("0x%1").arg(offsetText));
    if (session_.totalChunks() > 0) {
        ui_->statusChunkValue->setText(
            tr("%1 / %2").arg(session_.chunkIndex() + 1).arg(session_.totalChunks()));
    } else {
        ui_->statusChunkValue->setText(QStringLiteral("—"));
    }

    if (session_.selectionStart() >= 0 && session_.selectionLength() > 0) {
        const QString selectionOffset = QStringLiteral("%1")
            .arg(static_cast<qulonglong>(
                     session_.currentOffset() + session_.selectionStart()),
                 16, 16, QLatin1Char('0')).toUpper();
        ui_->statusSelectionValue->setText(
            tr("%1 bytes @ 0x%2").arg(session_.selectionLength()).arg(selectionOffset));
    } else {
        ui_->statusSelectionValue->setText(QStringLiteral("—"));
    }
}

void MainWindow::showFileError(const QString& message)
{
    QMessageBox::critical(this, tr("File error"), message);
}
