#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct DeepSeekChatMessage {
    QString role;
    QString content;
};

/**
 * LEGACY / demoted: direct HTTPS client to api.deepseek.com.
 *
 * Hot path is GatewayClient → local OpenClaw Gateway HTTP.
 * This class is kept for reference / optional offline experiments only;
 * MainWindow no longer uses it.
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
    QVector<DeepSeekChatMessage> m_history;
};
