#include "GatewayClient.h"
#include "OpenClawConfigSync.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

GatewayClient::GatewayClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void GatewayClient::setGatewayHttpBase(const QString &baseUrl)
{
    QString u = baseUrl.trimmed();
    while (u.endsWith(QLatin1Char('/')))
        u.chop(1);
    if (!u.isEmpty())
        m_httpBase = u;
}

void GatewayClient::setGatewayToken(const QString &token)
{
    m_token = token.trimmed();
}

void GatewayClient::setBackendModel(const QString &modelRef)
{
    const QString m = OpenClawConfigSync::normalizeModelRef(modelRef);
    if (!m.isEmpty())
        m_backendModel = m;
}

void GatewayClient::setSystemPrompt(const QString &prompt)
{
    if (!m_history.isEmpty() && m_history.first().role == QLatin1String("system"))
        m_history[0].content = prompt;
    else
        m_history.prepend({QStringLiteral("system"), prompt});
}

void GatewayClient::clearHistory()
{
    QString system;
    if (!m_history.isEmpty() && m_history.first().role == QLatin1String("system"))
        system = m_history.first().content;
    m_history.clear();
    if (!system.isEmpty())
        m_history.append({QStringLiteral("system"), system});
}

QString GatewayClient::wsUrlToHttpBase(const QString &wsUrl)
{
    QUrl u(wsUrl.trimmed());
    if (!u.isValid() || u.host().isEmpty())
        return QStringLiteral("http://127.0.0.1:18789");
    const QString scheme = (u.scheme() == QLatin1String("wss") || u.scheme() == QLatin1String("https"))
                               ? QStringLiteral("https")
                               : QStringLiteral("http");
    // OpenClaw multiplexes WS + HTTP on the same Gateway port (default 18789).
    int httpPort = u.port(-1);
    if (httpPort < 0)
        httpPort = (scheme == QLatin1String("https")) ? 443 : 18789;
    return QStringLiteral("%1://%2:%3").arg(scheme, u.host()).arg(httpPort);
}

void GatewayClient::connectToGateway(const QString &url, const QString &token)
{
    m_url = url;
    m_httpBase = wsUrlToHttpBase(url);
    if (!token.trimmed().isEmpty())
        m_token = token.trimmed();
    else if (m_token.isEmpty())
        m_token = OpenClawConfigSync::readGatewayToken();

    m_connected = true;
    emit logMessage(tr("[gateway] chat HTTP base → %1/v1/chat/completions").arg(m_httpBase));
    emit logMessage(tr("[stub-ws] connect noted → %1 (chat uses Gateway HTTP, not DeepSeek direct)").arg(url));
    emit statusChanged(tr("Connected (Gateway HTTP chat)"));
}

void GatewayClient::disconnectFromGateway()
{
    if (!m_connected) {
        emit logMessage(tr("[gateway] already disconnected"));
        return;
    }
    m_connected = false;
    emit logMessage(tr("[gateway] disconnect from %1").arg(m_url));
    emit statusChanged(tr("Disconnected"));
    m_url.clear();
}

QUrl GatewayClient::completionsUrl() const
{
    return QUrl(m_httpBase + QStringLiteral("/v1/chat/completions"));
}

QByteArray GatewayClient::buildRequestBody() const
{
    QJsonArray messages;
    for (const GatewayChatMessage &m : m_history) {
        QJsonObject o;
        o.insert(QStringLiteral("role"), m.role);
        o.insert(QStringLiteral("content"), m.content);
        messages.append(o);
    }

    QJsonObject root;
    // OpenClaw agent-target model id (not raw DeepSeek id)
    root.insert(QStringLiteral("model"), QStringLiteral("openclaw"));
    root.insert(QStringLiteral("messages"), messages);
    root.insert(QStringLiteral("stream"), false);
    // Stable session so multi-turn shares Gateway agent session
    root.insert(QStringLiteral("user"), QStringLiteral("qt-openclaw"));
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void GatewayClient::sendUserMessage(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return;

    if (m_reply) {
        emit errorOccurred(tr("A request is already in flight."));
        return;
    }

    if (m_token.isEmpty())
        m_token = OpenClawConfigSync::readGatewayToken();

    if (m_httpBase.isEmpty())
        m_httpBase = OpenClawConfigSync::gatewayHttpBase();

    // Ensure system prompt is present
    if (m_history.isEmpty() || m_history.first().role != QLatin1String("system")) {
        emit errorOccurred(tr("System prompt not set. Open Settings and Save."));
        return;
    }

    m_history.append({QStringLiteral("user"), trimmed});

    QNetworkRequest req(completionsUrl());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_token.isEmpty()) {
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_token.toUtf8());
    }
    // Override backend model on the selected agent when Gateway allows it
    req.setRawHeader("x-openclaw-model", m_backendModel.toUtf8());

    emit logMessage(tr("[chat] → Gateway %1 (model override %2)")
                        .arg(completionsUrl().toString(), m_backendModel));
    emit requestStarted();
    m_reply = m_nam->post(req, buildRequestBody());
    connect(m_reply, &QNetworkReply::finished, this, &GatewayClient::onReplyFinished);
}

void GatewayClient::onReplyFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    if (!reply) {
        emit requestFinished();
        return;
    }
    reply->deleteLater();

    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        if (!m_history.isEmpty() && m_history.last().role == QLatin1String("user"))
            m_history.removeLast();

        QString detail = QString::fromUtf8(body).trimmed();
        if (detail.size() > 400)
            detail = detail.left(400) + QStringLiteral("…");
        emit errorOccurred(tr("Gateway HTTP error: %1 (%2)")
                               .arg(reply->errorString(), detail));
        emit requestFinished();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    const QJsonObject root = doc.object();
    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        emit errorOccurred(tr("Gateway response had no choices."));
        emit requestFinished();
        return;
    }

    const QJsonObject message = choices.at(0).toObject().value(QStringLiteral("message")).toObject();
    QString content = message.value(QStringLiteral("content")).toString();
    if (content.isEmpty()) {
        emit errorOccurred(tr("Gateway returned an empty assistant message."));
        emit requestFinished();
        return;
    }

    m_history.append({QStringLiteral("assistant"), content});
    emit assistantMessage(content);
    emit requestFinished();
}
