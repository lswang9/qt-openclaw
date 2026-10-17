#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

/** UI-facing role fields only (display). Engine persona lives in Gateway plugins. */
struct RoleInfo {
    QString id;
    QString displayName;
    QString welcomeMessage;
    QString avatarPath;
    QString backgroundPath;
    int schemaVersion = 1;
    int contractVersion = 1;
};

class RoleLoader
{
public:
    static constexpr int kMinSchemaVersion = 1;
    static constexpr int kMaxSchemaVersion = 3;
    static constexpr auto kDefaultRoleId = "babata";

    static QString agentsRoot();
    static void setAgentsRoot(const QString &path);
    static QVector<RoleInfo> listRoles();
    static RoleInfo loadRole(const QString &id);
    static bool isSupportedSchema(int schemaVersion);

private:
    static QString resolveAsset(const QString &roleId, const QString &relativeFromRoleDir,
                                const QString &fallbackFileName);
    static QString m_overrideRoot;
};
