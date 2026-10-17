#include "GatewayClient.h"

GatewayClient::GatewayClient(QObject *parent)
    : QObject(parent)
{
}

void GatewayClient::connectToGateway(const QString &url, const QString &token)
{
    Q_UNUSED(token);

    m_url = url;
    m_connected = true;

    // TODO: QWebSocket to OpenClaw Gateway; authenticate with token.
    emit logMessage(tr("[stub] connect requested → %1").arg(url));
    emit statusChanged(tr("Connected (stub)"));
}

void GatewayClient::disconnectFromGateway()
{
    if (!m_connected) {
        emit logMessage(tr("[stub] already disconnected"));
        return;
    }

    m_connected = false;
    emit logMessage(tr("[stub] disconnect requested from %1").arg(m_url));
    emit statusChanged(tr("Disconnected"));
    m_url.clear();
}
