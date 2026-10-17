#include "RoleMaterializer.h"
#include "GatewayConfig.h"
#include "RoleLoader.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace {
bool runProcess(const QString &program, const QStringList &args, QString *errOut, int timeoutMs = 120000)
{
    QProcess p;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    GatewayConfig::applyProcessEnvironment(env);
    p.setProcessEnvironment(env);
    p.setProgram(program);
    p.setArguments(args);
    p.start();
    if (!p.waitForStarted(15000)) {
        if (errOut)
            *errOut = QStringLiteral("failed to start %1: %2").arg(program, p.errorString());
        return false;
    }
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        if (errOut)
            *errOut = QStringLiteral("timed out: %1").arg(program);
        return false;
    }
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        QString msg = QString::fromUtf8(p.readAllStandardError()).trimmed();
        if (msg.isEmpty())
            msg = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        if (errOut)
            *errOut = QStringLiteral("%1 exited %2: %3").arg(program).arg(p.exitCode()).arg(msg.left(500));
        return false;
    }
    return true;
}

QString findNode()
{
    const QString n = QStandardPaths::findExecutable(QStringLiteral("node"));
    if (!n.isEmpty())
        return n;
    const QString home = QDir::home().absoluteFilePath(
        QStringLiteral(".openclaw/tools/node-v24.19.0/bin/node"));
    if (QFileInfo::exists(home))
        return home;
    return {};
}

QString materializeScriptPath()
{
    const QString root = RoleLoader::agentsRoot();
    const QString script = QDir(root).absoluteFilePath(QStringLiteral("scripts/materialize-workspace.mjs"));
    if (QFileInfo::exists(script))
        return script;
    return {};
}
} // namespace

QString RoleMaterializer::workspacePath(const QString &roleId)
{
    return QDir(GatewayConfig::stateDir()).absoluteFilePath(
        QStringLiteral("workspaces/%1").arg(roleId));
}

bool RoleMaterializer::materializeRole(const QString &roleId, QString *errorOut)
{
    const QString id = roleId.trimmed();
    if (id.isEmpty()) {
        if (errorOut)
            *errorOut = QStringLiteral("empty role id");
        return false;
    }

    QString findErr;
    const QString cli = GatewayConfig::findOpenclawCli(&findErr);
    if (cli.isEmpty()) {
        if (errorOut)
            *errorOut = findErr;
        return false;
    }

    const QString ws = workspacePath(id);
    QDir().mkpath(ws);

    // 1) agents add (ok if already exists)
    QString err;
    const QStringList addArgs = GatewayConfig::profileArgs() + QStringList{
        QStringLiteral("agents"), QStringLiteral("add"), id,
        QStringLiteral("--workspace"), ws,
        QStringLiteral("--non-interactive"),
    };
    if (!runProcess(cli, addArgs, &err, 90000)) {
        // Treat "already exists" style failures as soft-ok
        const QString lower = err.toLower();
        if (!lower.contains(QLatin1String("exist")) && !lower.contains(QLatin1String("already")))
            qWarning().noquote() << "[RoleMaterializer] agents add:" << err;
    }

    // 2) materialize-workspace.mjs (official pack → workspace files)
    const QString node = findNode();
    const QString script = materializeScriptPath();
    if (node.isEmpty() || script.isEmpty()) {
        if (errorOut)
            *errorOut = QStringLiteral("materialize script or node not found");
        return false;
    }
    const QStringList matArgs = {
        script,
        QStringLiteral("--role"), id,
        QStringLiteral("--pack-root"), RoleLoader::agentsRoot(),
        QStringLiteral("--workspace"), ws,
    };
    if (!runProcess(node, matArgs, &err, 120000)) {
        if (errorOut)
            *errorOut = err;
        return false;
    }

    // 3) set-identity --from-identity (no Qt systemPrompt)
    const QStringList idArgs = GatewayConfig::profileArgs() + QStringList{
        QStringLiteral("agents"), QStringLiteral("set-identity"),
        QStringLiteral("--agent"), id,
        QStringLiteral("--from-identity"),
    };
    if (!runProcess(cli, idArgs, &err, 90000)) {
        // Try workspace form
        const QStringList idArgs2 = GatewayConfig::profileArgs() + QStringList{
            QStringLiteral("agents"), QStringLiteral("set-identity"),
            QStringLiteral("--workspace"), ws,
            QStringLiteral("--from-identity"),
        };
        if (!runProcess(cli, idArgs2, &err, 90000)) {
            if (errorOut)
                *errorOut = err;
            return false;
        }
    }

    qInfo() << "[RoleMaterializer] materialized" << id << "→" << ws;
    return true;
}
