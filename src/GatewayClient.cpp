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
    emit logMessage(tr("[stub-ws] noted connect → %1 (chat uses Gateway HTTP)").arg(url));
    emit statusChanged(tr("已就绪"));
}

void GatewayClient::disconnectFromGateway()
{
    if (!m_connected) {
        emit logMessage(tr("[stub-ws] already disconnected"));
        return;
    }
    m_connected = false;
    emit logMessage(tr("[stub-ws] disconnect %1").arg(m_url));
    emit statusChanged(tr("未连接"));
    m_url.clear();
}
