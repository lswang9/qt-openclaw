#pragma once

#include <QObject>
#include <QString>

/**
 * Stub client for the OpenClaw Gateway WebSocket protocol.
 *
 * Later: open a QWebSocket to the gateway URL, authenticate with the token,
 * and exchange chat / tool events. For now only appends log lines.
 */
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
