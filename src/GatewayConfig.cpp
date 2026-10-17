#include "GatewayConfig.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

namespace {
constexpr auto kOrg = "qt-openclaw";
constexpr auto kApp = "qt-openclaw";
constexpr auto kKeyToken = "gateway/token";

QProcessEnvironment isolatedEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    GatewayConfig::applyProcessEnvironment(env);
    return env;
}

bool runCli(const QString &program, const QStringList &args,
            const QByteArray &stdinData, QString *stdoutOut, QString *stderrOut, int timeoutMs = 60000)
{
    QProcess p;
    p.setProcessEnvironment(isolatedEnv());
    p.setProgram(program);
    p.setArguments(args);
    p.start();
    if (!p.waitForStarted(10000)) {
        if (stderrOut)
            *stderrOut = QStringLiteral("failed to start: %1").arg(p.errorString());
        return false;
    }
    if (!stdinData.isEmpty()) {
        p.write(stdinData);
        p.closeWriteChannel();
    } else {
        p.closeWriteChannel();
    }
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        if (stderrOut)
            *stderrOut = QStringLiteral("CLI timed out");
        return false;
    }
    if (stdoutOut)
        *stdoutOut = QString::fromUtf8(p.readAllStandardOutput());
    if (stderrOut)
        *stderrOut = QString::fromUtf8(p.readAllStandardError());
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

QStringList withProfile(const QStringList &rest)
{
    return GatewayConfig::profileArgs() + rest;
}
} // namespace

QStringList GatewayConfig::profileArgs()
{
    return {QStringLiteral("--profile"), QString::fromUtf8(kProfileName)};
}

QString GatewayConfig::stateDir()
{
    // Official --profile qiling maps here; do not hand-edit contents.
    return QDir::home().absoluteFilePath(QStringLiteral(".openclaw-qiling"));
}

void GatewayConfig::applyProcessEnvironment(QProcessEnvironment &env)
{
    // Isolation is --profile qiling, not OPENCLAW_HOME override.
    // Clearing these avoids fighting LaunchAgent ai.openclaw.gateway (~/.openclaw).
    env.remove(QStringLiteral("OPENCLAW_HOME"));
    env.remove(QStringLiteral("OPENCLAW_STATE_DIR"));
    env.remove(QStringLiteral("OPENCLAW_CONFIG_PATH"));
}

QString GatewayConfig::ensureGatewayToken()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    QString tok = s.value(QString::fromUtf8(kKeyToken)).toString().trimmed();
    if (tok.isEmpty()) {
        tok = QUuid::createUuid().toString(QUuid::WithoutBraces);
        s.setValue(QString::fromUtf8(kKeyToken), tok);
    }
    // Best-effort: push token into profile config via official CLI (no hand-edit)
    QString err;
    const QString cli = findOpenclawCli(&err);
    if (!cli.isEmpty()) {
        QString out, errout;
        runCli(cli, withProfile({QStringLiteral("config"), QStringLiteral("set"),
                                 QStringLiteral("gateway.auth.mode"), QStringLiteral("token")}),
               {}, &out, &errout, 30000);
        runCli(cli, withProfile({QStringLiteral("config"), QStringLiteral("set"),
                                 QStringLiteral("gateway.auth.token"), tok}),
               {}, &out, &errout, 30000);
    }
    return tok;
}

QString GatewayConfig::findOpenclawCli(QString *errorOut)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).absoluteFilePath(QStringLiteral("../runtime/bin/openclaw")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../runtime/bin/openclaw")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../runtime/bin/openclaw")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../../runtime/bin/openclaw")),
        QDir::current().absoluteFilePath(QStringLiteral("runtime/bin/openclaw")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../runtime/node_modules/.bin/openclaw")),
        QDir::current().absoluteFilePath(QStringLiteral("runtime/node_modules/.bin/openclaw")),
        QStandardPaths::findExecutable(QStringLiteral("openclaw")),
    };
    for (const QString &c : candidates) {
        if (!c.isEmpty() && QFileInfo::exists(c) && QFileInfo(c).isExecutable())
            return c;
    }
    // Binary only; state is --profile qiling → ~/.openclaw-qiling
    const QString homeBin = QDir::home().absoluteFilePath(QStringLiteral(".openclaw/bin/openclaw"));
    if (QFileInfo::exists(homeBin) && QFileInfo(homeBin).isExecutable())
        return homeBin;

    if (errorOut)
        *errorOut = QStringLiteral("local runtime not found");
    return {};
}

bool GatewayConfig::configureDeepSeekViaCli(const QString &apiKey, QString *errorOut)
{
    const QString key = apiKey.trimmed();
    if (key.isEmpty()) {
        if (errorOut)
            *errorOut = QStringLiteral("API key is empty");
        return false;
    }

    QString findErr;
    const QString cli = findOpenclawCli(&findErr);
    if (cli.isEmpty()) {
        if (errorOut)
            *errorOut = findErr;
        return false;
    }

    QString out, err;

    // Port isolation under profile (CLI writes; shell does not touch files)
    if (!runCli(cli, withProfile({QStringLiteral("config"), QStringLiteral("set"),
                                  QStringLiteral("gateway.port"), QString::number(kDefaultGatewayPort)}),
                {}, &out, &err)) {
        qWarning().noquote() << "[GatewayConfig] config set gateway.port:" << err << out;
    }

    runCli(cli, withProfile({QStringLiteral("config"), QStringLiteral("set"),
                             QStringLiteral("gateway.bind"), QStringLiteral("loopback")}),
           {}, &out, &err);
    runCli(cli, withProfile({QStringLiteral("config"), QStringLiteral("set"),
                             QStringLiteral("gateway.mode"), QStringLiteral("local")}),
           {}, &out, &err);
    runCli(cli, withProfile({QStringLiteral("config"), QStringLiteral("set"),
                             QStringLiteral("gateway.http.endpoints.chatCompletions.enabled"),
                             QStringLiteral("true")}),
           {}, &out, &err);
    runCli(cli, withProfile({QStringLiteral("config"), QStringLiteral("set"),
                             QStringLiteral("agents.defaults.model.primary"),
                             QStringLiteral("deepseek/deepseek-flash")}),
           {}, &out, &err);
    runCli(cli, withProfile({QStringLiteral("plugins"), QStringLiteral("enable"),
                             QStringLiteral("deepseek"), QStringLiteral("--accept-capabilities")}),
           {}, &out, &err);

    // Auth via official paste-api-key (key on stdin; never logged)
    // OpenClaw 2026.9.7+: multi-agent profiles require an explicit owner.
    const bool ok = runCli(cli,
                           withProfile({QStringLiteral("models"), QStringLiteral("auth"),
                                        QStringLiteral("paste-api-key"),
                                        QStringLiteral("--provider"), QStringLiteral("deepseek"),
                                        QStringLiteral("--agent"), QStringLiteral("main")}),
                           key.toUtf8() + '\n', &out, &err, 90000);
    if (!ok) {
        QString msg = err.trimmed().isEmpty() ? out.trimmed() : err.trimmed();
        if (msg.size() > 600)
            msg = msg.left(600) + QStringLiteral("…");
        if (errorOut)
            *errorOut = QStringLiteral("openclaw --profile qiling models auth paste-api-key failed: %1")
                            .arg(msg);
        qWarning() << "[GatewayConfig] paste-api-key failed (key redacted)";
        return false;
    }

    ensureGatewayToken();
    qInfo() << "[GatewayConfig] DeepSeek key configured via --profile qiling → ~/.openclaw-qiling (key not logged)";
    return true;
}


QString GatewayConfig::controlUiUrl()
{
    return QStringLiteral("http://127.0.0.1:%1/").arg(kDefaultGatewayPort);
}

bool GatewayConfig::ensureQilingBrandPlugin(QString *errorOut)
{
    QString findErr;
    const QString cli = findOpenclawCli(&findErr);
    if (cli.isEmpty()) {
        if (errorOut)
            *errorOut = findErr;
        return false;
    }

    QString out, err;
    // True source: https://github.com/lswang9/qiling-brand @ v0.1.1 (brandContractVersion=1; pin tag, never bare main)
    // Do NOT install from resources/control-ui-themes (pointer README only).
    const bool installed = runCli(
        cli,
        withProfile({QStringLiteral("plugins"), QStringLiteral("install"),
                     QStringLiteral("github:lswang9/qiling-brand#v0.1.1"),
                     QStringLiteral("--force"), QStringLiteral("--accept-capabilities")}),
        {}, &out, &err, 180000);
    if (!installed) {
        QString msg = err.trimmed().isEmpty() ? out.trimmed() : err.trimmed();
        if (msg.size() > 600)
            msg = msg.left(600) + QStringLiteral("…");
        if (errorOut)
            *errorOut = QStringLiteral("plugins install github:lswang9/qiling-brand#v0.1.1 failed: %1")
                            .arg(msg);
        return false;
    }

    runCli(cli,
           withProfile({QStringLiteral("plugins"), QStringLiteral("enable"),
                        QStringLiteral("qiling-brand"), QStringLiteral("--accept-capabilities")}),
           {}, &out, &err, 60000);

    runCli(cli,
           withProfile({QStringLiteral("plugins"), QStringLiteral("reload"),
                        QStringLiteral("qiling-brand")}),
           {}, &out, &err, 30000);

    qInfo() << "[GatewayConfig] qiling-brand ensured via github:lswang9/qiling-brand#v0.1.1";
    return true;
}
