#pragma once

#include "core/ViewerSession.h"

#include <QMainWindow>

#include <memory>

struct MainWindowUi;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void openFile();
    void previousChunk();
    void nextChunk();
    void chunkSizeChanged(int sizeInBytes);
    void conversionSettingsChanged();
    void selectionChanged(int start, int length);

private:
    void showCurrentChunk();
    void updateNavigation();
    void updateBinaryView();
    void updateStatusBar();
    void showFileError(const QString& message);

    ViewerSession session_;
    std::unique_ptr<MainWindowUi> ui_;
};
