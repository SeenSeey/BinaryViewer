#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Binary Viewer"));
    application.setApplicationDisplayName(QStringLiteral("Binary Viewer"));
    application.setOrganizationName(QStringLiteral("Engineering Tools"));

    MainWindow window;
    window.show();
    return application.exec();
}
