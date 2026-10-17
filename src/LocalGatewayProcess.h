#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

/**
 * Launches local OpenClaw Gateway as QProcess.
 * Isolation: openclaw --profile qiling (state ~/.openclaw-qiling; never ~/.openclaw).
 */
class LocalGatewayProcess : public QObject
{
    Q_OBJECT
public:
    static constexpr int kDefaultPort = 19889;

    explicit LocalGatewayProcess(QObject *parent = nullptr);
    ~LocalGatewayProcess() override;

    bool isRunning() const;
    QString defaultUrl() const;
    int port() const { return m_port; }
    void setPort(int port);

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
};
