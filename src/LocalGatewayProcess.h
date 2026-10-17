#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

/**
 * Launches a local OpenClaw Gateway as a child process (QProcess).
 *
 * Discovery order for the CLI:
 *  1. <appDir>/../runtime/bin/openclaw  (bundled next to the binary / repo)
 *  2. <repo>/runtime/bin/openclaw       (dev: cwd / source-relative)
 *  3. OPENCLAW_RUNTIME env → bin/openclaw
 *  4. `openclaw` on PATH
 *  5. `node` on PATH + runtime/node_modules/openclaw/dist/entry.js
 *
 * Default command (foreground): openclaw gateway run --port <port> --verbose --allow-unconfigured
 * Default WS URL: ws://127.0.0.1:18789
 */
class LocalGatewayProcess : public QObject
{
    Q_OBJECT

public:
    static constexpr int kDefaultPort = 18789;

    explicit LocalGatewayProcess(QObject *parent = nullptr);
    ~LocalGatewayProcess() override;

    bool isRunning() const;
    QString defaultUrl() const;
    int port() const { return m_port; }
    void setPort(int port);

    /** Injected into gateway child env as DEEPSEEK_API_KEY (Gateway uses DeepSeek). */
    void setDeepSeekApiKey(const QString &key) { m_deepSeekApiKey = key.trimmed(); }

    /** Resolve CLI path without starting; empty if not found. */
    QString resolvedCliPath() const;
    QString resolveError() const { return m_resolveError; }

public slots:
    void start();
    void stop();

signals:
    void logMessage(const QString &line);
    void statusChanged(const QString &status);
    void started();
    void stopped(int exitCode, QProcess::ExitStatus exitStatus);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError error);

private:
    struct LaunchPlan {
        QString program;
        QStringList arguments;
        QString workingDirectory;
    };

    LaunchPlan buildLaunchPlan();
    QString findOpenclawCli() const;
    QString findNode() const;
    QStringList runtimeSearchRoots() const;

    QProcess *m_process = nullptr;
    int m_port = kDefaultPort;
    mutable QString m_resolveError;
    QString m_deepSeekApiKey;
};
