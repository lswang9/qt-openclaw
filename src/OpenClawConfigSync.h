#pragma once

#include <QString>

/**
 * Writes DeepSeek provider key + model + persona into the local OpenClaw
 * Gateway config (not used for direct api.deepseek.com calls from Qt).
 *
 * Targets (~/.openclaw):
 *  - .env                 DEEPSEEK_API_KEY
 *  - openclaw.json        model, plugin, HTTP chatCompletions, identity, auth profile meta
 *  - agents/main/agent/auth-profiles.json   inline api_key (Gateway reads this)
 *  - workspace/IDENTITY.md                  persona name for agent bootstrap
 */
class OpenClawConfigSync
{
public:
    struct Input {
        QString apiKey;
        QString model;   // e.g. deepseek-flash or deepseek/deepseek-flash
        QString role;    // 巴巴塔 | 银月
    };

    struct Result {
        bool ok = false;
        QString summary; // human-readable log lines
        QString error;
        QString openclawHome;
        QString configPath;
        QString envPath;
        QString authProfilesPath;
        QString identityPath;
        QString modelRef; // deepseek/...
    };

    static QString openclawHome();
    static QString normalizeModelRef(const QString &model);
    static QString readGatewayToken();
    static QString gatewayHttpBase(int port = 18789);

    /** Apply key/model/role into OpenClaw files. Safe to call on Save or Start Gateway. */
    static Result apply(const Input &in);
};
