#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct ChatMessage {
    QString role;    // "system" | "user" | "assistant"
    QString content;
};

/**
 * MVP chat path: call DeepSeek OpenAI-compatible /chat/completions directly
 * (not via OpenClaw Gateway). Faster to ship a working chat UI.
 *
 * Defaults (overridable via QSettings / SettingsDialog):
 *   base URL: https://api.deepseek.com
 *   model:    deepseek-flash
 */
class DeepSeekClient : public QObject
{
    Q_OBJECT

public:
    explicit DeepSeekClient(QObject *parent = nullptr);

    void setApiKey(const QString &key);
    void setBaseUrl(const QString &url);
    void setModel(const QString &model);

    QString apiKey() const { return m_apiKey; }
    QString baseUrl() const { return m_baseUrl; }
    QString model() const { return m_model; }

    bool isConfigured() const;
    bool isBusy() const { return m_reply != nullptr; }

    void clearHistory();
    void setSystemPrompt(const QString &prompt);

public slots:
    void sendUserMessage(const QString &text);

signals:
    void assistantMessage(const QString &text);
    void errorOccurred(const QString &message);
    void requestStarted();
    void requestFinished();

private slots:
    void onReplyFinished();

private:
    QUrl completionsUrl() const;
    QByteArray buildRequestBody() const;

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    QString m_apiKey;
    QString m_baseUrl = QStringLiteral("https://api.deepseek.com");
    QString m_model = QStringLiteral("deepseek-flash");
    QVector<ChatMessage> m_history;
};
