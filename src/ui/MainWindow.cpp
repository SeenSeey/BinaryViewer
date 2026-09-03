#include "ui/MainWindow.h"

#include "core/BinaryConverter.h"
#include "ui/HexView.h"

#include <QAction>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QEasingCurve>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTextOption>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <limits>

namespace
{
constexpr int MaximumChunkSizeBytes = 4 * 1024 * 1024;

QString hexByteLabels(const QByteArray& bytes)
{
    QStringList labels;
    labels.reserve(bytes.size());
    for (const char byte : bytes) {
        const QString hex = QStringLiteral("%1")
            .arg(static_cast<unsigned char>(byte), 2, 16, QLatin1Char('0')).toUpper();
        labels.append(QStringLiteral("   %1   ").arg(hex));
    }
    return labels.join(QLatin1Char(' '));
}

class PressFeedback final : public QObject
{
public:
    explicit PressFeedback(QAbstractButton* button)
        : QObject(button)
        , effect_(new QGraphicsOpacityEffect(button))
    {
        effect_->setOpacity(1.0);
        button->setGraphicsEffect(effect_);
        button->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        bool shouldAnimate = event->type() == QEvent::MouseButtonPress;
        if (event->type() == QEvent::KeyPress) {
            const auto* keyEvent = static_cast<QKeyEvent*>(event);
            shouldAnimate = !keyEvent->isAutoRepeat()
                && (keyEvent->key() == Qt::Key_Space
                    || keyEvent->key() == Qt::Key_Return
                    || keyEvent->key() == Qt::Key_Enter);
        }
        if (shouldAnimate) {
            animate();
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void animate()
    {
        if (animation_) {
            animation_->stop();
            animation_->deleteLater();
        }
        effect_->setOpacity(1.0);
        animation_ = new QPropertyAnimation(effect_, "opacity", this);
        animation_->setDuration(160);
        animation_->setStartValue(1.0);
        animation_->setKeyValueAt(0.35, 0.70);
        animation_->setEndValue(1.0);
        animation_->setEasingCurve(QEasingCurve::OutCubic);
        animation_->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QGraphicsOpacityEffect* effect_;
    QPointer<QPropertyAnimation> animation_;
};

class NavigationMotion final : public QObject
{
public:
    NavigationMotion(QToolButton* button, const int direction, QObject* parent)
        : QObject(parent)
        , button_(button)
        , direction_(direction)
    {
    }

    void play()
    {
        if (animation_) {
            animation_->stop();
            button_->move(basePosition_);
            animation_->deleteLater();
        }

        basePosition_ = button_->pos();
        animation_ = new QPropertyAnimation(button_, "pos", this);
        animation_->setDuration(150);
        animation_->setStartValue(basePosition_);
        animation_->setKeyValueAt(
            0.45, basePosition_ + QPoint(direction_ * 4, 0));
        animation_->setEndValue(basePosition_);
        animation_->setEasingCurve(QEasingCurve::InOutQuad);
        connect(animation_, &QPropertyAnimation::finished, this, [this] {
            button_->move(basePosition_);
        });
        animation_->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    QToolButton* button_;
    int direction_;
    QPoint basePosition_;
    QPointer<QPropertyAnimation> animation_;
};

void addPressFeedback(QAbstractButton* button)
{
    button->setCursor(Qt::PointingHandCursor);
    new PressFeedback(button);
}

void addNavigationMotion(QToolButton* button, const int direction, QAction* action)
{
    auto* motion = new NavigationMotion(button, direction, button);
    QObject::connect(action, &QAction::triggered, motion, [motion] {
        motion->play();
    });
}

QWidget* createSegmentedControl(
    const QList<QPair<QString, int>>& options,
    QButtonGroup* buttonGroup,
    const int checkedId,
    QWidget* parent)
{
    auto* control = new QWidget(parent);
    control->setObjectName(QStringLiteral("segmentedControl"));
    auto* layout = new QHBoxLayout(control);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    for (const auto& option : options) {
        auto* button = new QToolButton(control);
        button->setObjectName(QStringLiteral("segmentButton"));
        button->setText(option.first);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        buttonGroup->addButton(button, option.second);
        layout->addWidget(button);
        addPressFeedback(button);
    }
    if (QAbstractButton* checkedButton = buttonGroup->button(checkedId)) {
        checkedButton->setChecked(true);
    }
    return control;
}

QWidget* createChunkSizeControl(QSpinBox* spinBox, QWidget* parent)
{
    auto* control = new QWidget(parent);
    control->setObjectName(QStringLiteral("chunkSizeControl"));
    auto* layout = new QHBoxLayout(control);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(spinBox);

    auto* decreaseButton = new QToolButton(control);
    decreaseButton->setObjectName(QStringLiteral("stepButton"));
    decreaseButton->setText(QStringLiteral("−"));
    decreaseButton->setToolTip(QObject::tr("Decrease chunk size by 1024 bytes"));
    decreaseButton->setAccessibleName(QObject::tr("Decrease chunk size"));
    auto* increaseButton = new QToolButton(control);
    increaseButton->setObjectName(QStringLiteral("stepButton"));
    increaseButton->setText(QStringLiteral("+"));
    increaseButton->setToolTip(QObject::tr("Increase chunk size by 1024 bytes"));
    increaseButton->setAccessibleName(QObject::tr("Increase chunk size"));
    layout->addWidget(decreaseButton);
    layout->addWidget(increaseButton);

    QObject::connect(decreaseButton, &QToolButton::clicked, spinBox, &QSpinBox::stepDown);
    QObject::connect(increaseButton, &QToolButton::clicked, spinBox, &QSpinBox::stepUp);
    QObject::connect(spinBox, qOverload<int>(&QSpinBox::valueChanged), control,
                     [spinBox, decreaseButton, increaseButton](const int value) {
                         decreaseButton->setEnabled(value > spinBox->minimum());
                         increaseButton->setEnabled(value < spinBox->maximum());
                     });
    decreaseButton->setEnabled(spinBox->value() > spinBox->minimum());
    increaseButton->setEnabled(spinBox->value() < spinBox->maximum());
    addPressFeedback(decreaseButton);
    addPressFeedback(increaseButton);
    return control;
}

QWidget* createToolbarGroup(const QString& caption, QWidget* control, QWidget* parent)
{
    auto* group = new QWidget(parent);
    group->setObjectName(QStringLiteral("toolbarGroup"));

    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    auto* label = new QLabel(caption.toUpper(), group);
    label->setObjectName(QStringLiteral("toolbarCaption"));
    layout->addWidget(label);
    layout->addWidget(control);
    return group;
}

class BalancedSplitter final : public QSplitter
{
public:
    using QSplitter::QSplitter;

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        const QList<int> previousSizes = sizes();
        const int widthDelta = event->oldSize().isValid()
            ? event->size().width() - event->oldSize().width()
            : 0;

        QSplitter::resizeEvent(event);

        if (widthDelta != 0 && previousSizes.size() == 2) {
            const int leftDelta = widthDelta / 2;
            setSizes({previousSizes.at(0) + leftDelta,
                      previousSizes.at(1) + widthDelta - leftDelta});
        }
    }
};
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    createInterface();
    applyTheme();
    updateNavigation();
    updateStatusBar();
    setWindowTitle(QStringLiteral("Binary Viewer"));
    resize(1180, 720);
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open binary file"));
    if (path.isEmpty()) {
        return;
    }

    if (!reader_.open(path)) {
        currentChunk_.clear();
        filePath_.clear();
        fileSize_ = 0;
        chunkIndex_ = 0;
        hexView_->setData({}, 0);
        binaryView_->clear();
        updateNavigation();
        updateStatusBar();
        showFileError(tr("Unable to open file.\n%1").arg(reader_.errorString()));
        return;
    }

    filePath_ = path;
    fileSize_ = reader_.fileSize();
    chunkIndex_ = 0;
    if (!loadChunk(0)) {
        reader_.close();
        filePath_.clear();
        fileSize_ = 0;
        currentChunk_.clear();
        hexView_->setData({}, 0);
        updateNavigation();
        updateStatusBar();
    }
}

void MainWindow::previousChunk()
{
    if (chunkIndex_ > 0) {
        (void)loadChunk(chunkIndex_ - 1);
    }
}

void MainWindow::nextChunk()
{
    if (reader_.isOpen() && currentOffset() < fileSize_
        && chunkSize_ < fileSize_ - currentOffset()) {
        (void)loadChunk(chunkIndex_ + 1);
    }
}

void MainWindow::chunkSizeChanged(const int sizeInBytes)
{
    const qint64 newChunkSize = sizeInBytes;
    if (newChunkSize == chunkSize_) {
        return;
    }
    if (!reader_.isOpen()) {
        chunkSize_ = newChunkSize;
        updateNavigation();
        return;
    }

    const qint64 oldChunkSize = chunkSize_;
    const qint64 oldChunkIndex = chunkIndex_;
    const qint64 oldOffset = currentOffset();
    chunkSize_ = newChunkSize;
    const qint64 newChunkIndex = oldOffset / chunkSize_;
    if (!loadChunk(newChunkIndex)) {
        chunkSize_ = oldChunkSize;
        chunkIndex_ = oldChunkIndex;
        const QSignalBlocker blocker(chunkSizeSpinBox_);
        chunkSizeSpinBox_->setValue(static_cast<int>(oldChunkSize));
        updateNavigation();
        updateStatusBar();
    }
}

void MainWindow::conversionSettingsChanged()
{
    updateBinaryView();
    updateStatusBar();
}

void MainWindow::selectionChanged(const int start, const int length)
{
    selectionStart_ = start;
    selectionLength_ = length;
    updateBinaryView();
    updateStatusBar();
}

void MainWindow::createInterface()
{
    auto* toolbar = addToolBar(tr("Main toolbar"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setAllowedAreas(Qt::TopToolBarArea);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolbar->setMinimumHeight(76);

    auto* brandLabel = new QLabel(QStringLiteral("✣  Binary Viewer"), toolbar);
    brandLabel->setObjectName(QStringLiteral("brandLabel"));
    toolbar->addWidget(brandLabel);

    auto* openAction = toolbar->addAction(tr("Open File"));
    openAction->setShortcut(QKeySequence::Open);
    openAction->setToolTip(tr("Open file (Ctrl+O)"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    if (auto* openButton = qobject_cast<QToolButton*>(toolbar->widgetForAction(openAction))) {
        openButton->setObjectName(QStringLiteral("primaryButton"));
        addPressFeedback(openButton);
    }

    toolbar->addSeparator();
    chunkSizeSpinBox_ = new QSpinBox(toolbar);
    chunkSizeSpinBox_->setRange(1, MaximumChunkSizeBytes);
    chunkSizeSpinBox_->setValue(4096);
    chunkSizeSpinBox_->setSuffix(tr(" bytes"));
    chunkSizeSpinBox_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    chunkSizeSpinBox_->setKeyboardTracking(false);
    chunkSizeSpinBox_->setSingleStep(1024);
    chunkSizeSpinBox_->setAccelerated(true);
    chunkSizeSpinBox_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    chunkSizeSpinBox_->setGroupSeparatorShown(true);
    chunkSizeSpinBox_->setMinimumWidth(132);
    chunkSizeSpinBox_->setToolTip(tr("Chunk size in bytes (1 to 4,194,304)"));
    toolbar->addWidget(createToolbarGroup(
        tr("Chunk size"), createChunkSizeControl(chunkSizeSpinBox_, toolbar), toolbar));

    byteOrderButtonGroup_ = new QButtonGroup(this);
    byteOrderButtonGroup_->setExclusive(true);
    auto* byteOrderControl = createSegmentedControl(
        {{tr("Little"), static_cast<int>(ByteOrder::LittleEndian)},
         {tr("Big"), static_cast<int>(ByteOrder::BigEndian)}},
        byteOrderButtonGroup_, static_cast<int>(ByteOrder::LittleEndian), toolbar);
    toolbar->addWidget(createToolbarGroup(tr("Byte order"), byteOrderControl, toolbar));

    wordSizeButtonGroup_ = new QButtonGroup(this);
    wordSizeButtonGroup_->setExclusive(true);
    auto* wordSizeControl = createSegmentedControl(
        {{QStringLiteral("16"), static_cast<int>(WordSize::Bits16)},
         {QStringLiteral("32"), static_cast<int>(WordSize::Bits32)},
         {QStringLiteral("64"), static_cast<int>(WordSize::Bits64)},
         {QStringLiteral("128"), static_cast<int>(WordSize::Bits128)}},
        wordSizeButtonGroup_, static_cast<int>(WordSize::Bits32), toolbar);
    toolbar->addWidget(createToolbarGroup(tr("Word size · bit"), wordSizeControl, toolbar));

    auto* spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    previousAction_ = toolbar->addAction(QStringLiteral("←"));
    previousAction_->setToolTip(tr("Previous chunk (PageUp)"));
    previousAction_->setShortcut(QKeySequence(Qt::Key_PageUp));
    previousAction_->setShortcutContext(Qt::WindowShortcut);
    nextAction_ = toolbar->addAction(QStringLiteral("→"));
    nextAction_->setToolTip(tr("Next chunk (PageDown)"));
    nextAction_->setShortcut(QKeySequence(Qt::Key_PageDown));
    nextAction_->setShortcutContext(Qt::WindowShortcut);
    if (auto* previousButton = qobject_cast<QToolButton*>(toolbar->widgetForAction(previousAction_))) {
        previousButton->setObjectName(QStringLiteral("navigationButton"));
        addPressFeedback(previousButton);
        addNavigationMotion(previousButton, -1, previousAction_);
    }
    if (auto* nextButton = qobject_cast<QToolButton*>(toolbar->widgetForAction(nextAction_))) {
        nextButton->setObjectName(QStringLiteral("navigationButton"));
        addPressFeedback(nextButton);
        addNavigationMotion(nextButton, 1, nextAction_);
    }

    hexView_ = new HexView(this);
    binaryView_ = new QPlainTextEdit(this);
    binaryView_->setReadOnly(true);
    binaryView_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    binaryView_->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    binaryView_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    binaryView_->setPlaceholderText(tr("Select bytes in the HEX view to see their binary representation."));

    auto* splitter = new BalancedSplitter(Qt::Horizontal, this);
    splitter->addWidget(hexView_);
    splitter->addWidget(binaryView_);
    splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({720, 480});
    setCentralWidget(splitter);

    auto* infoBar = statusBar();
    infoBar->setObjectName(QStringLiteral("infoStatusBar"));
    infoBar->setSizeGripEnabled(false);
    infoBar->setContentsMargins(0, 0, 0, 0);

    auto* statusPanel = new QWidget(infoBar);
    statusPanel->setObjectName(QStringLiteral("statusPanel"));
    auto* statusLayout = new QHBoxLayout(statusPanel);
    statusLayout->setContentsMargins(16, 6, 16, 6);
    statusLayout->setSpacing(0);

    const auto addSeparator = [statusPanel, statusLayout] {
        auto* separator = new QFrame(statusPanel);
        separator->setObjectName(QStringLiteral("statusSeparator"));
        separator->setFrameShape(QFrame::VLine);
        statusLayout->addWidget(separator);
    };
    const auto addMetric = [statusPanel, statusLayout](
                               const QString& caption, QLabel*& valueLabel) {
        auto* metric = new QWidget(statusPanel);
        metric->setObjectName(QStringLiteral("statusMetric"));
        auto* metricLayout = new QHBoxLayout(metric);
        metricLayout->setContentsMargins(0, 0, 0, 0);
        metricLayout->setSpacing(7);

        auto* captionLabel = new QLabel(caption, metric);
        captionLabel->setObjectName(QStringLiteral("statusCaption"));
        valueLabel = new QLabel(QStringLiteral("—"), metric);
        valueLabel->setObjectName(QStringLiteral("statusValue"));
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        metricLayout->addWidget(captionLabel);
        metricLayout->addWidget(valueLabel);
        statusLayout->addWidget(metric);
        return metric;
    };

    auto* fileMetric = addMetric(tr("File"), statusFileValue_);
    statusFileValue_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    fileMetric->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    addSeparator();
    addMetric(tr("Size"), statusSizeValue_);
    addSeparator();
    addMetric(tr("Offset"), statusOffsetValue_);
    statusOffsetValue_->setProperty("monospace", true);
    addSeparator();
    addMetric(tr("Chunk"), statusChunkValue_);
    addSeparator();
    addMetric(tr("Selected"), statusSelectionValue_);
    statusSelectionValue_->setProperty("monospace", true);
    statusLayout->addStretch(1);
    infoBar->addWidget(statusPanel, 1);

    connect(previousAction_, &QAction::triggered, this, &MainWindow::previousChunk);
    connect(nextAction_, &QAction::triggered, this, &MainWindow::nextChunk);
    connect(chunkSizeSpinBox_, qOverload<int>(&QSpinBox::valueChanged),
            this, &MainWindow::chunkSizeChanged);
    connect(byteOrderButtonGroup_, &QButtonGroup::idClicked,
            this, &MainWindow::conversionSettingsChanged);
    connect(wordSizeButtonGroup_, &QButtonGroup::idClicked,
            this, &MainWindow::conversionSettingsChanged);
    connect(hexView_, &HexView::byteSelectionChanged,
            this, &MainWindow::selectionChanged);
}

void MainWindow::applyTheme()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QSplitter { background: #faf9f5; color: #141413; }
        QToolBar#mainToolbar { background: #faf9f5; border: 0;
                              border-bottom: 1px solid #e6dfd8;
                              spacing: 12px; padding: 9px 16px; }
        QToolBar#mainToolbar::separator { background: #e6dfd8; width: 1px;
                                         margin: 8px 2px; }
        QLabel#brandLabel { color: #141413;
                            font-family: "Cormorant Garamond", "EB Garamond", Georgia, serif;
                            font-size: 22px; font-weight: 500;
                            padding-right: 8px; }
        QLabel#toolbarCaption { color: #8e8b82; font-size: 10px;
                                font-weight: 500; padding-left: 3px; }
        QToolButton { background: #faf9f5; color: #141413;
                      border: 1px solid #e6dfd8; border-radius: 8px;
                      padding: 9px 14px; font-weight: 500; }
        QToolButton:pressed { background: #efe9de; }
        QToolButton:disabled { color: #8e8b82; background: #f5f0e8; }
        QToolButton#primaryButton { background: #cc785c; color: #ffffff;
                                    border: 0; padding: 10px 18px; }
        QToolButton#primaryButton:pressed { background: #a9583e; }
        QToolButton#navigationButton { background: #faf9f5; color: #141413;
                                       min-width: 20px; min-height: 20px;
                                       border-radius: 19px; padding: 8px; font-size: 18px; }
        QToolButton#navigationButton:disabled { color: #b7b0a6;
                                                background: #f5f0e8; }
        QSpinBox { background: #faf9f5; color: #141413;
                   border: 1px solid #e6dfd8; border-radius: 8px;
                   padding: 5px 10px; min-height: 23px; }
        QSpinBox:focus { border: 1px solid #cc785c; background: #fffdf9; }
        QWidget#segmentedControl { background: #f5f0e8;
                                   border: 1px solid #e6dfd8; border-radius: 8px; }
        QToolButton#segmentButton { background: transparent; color: #6c6a64;
                                    border: 0; border-radius: 6px;
                                    padding: 6px 11px; min-height: 21px; }
        QToolButton#segmentButton:checked { background: #e8e0d2; color: #141413; }
        QToolButton#segmentButton:pressed { background: #efe9de; color: #141413; }
        QToolButton#stepButton { background: #faf9f5; color: #3d3d3a;
                                 border: 1px solid #e6dfd8; border-radius: 8px;
                                 min-width: 18px; min-height: 18px;
                                 padding: 7px; font-size: 16px; }
        QToolButton#stepButton:pressed { background: #e8e0d2; }
        QToolButton#stepButton:disabled { color: #b7b0a6; background: #f5f0e8; }
        QPlainTextEdit { background: #181715; color: #faf9f5; border: 0;
                         padding: 14px; selection-background-color: #cc785c; }
        QStatusBar#infoStatusBar { background: #f5f0e8; color: #3d3d3a;
                                   border-top: 1px solid #e6dfd8;
                                   min-height: 46px; }
        QStatusBar#infoStatusBar::item { border: 0; }
        QWidget#statusPanel, QWidget#statusMetric { background: transparent; }
        QLabel#statusCaption { color: #6c6a64; font-size: 12px;
                               font-weight: 500; letter-spacing: 1px; }
        QLabel#statusValue { color: #141413; font-size: 14px; font-weight: 500; }
        QLabel#statusValue[monospace="true"] {
            font-family: "JetBrains Mono", "DejaVu Sans Mono", monospace;
        }
        QFrame#statusSeparator { color: #e6dfd8; background: #e6dfd8;
                                 min-width: 1px; max-width: 1px;
                                 margin: 3px 14px; }
        QScrollBar:vertical { background: #f5f0e8; width: 12px; margin: 0; }
        QScrollBar::handle:vertical { background: #c9c1b5; min-height: 24px;
                                      border-radius: 6px; }
        QScrollBar:horizontal { background: #f5f0e8; height: 12px; margin: 0; }
        QScrollBar::handle:horizontal { background: #c9c1b5; min-width: 24px;
                                        border-radius: 6px; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
    )"));

    QPalette hexPalette = hexView_->palette();
    hexPalette.setColor(QPalette::Base, QColor(QStringLiteral("#faf9f5")));
    hexPalette.setColor(QPalette::Text, QColor(QStringLiteral("#141413")));
    hexPalette.setColor(QPalette::Highlight, QColor(QStringLiteral("#cc785c")));
    hexPalette.setColor(QPalette::HighlightedText, Qt::white);
    hexView_->setPalette(hexPalette);
}

bool MainWindow::loadChunk(const qint64 chunkIndex)
{
    if (!reader_.isOpen() || chunkIndex < 0
        || (chunkIndex > 0 && chunkIndex > std::numeric_limits<qint64>::max() / chunkSize_)) {
        return false;
    }
    const qint64 offset = chunkIndex * chunkSize_;
    const auto data = reader_.readChunk(offset, chunkSize_);
    if (!data.has_value()) {
        showFileError(tr("Unable to read file chunk.\n%1").arg(reader_.errorString()));
        return false;
    }

    chunkIndex_ = chunkIndex;
    currentChunk_ = *data;
    selectionStart_ = -1;
    selectionLength_ = 0;
    hexView_->setData(currentChunk_, offset);
    binaryView_->clear();
    updateNavigation();
    updateStatusBar();
    return true;
}

void MainWindow::updateNavigation()
{
    const bool open = reader_.isOpen();
    previousAction_->setEnabled(open && chunkIndex_ > 0);
    const bool hasNext = open && currentOffset() < fileSize_
        && chunkSize_ < fileSize_ - currentOffset();
    nextAction_->setEnabled(hasNext);
}

void MainWindow::updateBinaryView()
{
    if (selectionStart_ < 0 || selectionLength_ <= 0) {
        binaryView_->clear();
        return;
    }

    const qint64 absoluteOffset = currentOffset() + selectionStart_;
    const int wordBits = static_cast<int>(wordSize());
    const QString offsetText = QStringLiteral("%1")
        .arg(static_cast<qulonglong>(absoluteOffset), 16, 16, QLatin1Char('0')).toUpper();
    QStringList lines;
    lines << tr("Offset: 0x%1").arg(offsetText)
          << tr("Length: %1 bytes").arg(selectionLength_)
          << tr("Word: %1 bit").arg(wordBits)
          << tr("Endian: %1").arg(byteOrder() == ByteOrder::LittleEndian
                                       ? tr("Little Endian") : tr("Big Endian"))
          << QString();

    if (selectionLength_ > MaxBinaryPreviewBytes) {
        lines << tr("Binary preview is limited to %1 KB.")
                     .arg(MaxBinaryPreviewBytes / 1024)
              << tr("Reduce the selection to display binary data.");
        binaryView_->setPlainText(lines.join(QLatin1Char('\n')));
        return;
    }

    const QByteArray selected = currentChunk_.mid(selectionStart_, selectionLength_);
    const ConversionResult converted = BinaryConverter::convert(selected, byteOrder(), wordSize());
    int completeIndex = 0;
    for (const BinaryWord& word : converted.words) {
        QString prefix;
        if (word.complete) {
            prefix = QStringLiteral("[%1] ").arg(completeIndex++);
        } else {
            prefix = tr("Partial word (%1/%2 bit): ")
                         .arg(word.bytes.size() * 8).arg(wordBits);
        }
        lines << QString(prefix.size(), QLatin1Char(' ')) + hexByteLabels(word.displayedBytes)
              << prefix + word.binary;
    }
    binaryView_->setPlainText(lines.join(QLatin1Char('\n')));
}

void MainWindow::updateStatusBar()
{
    if (!reader_.isOpen()) {
        statusFileValue_->setText(tr("No file opened"));
        statusFileValue_->setToolTip({});
        statusSizeValue_->setText(QStringLiteral("—"));
        statusOffsetValue_->setText(QStringLiteral("—"));
        statusChunkValue_->setText(QStringLiteral("—"));
        statusSelectionValue_->setText(QStringLiteral("—"));
        return;
    }

    const QString offsetText = QStringLiteral("%1")
        .arg(static_cast<qulonglong>(currentOffset()), 16, 16, QLatin1Char('0')).toUpper();
    statusFileValue_->setText(QFileInfo(filePath_).fileName());
    statusFileValue_->setToolTip(QFileInfo(filePath_).absoluteFilePath());
    statusSizeValue_->setText(formattedFileSize(fileSize_));
    statusOffsetValue_->setText(QStringLiteral("0x%1").arg(offsetText));
    if (fileSize_ > 0) {
        const qint64 totalChunks = fileSize_ / chunkSize_ + (fileSize_ % chunkSize_ != 0 ? 1 : 0);
        statusChunkValue_->setText(tr("%1 / %2").arg(chunkIndex_ + 1).arg(totalChunks));
    } else {
        statusChunkValue_->setText(QStringLiteral("—"));
    }
    if (selectionStart_ >= 0 && selectionLength_ > 0) {
        const QString selectionOffset = QStringLiteral("%1")
            .arg(static_cast<qulonglong>(currentOffset() + selectionStart_),
                 16, 16, QLatin1Char('0')).toUpper();
        statusSelectionValue_->setText(tr("%1 bytes @ 0x%2")
                                           .arg(selectionLength_).arg(selectionOffset));
    } else {
        statusSelectionValue_->setText(QStringLiteral("—"));
    }
}

void MainWindow::showFileError(const QString& message)
{
    QMessageBox::critical(this, tr("File error"), message);
}

qint64 MainWindow::currentOffset() const
{
    if (chunkIndex_ > 0 && chunkIndex_ > std::numeric_limits<qint64>::max() / chunkSize_) {
        return std::numeric_limits<qint64>::max();
    }
    return chunkIndex_ * chunkSize_;
}

ByteOrder MainWindow::byteOrder() const
{
    return static_cast<ByteOrder>(byteOrderButtonGroup_->checkedId());
}

WordSize MainWindow::wordSize() const
{
    return static_cast<WordSize>(wordSizeButtonGroup_->checkedId());
}

QString MainWindow::formattedFileSize(const qint64 bytes) const
{
    if (bytes < 1024) {
        return tr("%1 bytes").arg(bytes);
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
