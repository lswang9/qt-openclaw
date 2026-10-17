#pragma once

#include <QObject>
#include <QString>

/** Lightweight WS connect stub (chat uses GatewayChatClient HTTP). */
class GatewayClient : public QObject
{
    Q_OBJECT
public:
    explicit GatewayClient(QObject *parent = nullptr);

public slots:
    void connectToGateway(const QString &url, const QString &token);
    void disconnectFromGateway();

signals:
    void logMessage(const QString &line);
    void statusChanged(const QString &status);

private:
    bool m_connected = false;
    QString m_url;
};
