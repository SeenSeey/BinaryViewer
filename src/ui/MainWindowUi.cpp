#include "ui/MainWindowUi.h"

#include "core/ViewerSession.h"
#include "ui/HexView.h"
#include "ui/UiEffects.h"

#include <QButtonGroup>
#include <QCoreApplication>
#include <QFile>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QPalette>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTextOption>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
constexpr int MaximumChunkSizeBytes = 4 * 1024 * 1024;

QString translated(const char* text)
{
    return QCoreApplication::translate("MainWindow", text);
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
        buttonGroup->addButton(button, option.second);
        layout->addWidget(button);
        UiEffects::addPressFeedback(button);
        if (option.second == checkedId) {
            button->setChecked(true);
        }
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
    decreaseButton->setToolTip(translated("Decrease chunk size by 1024 bytes"));
    decreaseButton->setAccessibleName(translated("Decrease chunk size"));
    auto* increaseButton = new QToolButton(control);
    increaseButton->setObjectName(QStringLiteral("stepButton"));
    increaseButton->setText(QStringLiteral("+"));
    increaseButton->setToolTip(translated("Increase chunk size by 1024 bytes"));
    increaseButton->setAccessibleName(translated("Increase chunk size"));
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
    UiEffects::addPressFeedback(decreaseButton);
    UiEffects::addPressFeedback(increaseButton);
    return control;
}

QWidget* createToolbarGroup(const QString& caption, QWidget* control)
{
    auto* group = new QWidget(control->parentWidget());
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

void MainWindowUi::setup(QMainWindow* window)
{
    auto* toolbar = window->addToolBar(translated("Main toolbar"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setAllowedAreas(Qt::TopToolBarArea);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolbar->setMinimumHeight(76);

    auto* brandLabel = new QLabel(QStringLiteral("✣  Binary Viewer"), toolbar);
    brandLabel->setObjectName(QStringLiteral("brandLabel"));
    toolbar->addWidget(brandLabel);

    openAction = toolbar->addAction(translated("Open File"));
    openAction->setShortcut(QKeySequence::Open);
    openAction->setToolTip(translated("Open file (Ctrl+O)"));
    if (auto* openButton = qobject_cast<QToolButton*>(toolbar->widgetForAction(openAction))) {
        openButton->setObjectName(QStringLiteral("primaryButton"));
        UiEffects::addPressFeedback(openButton);
    }

    toolbar->addSeparator();
    chunkSizeSpinBox = new QSpinBox(toolbar);
    chunkSizeSpinBox->setRange(1, MaximumChunkSizeBytes);
    chunkSizeSpinBox->setValue(static_cast<int>(ViewerSession::DefaultChunkSize));
    chunkSizeSpinBox->setSuffix(translated(" bytes"));
    chunkSizeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    chunkSizeSpinBox->setKeyboardTracking(false);
    chunkSizeSpinBox->setSingleStep(1024);
    chunkSizeSpinBox->setAccelerated(true);
    chunkSizeSpinBox->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    chunkSizeSpinBox->setGroupSeparatorShown(true);
    chunkSizeSpinBox->setMinimumWidth(132);
    chunkSizeSpinBox->setToolTip(translated("Chunk size in bytes (1 to 4,194,304)"));
    toolbar->addWidget(createToolbarGroup(
        translated("Chunk size"), createChunkSizeControl(chunkSizeSpinBox, toolbar)));

    byteOrderButtonGroup = new QButtonGroup(window);
    byteOrderButtonGroup->setExclusive(true);
    auto* byteOrderControl = createSegmentedControl(
        {{translated("Little"), static_cast<int>(ByteOrder::LittleEndian)},
         {translated("Big"), static_cast<int>(ByteOrder::BigEndian)}},
        byteOrderButtonGroup, static_cast<int>(ByteOrder::LittleEndian), toolbar);
    toolbar->addWidget(createToolbarGroup(translated("Byte order"), byteOrderControl));

    wordSizeButtonGroup = new QButtonGroup(window);
    wordSizeButtonGroup->setExclusive(true);
    auto* wordSizeControl = createSegmentedControl(
        {{QStringLiteral("16"), static_cast<int>(WordSize::Bits16)},
         {QStringLiteral("32"), static_cast<int>(WordSize::Bits32)},
         {QStringLiteral("64"), static_cast<int>(WordSize::Bits64)},
         {QStringLiteral("128"), static_cast<int>(WordSize::Bits128)}},
        wordSizeButtonGroup, static_cast<int>(WordSize::Bits32), toolbar);
    toolbar->addWidget(createToolbarGroup(translated("Word size · bit"), wordSizeControl));

    auto* spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    previousAction = toolbar->addAction(QStringLiteral("←"));
    previousAction->setToolTip(translated("Previous chunk (PageUp)"));
    previousAction->setShortcut(QKeySequence(Qt::Key_PageUp));
    previousAction->setShortcutContext(Qt::WindowShortcut);
    nextAction = toolbar->addAction(QStringLiteral("→"));
    nextAction->setToolTip(translated("Next chunk (PageDown)"));
    nextAction->setShortcut(QKeySequence(Qt::Key_PageDown));
    nextAction->setShortcutContext(Qt::WindowShortcut);
    if (auto* button = qobject_cast<QToolButton*>(toolbar->widgetForAction(previousAction))) {
        button->setObjectName(QStringLiteral("navigationButton"));
        UiEffects::addPressFeedback(button);
        UiEffects::addNavigationMotion(button, -1, previousAction);
    }
    if (auto* button = qobject_cast<QToolButton*>(toolbar->widgetForAction(nextAction))) {
        button->setObjectName(QStringLiteral("navigationButton"));
        UiEffects::addPressFeedback(button);
        UiEffects::addNavigationMotion(button, 1, nextAction);
    }

    hexView = new HexView(window);
    binaryView = new QPlainTextEdit(window);
    binaryView->setReadOnly(true);
    binaryView->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    binaryView->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    binaryView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    binaryView->setPlaceholderText(
        translated("Select bytes in the HEX view to see their binary representation."));

    auto* splitter = new BalancedSplitter(Qt::Horizontal, window);
    splitter->addWidget(hexView);
    splitter->addWidget(binaryView);
    splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({720, 480});
    window->setCentralWidget(splitter);

    auto* infoBar = window->statusBar();
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

    auto* fileMetric = addMetric(translated("File"), statusFileValue);
    statusFileValue->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    fileMetric->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    addSeparator();
    addMetric(translated("Size"), statusSizeValue);
    addSeparator();
    addMetric(translated("Offset"), statusOffsetValue);
    statusOffsetValue->setProperty("monospace", true);
    addSeparator();
    addMetric(translated("Chunk"), statusChunkValue);
    addSeparator();
    addMetric(translated("Selected"), statusSelectionValue);
    statusSelectionValue->setProperty("monospace", true);
    statusLayout->addStretch(1);
    infoBar->addWidget(statusPanel, 1);
}

void MainWindowUi::applyTheme(QMainWindow* window) const
{
    QFile styleFile(QStringLiteral(":/styles/main.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        window->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    QPalette hexPalette = hexView->palette();
    hexPalette.setColor(QPalette::Base, QColor(QStringLiteral("#faf9f5")));
    hexPalette.setColor(QPalette::Text, QColor(QStringLiteral("#141413")));
    hexPalette.setColor(QPalette::Highlight, QColor(QStringLiteral("#cc785c")));
    hexPalette.setColor(QPalette::HighlightedText, Qt::white);
    hexView->setPalette(hexPalette);
}

ByteOrder MainWindowUi::byteOrder() const
{
    return static_cast<ByteOrder>(byteOrderButtonGroup->checkedId());
}

WordSize MainWindowUi::wordSize() const
{
    return static_cast<WordSize>(wordSizeButtonGroup->checkedId());
}
