#include "OpenClawConfigSync.h"
#include "Persona.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

namespace {

bool writeTextFile(const QString &path, const QString &content, QString *err)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err)
            *err = QStringLiteral("cannot open %1: %2").arg(path, f.errorString());
        return false;
    }
    f.write(content.toUtf8());
    if (!f.commit()) {
        if (err)
            *err = QStringLiteral("cannot write %1: %2").arg(path, f.errorString());
        return false;
    }
    return true;
}

QJsonObject readJsonObject(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

bool writeJsonObject(const QString &path, const QJsonObject &obj, QString *err)
{
    return writeTextFile(path, QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented)), err);
}

/** Upsert DEEPSEEK_API_KEY in ~/.openclaw/.env without clobbering other keys. */
bool upsertEnvFile(const QString &envPath, const QString &apiKey, QString *err)
{
    QStringList lines;
    if (QFile::exists(envPath)) {
        QFile f(envPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (err)
                *err = QStringLiteral("cannot read %1").arg(envPath);
            return false;
        }
        QTextStream in(&f);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.startsWith(QStringLiteral("DEEPSEEK_API_KEY=")))
                continue;
            lines.append(line);
        }
    }
    lines.append(QStringLiteral("DEEPSEEK_API_KEY=%1").arg(apiKey));
    return writeTextFile(envPath, lines.join(QLatin1Char('\n')) + QLatin1Char('\n'), err);
}

} // namespace

QString OpenClawConfigSync::openclawHome()
{
    return QDir::home().absoluteFilePath(QStringLiteral(".openclaw"));
}

QString OpenClawConfigSync::normalizeModelRef(const QString &model)
{
    QString m = model.trimmed();
    if (m.isEmpty())
        m = QStringLiteral("deepseek-flash");
    if (!m.contains(QLatin1Char('/')))
        m = QStringLiteral("deepseek/") + m;
    return m;
}

QString OpenClawConfigSync::readGatewayToken()
{
    const QJsonObject root = readJsonObject(QDir(openclawHome()).absoluteFilePath(QStringLiteral("openclaw.json")));
    return root.value(QStringLiteral("gateway")).toObject()
        .value(QStringLiteral("auth")).toObject()
        .value(QStringLiteral("token")).toString();
}

QString OpenClawConfigSync::gatewayHttpBase(int port)
{
    return QStringLiteral("http://127.0.0.1:%1").arg(port);
}

OpenClawConfigSync::Result OpenClawConfigSync::apply(const Input &in)
{
    Result r;
    r.openclawHome = openclawHome();
    QDir home(r.openclawHome);
    if (!home.exists() && !QDir().mkpath(r.openclawHome)) {
        r.error = QStringLiteral("cannot create %1").arg(r.openclawHome);
        return r;
    }

    const QString apiKey = in.apiKey.trimmed();
    if (apiKey.isEmpty()) {
        r.error = QStringLiteral("API key is empty");
        return r;
    }

    const QString role = Persona::normalizeRole(in.role);
    r.modelRef = normalizeModelRef(in.model);
    r.envPath = home.absoluteFilePath(QStringLiteral(".env"));
    r.configPath = home.absoluteFilePath(QStringLiteral("openclaw.json"));
    r.identityPath = home.absoluteFilePath(QStringLiteral("workspace/IDENTITY.md"));
    r.authProfilesPath = home.absoluteFilePath(QStringLiteral("agents/main/agent/auth-profiles.json"));

    QString err;
    QStringList log;

    // 1) .env — Gateway process and DeepSeek plugin read DEEPSEEK_API_KEY
    if (!upsertEnvFile(r.envPath, apiKey, &err)) {
        r.error = err;
        return r;
    }
    log << QStringLiteral("wrote %1 (DEEPSEEK_API_KEY)").arg(r.envPath);

    // 2) auth-profiles.json — provider key for agent runtime
    {
        QDir().mkpath(QFileInfo(r.authProfilesPath).absolutePath());
        QJsonObject store = readJsonObject(r.authProfilesPath);
        if (!store.contains(QStringLiteral("version")))
            store.insert(QStringLiteral("version"), 1);
        QJsonObject profiles = store.value(QStringLiteral("profiles")).toObject();
        const QString profileId = QStringLiteral("deepseek:manual");
        QJsonObject cred;
        cred.insert(QStringLiteral("type"), QStringLiteral("api_key"));
        cred.insert(QStringLiteral("provider"), QStringLiteral("deepseek"));
        cred.insert(QStringLiteral("key"), apiKey);
        profiles.insert(profileId, cred);
        store.insert(QStringLiteral("profiles"), profiles);
        if (!writeJsonObject(r.authProfilesPath, store, &err)) {
            r.error = err;
            return r;
        }
        log << QStringLiteral("wrote %1 (deepseek:manual)").arg(r.authProfilesPath);
    }

    // 3) openclaw.json — model, plugin, HTTP chat endpoint, identity
    {
        QJsonObject root = readJsonObject(r.configPath);

        // auth.profiles meta (key lives in auth-profiles.json)
        {
            QJsonObject auth = root.value(QStringLiteral("auth")).toObject();
            QJsonObject profiles = auth.value(QStringLiteral("profiles")).toObject();
            QJsonObject meta;
            meta.insert(QStringLiteral("mode"), QStringLiteral("api_key"));
            meta.insert(QStringLiteral("provider"), QStringLiteral("deepseek"));
            profiles.insert(QStringLiteral("deepseek:manual"), meta);
            auth.insert(QStringLiteral("profiles"), profiles);
            root.insert(QStringLiteral("auth"), auth);
        }

        // models.providers.deepseek
        {
            QJsonObject models = root.value(QStringLiteral("models")).toObject();
            QJsonObject providers = models.value(QStringLiteral("providers")).toObject();
            QJsonObject deepseek = providers.value(QStringLiteral("deepseek")).toObject();
            if (deepseek.isEmpty()) {
                deepseek.insert(QStringLiteral("api"), QStringLiteral("openai-completions"));
                deepseek.insert(QStringLiteral("baseUrl"), QStringLiteral("https://api.deepseek.com"));
                deepseek.insert(QStringLiteral("models"), QJsonArray());
            }
            providers.insert(QStringLiteral("deepseek"), deepseek);
            models.insert(QStringLiteral("providers"), providers);
            root.insert(QStringLiteral("models"), models);
        }

        // plugins.entries.deepseek.enabled
        {
            QJsonObject plugins = root.value(QStringLiteral("plugins")).toObject();
            QJsonObject entries = plugins.value(QStringLiteral("entries")).toObject();
            QJsonObject deepseek = entries.value(QStringLiteral("deepseek")).toObject();
            deepseek.insert(QStringLiteral("enabled"), true);
            entries.insert(QStringLiteral("deepseek"), deepseek);
            plugins.insert(QStringLiteral("entries"), entries);
            root.insert(QStringLiteral("plugins"), plugins);
        }

        // agents.defaults.model + identity on main
        {
            QJsonObject agents = root.value(QStringLiteral("agents")).toObject();
            QJsonObject defaults = agents.value(QStringLiteral("defaults")).toObject();
            defaults.insert(QStringLiteral("model"), r.modelRef);
            agents.insert(QStringLiteral("defaults"), defaults);

            QJsonObject entries = agents.value(QStringLiteral("entries")).toObject();
            QJsonObject main = entries.value(QStringLiteral("main")).toObject();
            // Prefer defaults model; keep entry model aligned without forced auth profile suffix
            main.insert(QStringLiteral("model"), r.modelRef);
            QJsonObject identity = main.value(QStringLiteral("identity")).toObject();
            identity.insert(QStringLiteral("name"), role);
            identity.insert(QStringLiteral("emoji"), QStringLiteral("✨"));
            identity.insert(QStringLiteral("theme"),
                            QStringLiteral("自称%1；禁止自称器灵/助手/AI").arg(role));
            main.insert(QStringLiteral("identity"), identity);
            entries.insert(QStringLiteral("main"), main);
            agents.insert(QStringLiteral("entries"), entries);
            root.insert(QStringLiteral("agents"), agents);
        }

        // gateway.http.endpoints.chatCompletions.enabled — Qt chats via local Gateway HTTP
        {
            QJsonObject gateway = root.value(QStringLiteral("gateway")).toObject();
            QJsonObject http = gateway.value(QStringLiteral("http")).toObject();
            QJsonObject endpoints = http.value(QStringLiteral("endpoints")).toObject();
            QJsonObject chatCompletions = endpoints.value(QStringLiteral("chatCompletions")).toObject();
            chatCompletions.insert(QStringLiteral("enabled"), true);
            endpoints.insert(QStringLiteral("chatCompletions"), chatCompletions);
            http.insert(QStringLiteral("endpoints"), endpoints);
            gateway.insert(QStringLiteral("http"), http);
            // Prefer local bind for the Qt-launched gateway
            if (!gateway.contains(QStringLiteral("bind")))
                gateway.insert(QStringLiteral("bind"), QStringLiteral("loopback"));
            root.insert(QStringLiteral("gateway"), gateway);
        }

        if (!writeJsonObject(r.configPath, root, &err)) {
            r.error = err;
            return r;
        }
        log << QStringLiteral("patched %1 (model=%2, chatCompletions, identity=%3)")
                   .arg(r.configPath, r.modelRef, role);
    }

    // 4) IDENTITY.md — agent workspace persona
    {
        QDir().mkpath(QFileInfo(r.identityPath).absolutePath());
        if (!writeTextFile(r.identityPath, Persona::identityMarkdown(role), &err)) {
            r.error = err;
            return r;
        }
        log << QStringLiteral("wrote %1 (Name=%2)").arg(r.identityPath, role);
    }

    r.ok = true;
    r.summary = log.join(QLatin1Char('\n'));
    return r;
}
