// LEGACY: demoted from hot path. Prefer GatewayClient → local OpenClaw Gateway.
#include "DeepSeekClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

DeepSeekClient::DeepSeekClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_history.append({QStringLiteral("system"),
                      QStringLiteral("You are a helpful assistant.")});
}

void DeepSeekClient::setApiKey(const QString &key)
{
    m_apiKey = key.trimmed();
}

void DeepSeekClient::setBaseUrl(const QString &url)
{
    QString u = url.trimmed();
    while (u.endsWith(QLatin1Char('/')))
        u.chop(1);
    if (!u.isEmpty())
        m_baseUrl = u;
}

void DeepSeekClient::setModel(const QString &model)
{
    if (!model.trimmed().isEmpty())
        m_model = model.trimmed();
}

bool DeepSeekClient::isConfigured() const
{
    return !m_apiKey.isEmpty() && !m_baseUrl.isEmpty() && !m_model.isEmpty();
}

void DeepSeekClient::clearHistory()
{
    QString system;
    if (!m_history.isEmpty() && m_history.first().role == QLatin1String("system"))
        system = m_history.first().content;
    m_history.clear();
    if (!system.isEmpty())
        m_history.append({QStringLiteral("system"), system});
}

void DeepSeekClient::setSystemPrompt(const QString &prompt)
{
    if (!m_history.isEmpty() && m_history.first().role == QLatin1String("system"))
        m_history[0].content = prompt;
    else
        m_history.prepend({QStringLiteral("system"), prompt});
}

QUrl DeepSeekClient::completionsUrl() const
{
    // DeepSeek accepts https://api.deepseek.com/chat/completions
    // (also works with /v1 prefix). Prefer without forcing /v1.
    return QUrl(m_baseUrl + QStringLiteral("/chat/completions"));
}

QByteArray DeepSeekClient::buildRequestBody() const
{
    QJsonArray messages;
    for (const DeepSeekChatMessage &m : m_history) {
        QJsonObject o;
        o.insert(QStringLiteral("role"), m.role);
        o.insert(QStringLiteral("content"), m.content);
        messages.append(o);
    }

    QJsonObject root;
    root.insert(QStringLiteral("model"), m_model);
    root.insert(QStringLiteral("messages"), messages);
    root.insert(QStringLiteral("stream"), false);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void DeepSeekClient::sendUserMessage(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return;

    if (!isConfigured()) {
        emit errorOccurred(tr("DeepSeek API key not set. Open Settings first."));
        return;
    }
    if (m_reply) {
        emit errorOccurred(tr("A request is already in flight."));
        return;
    }

    m_history.append({QStringLiteral("user"), trimmed});

    QNetworkRequest req(completionsUrl());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization",
                     QByteArray("Bearer ") + m_apiKey.toUtf8());

    emit requestStarted();
    m_reply = m_nam->post(req, buildRequestBody());
    connect(m_reply, &QNetworkReply::finished, this, &DeepSeekClient::onReplyFinished);
}

void DeepSeekClient::onReplyFinished()
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
        // Roll back the last user message on hard failure so retry is clean.
        if (!m_history.isEmpty() && m_history.last().role == QLatin1String("user"))
            m_history.removeLast();

        QString detail = QString::fromUtf8(body).trimmed();
        if (detail.size() > 400)
            detail = detail.left(400) + QStringLiteral("…");
        emit errorOccurred(tr("DeepSeek HTTP error: %1 (%2)")
                               .arg(reply->errorString(), detail));
        emit requestFinished();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    const QJsonObject root = doc.object();
    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        emit errorOccurred(tr("DeepSeek response had no choices."));
        emit requestFinished();
        return;
    }

    const QJsonObject message = choices.at(0).toObject().value(QStringLiteral("message")).toObject();
    QString content = message.value(QStringLiteral("content")).toString();
    if (content.isEmpty()) {
        // Some models put text under reasoning/other fields; keep a clear error.
        emit errorOccurred(tr("DeepSeek returned an empty assistant message."));
        emit requestFinished();
        return;
    }

    m_history.append({QStringLiteral("assistant"), content});
    emit assistantMessage(content);
    emit requestFinished();
}
