#include "LocalGatewayProcess.h"
#include "GatewayConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

LocalGatewayProcess::LocalGatewayProcess(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &LocalGatewayProcess::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &LocalGatewayProcess::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LocalGatewayProcess::onFinished);
    connect(m_process, &QProcess::errorOccurred, this, &LocalGatewayProcess::onErrorOccurred);
    connect(m_process, &QProcess::started, this, [this]() {
        emit logMessage(tr("[gateway] process started (pid %1)").arg(m_process->processId()));
        emit statusChanged(tr("已就绪"));
        emit started();
    });
}

LocalGatewayProcess::~LocalGatewayProcess()
{
    stop();
}

bool LocalGatewayProcess::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

QString LocalGatewayProcess::defaultUrl() const
{
    return QStringLiteral("ws://127.0.0.1:%1").arg(m_port);
}

void LocalGatewayProcess::setPort(int port)
{
    if (port > 0 && port < 65536)
        m_port = port;
}

QStringList LocalGatewayProcess::runtimeSearchRoots() const
{
    QStringList roots;
    const QString envRuntime = qEnvironmentVariable("OPENCLAW_RUNTIME");
    if (!envRuntime.isEmpty())
        roots << envRuntime;
    const QString appDir = QCoreApplication::applicationDirPath();
    roots << QDir(appDir).absoluteFilePath(QStringLiteral("../runtime"));
    roots << QDir(appDir).absoluteFilePath(QStringLiteral("runtime"));
    roots << QDir(appDir).absoluteFilePath(QStringLiteral("../../runtime"));
    roots << QDir::current().absoluteFilePath(QStringLiteral("runtime"));
    roots.removeDuplicates();
    return roots;
}

QString LocalGatewayProcess::findNode() const
{
    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("node"));
    if (!fromPath.isEmpty())
        return fromPath;
    for (const QString &root : runtimeSearchRoots()) {
        const QString bundled = QDir(root).absoluteFilePath(QStringLiteral("bin/node"));
        if (QFileInfo::exists(bundled) && QFileInfo(bundled).isExecutable())
            return bundled;
    }
    return {};
}

QString LocalGatewayProcess::findOpenclawCli() const
{
    m_resolveError.clear();
    for (const QString &root : runtimeSearchRoots()) {
        const QString cli = QDir(root).absoluteFilePath(QStringLiteral("bin/openclaw"));
        if (QFileInfo::exists(cli) && QFileInfo(cli).isExecutable())
            return cli;
    }
    const QString onPath = QStandardPaths::findExecutable(QStringLiteral("openclaw"));
    if (!onPath.isEmpty())
        return onPath;
    const QString node = findNode();
    if (!node.isEmpty()) {
        for (const QString &root : runtimeSearchRoots()) {
            const QString entry = QDir(root).absoluteFilePath(
                QStringLiteral("node_modules/openclaw/dist/entry.js"));
            if (QFileInfo::exists(entry))
                return QStringLiteral("node+entry:%1|%2").arg(node, entry);
        }
    }
    m_resolveError = tr(
        "未找到 OpenClaw CLI。请运行 ./scripts/setup-runtime.sh，或安装 openclaw 到 PATH。");
    return {};
}

QString LocalGatewayProcess::resolvedCliPath() const
{
    return findOpenclawCli();
}

LocalGatewayProcess::LaunchPlan LocalGatewayProcess::buildLaunchPlan()
{
    LaunchPlan plan;
    const QString cli = findOpenclawCli();
    if (cli.isEmpty())
        return plan;

    // Config written before start; no --allow-unconfigured
    const QStringList gatewayArgs = {
        QStringLiteral("gateway"),
        QStringLiteral("run"),
        QStringLiteral("--port"),
        QString::number(m_port),
        QStringLiteral("--verbose"),
    };

    if (cli.startsWith(QStringLiteral("node+entry:"))) {
        const QString payload = cli.mid(QStringLiteral("node+entry:").size());
        const QStringList parts = payload.split(QLatin1Char('|'));
        if (parts.size() == 2) {
            plan.program = parts[0];
            plan.arguments = QStringList{parts[1]} + gatewayArgs;
            plan.workingDirectory = QFileInfo(parts[1]).absolutePath();
        }
    } else {
        plan.program = cli;
        plan.arguments = gatewayArgs;
        plan.workingDirectory = QFileInfo(cli).absolutePath();
    }
    return plan;
}

void LocalGatewayProcess::start()
{
    if (isRunning()) {
        emit logMessage(tr("[gateway] already running"));
        return;
    }

    const LaunchPlan plan = buildLaunchPlan();
    if (plan.program.isEmpty()) {
        emit logMessage(QStringLiteral("[gateway] %1").arg(m_resolveError));
        emit statusChanged(tr("未找到 Gateway"));
        return;
    }

    emit logMessage(tr("[gateway] starting: %1 %2")
                        .arg(plan.program, plan.arguments.join(QLatin1Char(' '))));
    emit logMessage(tr("[gateway] OPENCLAW_STATE_DIR=%1").arg(GatewayConfig::stateDir()));
    emit statusChanged(tr("正在连接…"));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    GatewayConfig::applyProcessEnvironment(env);
    m_process->setProcessEnvironment(env);
    m_process->setWorkingDirectory(plan.workingDirectory);
    m_process->setProgram(plan.program);
    m_process->setArguments(plan.arguments);
    m_process->start();
}

void LocalGatewayProcess::stop()
{
    if (!isRunning())
        return;
    emit logMessage(tr("[gateway] stopping…"));
    emit statusChanged(tr("正在断开…"));
    m_process->terminate();
    if (!m_process->waitForFinished(5000)) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void LocalGatewayProcess::onReadyReadStdout()
{
    const QByteArray data = m_process->readAllStandardOutput();
    for (const QByteArray &line : data.split('\n')) {
        const QString text = QString::fromUtf8(line).trimmed();
        if (!text.isEmpty())
            emit logMessage(QStringLiteral("[gateway:out] %1").arg(text));
    }
}

void LocalGatewayProcess::onReadyReadStderr()
{
    const QByteArray data = m_process->readAllStandardError();
    for (const QByteArray &line : data.split('\n')) {
        const QString text = QString::fromUtf8(line).trimmed();
        if (!text.isEmpty())
            emit logMessage(QStringLiteral("[gateway:err] %1").arg(text));
    }
}

void LocalGatewayProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    emit logMessage(tr("[gateway] exited code %1").arg(exitCode));
    emit statusChanged(tr("未连接"));
    emit stopped(exitCode, exitStatus);
}

void LocalGatewayProcess::onErrorOccurred(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        emit logMessage(tr("[gateway] failed to start: %1").arg(m_process->errorString()));
        emit statusChanged(tr("启动失败"));
    }
}
