#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct GatewayChatMessage {
    QString role;    // "system" | "user" | "assistant"
    QString content;
};

/**
 * OpenClaw Gateway client.
 *
 * Chat hot path: HTTP OpenAI-compatible endpoint on the local Gateway
 *   POST http://127.0.0.1:<port>/v1/chat/completions
 * with Authorization: Bearer <gateway-token> (from Settings / openclaw.json).
 * DeepSeek is configured INTO the Gateway; Qt never calls api.deepseek.com.
 *
 * connectToGateway remains a light WS stub for lifecycle logging.
 */
class GatewayClient : public QObject
{
    Q_OBJECT

public:
    explicit GatewayClient(QObject *parent = nullptr);

    void setGatewayHttpBase(const QString &baseUrl); // e.g. http://127.0.0.1:18789
    void setGatewayToken(const QString &token);
    void setBackendModel(const QString &modelRef);   // e.g. deepseek/deepseek-flash
    void setSystemPrompt(const QString &prompt);

    QString gatewayHttpBase() const { return m_httpBase; }
    bool isBusy() const { return m_reply != nullptr; }
    bool isConnected() const { return m_connected; }

    void clearHistory();

public slots:
    void connectToGateway(const QString &url, const QString &token);
    void disconnectFromGateway();
    void sendUserMessage(const QString &text);

signals:
    void logMessage(const QString &line);
    void statusChanged(const QString &status);
    void assistantMessage(const QString &text);
    void errorOccurred(const QString &message);
    void requestStarted();
    void requestFinished();

private slots:
    void onReplyFinished();

private:
    QUrl completionsUrl() const;
    QByteArray buildRequestBody() const;
    static QString wsUrlToHttpBase(const QString &wsUrl);

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    bool m_connected = false;
    QString m_url;
    QString m_httpBase = QStringLiteral("http://127.0.0.1:18789");
    QString m_token;
    QString m_backendModel = QStringLiteral("deepseek/deepseek-flash");
    QVector<GatewayChatMessage> m_history;
};
