#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qt-openclaw"));
    QApplication::setOrganizationName(QStringLiteral("qt-openclaw"));

    MainWindow window;
    window.show();

    return app.exec();
}
