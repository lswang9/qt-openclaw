#pragma once

#include <QString>

/**
 * Materialize role knowledge pack into official OpenClaw agent workspace.
 * Uses: openclaw --profile qiling agents add / set-identity + materialize-workspace.mjs
 * Does NOT assemble systemPrompt in the Qt shell.
 */
class RoleMaterializer
{
public:
    /** ~/.openclaw-qiling/workspaces/<roleId> */
    static QString workspacePath(const QString &roleId);

    /**
     * agents add (best-effort) → node materialize-workspace.mjs → set-identity --from-identity
     * Returns false on hard failure; details in errorOut (for tech log only).
     */
    static bool materializeRole(const QString &roleId, QString *errorOut = nullptr);
};
