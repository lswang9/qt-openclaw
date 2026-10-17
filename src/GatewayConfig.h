#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

/**
 * Isolated OpenClaw profile for 器灵 Agent (never default ~/.openclaw).
 * Uses official CLI: openclaw --profile qiling → ~/.openclaw-qiling
 * Does NOT hand-edit openclaw.json / .env / IDENTITY / auth-profiles.
 */
class GatewayConfig
{
public:
    static constexpr int kDefaultGatewayPort = 19889;
    /** Official profile name; state lives under ~/.openclaw-qiling */
    static constexpr const char *kProfileName = "qiling";

    /** ~/.openclaw-qiling (profile state dir; do not hand-edit) */
    static QString stateDir();
    static QString ensureGatewayToken();

    /** Prefixed to every openclaw argv: --profile qiling … */
    static QStringList profileArgs();

    /**
     * Clear OPENCLAW_HOME / OPENCLAW_STATE_DIR / OPENCLAW_CONFIG_PATH so
     * --profile qiling owns isolation (avoids divergent with ai.openclaw.gateway).
     */
    static void applyProcessEnvironment(QProcessEnvironment &env);

    /**
     * Save DeepSeek key via official CLI:
     *   openclaw --profile qiling models auth paste-api-key --provider deepseek --agent main
     * Also ensures gateway.port=19889 via config set. Never writes config files
     * from the shell process itself.
     */
    static bool configureDeepSeekViaCli(const QString &apiKey, QString *errorOut = nullptr);

    /** Resolve openclaw CLI: runtime/bin → PATH → ~/.openclaw/bin (binary only). */
    static QString findOpenclawCli(QString *errorOut = nullptr);

    /** http://127.0.0.1:19889/ — official Control UI / Dashboard */
    static QString controlUiUrl();

    /**
     * Install theme from independent repo (not bundled in shell):
     *   openclaw --profile qiling plugins install github:lswang9/qiling-brand#v0.1.1
     *   openclaw --profile qiling plugins enable qiling-brand
     *   openclaw --profile qiling plugins reload qiling-brand
     * Appearance: qiling-brand/babata | qiling-brand/yinyue
     */
    static bool ensureQilingBrandPlugin(QString *errorOut = nullptr);
};
