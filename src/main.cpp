#include "MainWindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>

namespace {
constexpr auto kSingletonServer = "com.qiling.agent.singleton";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qt-openclaw"));
    QApplication::setOrganizationName(QStringLiteral("qt-openclaw"));

    // Second instance: ping the first and exit.
    {
        QLocalSocket probe;
        probe.connectToServer(QString::fromUtf8(kSingletonServer));
        if (probe.waitForConnected(300)) {
            probe.write("activate\n");
            probe.flush();
            probe.waitForBytesWritten(300);
            probe.disconnectFromServer();
            return 0;
        }
    }

    MainWindow window;

    QLocalServer::removeServer(QString::fromUtf8(kSingletonServer));
    auto *server = new QLocalServer(&app);
    if (!server->listen(QString::fromUtf8(kSingletonServer))) {
        qWarning("singleton server listen failed: %s", qPrintable(server->errorString()));
    } else {
        QObject::connect(server, &QLocalServer::newConnection, &window, [&window, server]() {
            while (QLocalSocket *sock = server->nextPendingConnection()) {
                sock->deleteLater();
            }
            window.activateFromSecondInstance();
        });
    }

    window.show();
    return app.exec();
}
