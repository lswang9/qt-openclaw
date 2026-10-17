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
} // namespace

QString GatewayConfig::stateDir()
{
    const QString dir = QDir::home().absoluteFilePath(QStringLiteral(".qiling-agent"));
    QDir().mkpath(dir);
    return dir;
}

void GatewayConfig::applyProcessEnvironment(QProcessEnvironment &env)
{
    const QString home = stateDir();
    env.insert(QStringLiteral("OPENCLAW_HOME"), home);
    env.insert(QStringLiteral("OPENCLAW_STATE_DIR"), home);
    // Avoid accidental use of official default home
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
    // Best-effort: push token into isolated config via official CLI (no hand-edit)
    QString err;
    const QString cli = findOpenclawCli(&err);
    if (!cli.isEmpty()) {
        QString out, errout;
        runCli(cli, {QStringLiteral("config"), QStringLiteral("set"),
                     QStringLiteral("gateway.auth.mode"), QStringLiteral("token")},
               {}, &out, &errout, 30000);
        runCli(cli, {QStringLiteral("config"), QStringLiteral("set"),
                     QStringLiteral("gateway.auth.token"), tok},
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
        QDir::current().absoluteFilePath(QStringLiteral("runtime/bin/openclaw")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../runtime/node_modules/.bin/openclaw")),
        QDir::current().absoluteFilePath(QStringLiteral("runtime/node_modules/.bin/openclaw")),
        QStandardPaths::findExecutable(QStringLiteral("openclaw")),
    };
    for (const QString &c : candidates) {
        if (!c.isEmpty() && QFileInfo::exists(c) && QFileInfo(c).isExecutable())
            return c;
    }
    // Last resort: known user global binary path (binary only; state stays ~/.qiling-agent)
    const QString homeBin = QDir::home().absoluteFilePath(QStringLiteral(".openclaw/bin/openclaw"));
    if (QFileInfo::exists(homeBin) && QFileInfo(homeBin).isExecutable())
        return homeBin;

    if (errorOut)
        *errorOut = QStringLiteral("openclaw CLI not found (runtime/ or PATH)");
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

    // Ensure directory exists for CLI state
    stateDir();

    QString out, err;

    // Port isolation
    if (!runCli(cli, {QStringLiteral("config"), QStringLiteral("set"),
                      QStringLiteral("gateway.port"), QString::number(kDefaultGatewayPort)},
                {}, &out, &err)) {
        qWarning().noquote() << "[GatewayConfig] config set gateway.port:" << err << out;
        // continue — may already be set
    }

    runCli(cli, {QStringLiteral("config"), QStringLiteral("set"),
                 QStringLiteral("gateway.bind"), QStringLiteral("loopback")},
           {}, &out, &err);
    runCli(cli, {QStringLiteral("config"), QStringLiteral("set"),
                 QStringLiteral("gateway.mode"), QStringLiteral("local")},
           {}, &out, &err);
    runCli(cli, {QStringLiteral("config"), QStringLiteral("set"),
                 QStringLiteral("gateway.http.endpoints.chatCompletions.enabled"), QStringLiteral("true")},
           {}, &out, &err);
    runCli(cli, {QStringLiteral("config"), QStringLiteral("set"),
                 QStringLiteral("agents.defaults.model.primary"), QStringLiteral("deepseek/deepseek-flash")},
           {}, &out, &err);
    runCli(cli, {QStringLiteral("plugins"), QStringLiteral("enable"), QStringLiteral("deepseek"),
                 QStringLiteral("--accept-capabilities")},
           {}, &out, &err);

    // Auth via official paste-api-key (key on stdin; never logged)
    const bool ok = runCli(cli,
                           {QStringLiteral("models"), QStringLiteral("auth"), QStringLiteral("paste-api-key"),
                            QStringLiteral("--provider"), QStringLiteral("deepseek")},
                           key.toUtf8() + '\n', &out, &err, 90000);
    if (!ok) {
        // Common: CLI refuses when a system Gateway still points at ~/.openclaw
        QString msg = err.trimmed().isEmpty() ? out.trimmed() : err.trimmed();
        if (msg.size() > 600)
            msg = msg.left(600) + QStringLiteral("…");
        if (errorOut)
            *errorOut = QStringLiteral("openclaw models auth paste-api-key failed: %1").arg(msg);
        qWarning() << "[GatewayConfig] paste-api-key failed (key redacted)";
        return false;
    }

    ensureGatewayToken();
    qInfo() << "[GatewayConfig] DeepSeek key configured via CLI under ~/.qiling-agent (key not logged)";
    return true;
}
