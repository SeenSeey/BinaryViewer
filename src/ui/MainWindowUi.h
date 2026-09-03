#pragma once

#include "core/Types.h"

class QAction;
class QButtonGroup;
class QLabel;
class QMainWindow;
class QPlainTextEdit;
class QSpinBox;
class HexView;

struct MainWindowUi final
{
    void setup(QMainWindow* window);
    void applyTheme(QMainWindow* window) const;

    [[nodiscard]] ByteOrder byteOrder() const;
    [[nodiscard]] WordSize wordSize() const;

    QAction* openAction = nullptr;
    QAction* previousAction = nullptr;
    QAction* nextAction = nullptr;
    HexView* hexView = nullptr;
    QPlainTextEdit* binaryView = nullptr;
    QLabel* statusFileValue = nullptr;
    QLabel* statusSizeValue = nullptr;
    QLabel* statusOffsetValue = nullptr;
    QLabel* statusChunkValue = nullptr;
    QLabel* statusSelectionValue = nullptr;
    QSpinBox* chunkSizeSpinBox = nullptr;
    QButtonGroup* byteOrderButtonGroup = nullptr;
    QButtonGroup* wordSizeButtonGroup = nullptr;
};
