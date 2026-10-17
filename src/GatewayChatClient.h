#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVector>

struct ChatMessage {
    QString role; // user | assistant (no shell-injected system)
    QString content;
};

/** HTTP chat client → local Gateway only (never api.deepseek.com). */
class GatewayChatClient : public QObject
{
    Q_OBJECT
public:
    explicit GatewayChatClient(QObject *parent = nullptr);

    void setPort(int port);
    void setGatewayToken(const QString &token);
    int port() const { return m_port; }
    bool isConfigured() const;
    bool isBusy() const { return m_reply != nullptr; }
    void clearHistory();

public slots:
    void sendUserMessage(const QString &text);

signals:
    void assistantMessage(const QString &text);
    void errorOccurred(const QString &message);
    void requestStarted();
    void requestFinished();
    void logMessage(const QString &line);

private slots:
    void onReplyFinished();

private:
    QUrl completionsUrl() const;
    QByteArray buildRequestBody() const;

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    int m_port = 19889;
    QString m_token;
    QVector<ChatMessage> m_history;
};
