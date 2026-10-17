#pragma once

#include <QProcessEnvironment>
#include <QString>

/**
 * Isolated OpenClaw home for 器灵 Agent (never ~/.openclaw).
 * Shell only sets env + invokes official CLI; does NOT hand-edit
 * openclaw.json / .env / IDENTITY / auth-profiles.
 */
class GatewayConfig
{
public:
    static constexpr int kDefaultGatewayPort = 19889;

    /** ~/.qiling-agent */
    static QString stateDir();
    static QString ensureGatewayToken();

    /** OPENCLAW_HOME + OPENCLAW_STATE_DIR = stateDir() */
    static void applyProcessEnvironment(QProcessEnvironment &env);

    /**
     * Save DeepSeek key via official CLI:
     *   openclaw models auth paste-api-key --provider deepseek
     * Also ensures gateway.port=19889 via `openclaw config set`.
     * Never writes config files from the shell process itself.
     */
    static bool configureDeepSeekViaCli(const QString &apiKey, QString *errorOut = nullptr);

    /** Resolve openclaw CLI: runtime/bin → PATH (not ~/.openclaw state). */
    static QString findOpenclawCli(QString *errorOut = nullptr);
};
