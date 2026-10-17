#include "GatewayChatClient.h"
#include "GatewayConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

GatewayChatClient::GatewayChatClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void GatewayChatClient::setPort(int port)
{
    if (port > 0 && port < 65536)
        m_port = port;
}

void GatewayChatClient::setGatewayToken(const QString &token)
{
    m_token = token.trimmed();
}

bool GatewayChatClient::isConfigured() const
{
    return !m_token.isEmpty() || !GatewayConfig::ensureGatewayToken().isEmpty();
}

void GatewayChatClient::clearHistory()
{
    m_history.clear();
}

QUrl GatewayChatClient::completionsUrl() const
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1/v1/chat/completions").arg(m_port));
}

QByteArray GatewayChatClient::buildRequestBody() const
{
    QJsonArray messages;
    for (const ChatMessage &m : m_history) {
        QJsonObject o;
        o.insert(QStringLiteral("role"), m.role);
        o.insert(QStringLiteral("content"), m.content);
        messages.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("model"), QStringLiteral("openclaw/default"));
    root.insert(QStringLiteral("messages"), messages);
    root.insert(QStringLiteral("stream"), false);
    root.insert(QStringLiteral("user"), QStringLiteral("qt-openclaw:main"));
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void GatewayChatClient::sendUserMessage(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return;
    if (m_reply) {
        emit errorOccurred(tr("请求进行中，请稍候。"));
        return;
    }
    QString token = m_token;
    if (token.isEmpty())
        token = GatewayConfig::ensureGatewayToken();
    if (token.isEmpty()) {
        emit logMessage(tr("[chat] missing gateway token"));
        emit errorOccurred(tr("还没准备好，请先在「设置」里填入 API Key。"));
        return;
    }
    m_token = token;
    m_history.append({QStringLiteral("user"), trimmed});

    QNetworkRequest req(completionsUrl());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + m_token.toUtf8());

    emit logMessage(tr("[chat] → %1").arg(completionsUrl().toString()));
    emit requestStarted();
    m_reply = m_nam->post(req, buildRequestBody());
    connect(m_reply, &QNetworkReply::finished, this, &GatewayChatClient::onReplyFinished);
}

void GatewayChatClient::onReplyFinished()
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
        emit logMessage(tr("[chat] gateway error: %1 (%2)").arg(reply->errorString(), detail));
        emit errorOccurred(tr("暂时连不上，请确认已点「连上」，稍后再试。"));
        emit requestFinished();
        return;
    }
    const QJsonArray choices = QJsonDocument::fromJson(body).object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        emit logMessage(tr("[chat] empty choices"));
        emit errorOccurred(tr("没有收到回复，请稍后再试。"));
        emit requestFinished();
        return;
    }
    QString content = choices.at(0).toObject().value(QStringLiteral("message")).toObject()
                          .value(QStringLiteral("content")).toString();
    if (content.isEmpty()) {
        emit logMessage(tr("[chat] empty message"));
        emit errorOccurred(tr("没有收到回复，请稍后再试。"));
        emit requestFinished();
        return;
    }
    m_history.append({QStringLiteral("assistant"), content});
    emit assistantMessage(content);
    emit requestFinished();
}
