#include "RoleLoader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QString RoleLoader::m_overrideRoot;

QString RoleLoader::agentsRoot()
{
    if (!m_overrideRoot.isEmpty())
        return m_overrideRoot;
    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../third_party/qiling-agents")),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../../third_party/qiling-agents")),
        QDir::current().absoluteFilePath(QStringLiteral("third_party/qiling-agents")),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../../../third_party/qiling-agents")),
    };
    for (const QString &c : candidates) {
        if (QFileInfo::exists(QDir(c).absoluteFilePath(QStringLiteral("catalog/index.json"))))
            return QDir(c).absolutePath();
    }
    return QDir::current().absoluteFilePath(QStringLiteral("third_party/qiling-agents"));
}

void RoleLoader::setAgentsRoot(const QString &path) { m_overrideRoot = path.trimmed(); }

bool RoleLoader::isSupportedSchema(int schemaVersion)
{
    return schemaVersion >= kMinSchemaVersion && schemaVersion <= kMaxSchemaVersion;
}

QString RoleLoader::resolveAsset(const QString &roleId, const QString &relativeFromRoleDir,
                                 const QString &fallbackFileName)
{
    const QString root = agentsRoot();
    if (!relativeFromRoleDir.isEmpty()) {
        const QString p = QDir(root).absoluteFilePath(
            QStringLiteral("roles/%1/%2").arg(roleId, relativeFromRoleDir));
        if (QFileInfo::exists(p))
            return p;
    }
    const QString qrc = QStringLiteral(":/characters/%1/%2").arg(roleId, fallbackFileName);
    if (QFileInfo::exists(qrc))
        return qrc;
    const QStringList shells = {
        QDir::current().absoluteFilePath(QStringLiteral("resources/characters/%1/%2").arg(roleId, fallbackFileName)),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(
            QStringLiteral("../resources/characters/%1/%2").arg(roleId, fallbackFileName)),
    };
    for (const QString &p : shells) {
        if (QFileInfo::exists(p))
            return p;
    }
    return qrc;
}

QVector<RoleInfo> RoleLoader::listRoles()
{
    QVector<RoleInfo> out;
    QFile f(QDir(agentsRoot()).absoluteFilePath(QStringLiteral("catalog/index.json")));
    if (!f.open(QIODevice::ReadOnly))
        return out;
    for (const QJsonValue &v : QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("roles")).toArray()) {
        const QString id = v.toObject().value(QStringLiteral("id")).toString();
        if (!id.isEmpty())
            out.append(loadRole(id));
    }
    return out;
}

RoleInfo RoleLoader::loadRole(const QString &id)
{
    RoleInfo info;
    const QString roleId = id.trimmed().isEmpty() ? QString::fromUtf8(kDefaultRoleId) : id.trimmed();
    info.id = roleId;

    QFile f(QDir(agentsRoot()).absoluteFilePath(QStringLiteral("roles/%1/role.json").arg(roleId)));
    if (!f.open(QIODevice::ReadOnly)) {
        info.displayName = (roleId == QLatin1String("yinyue")) ? QStringLiteral("银月") : QStringLiteral("巴巴塔");
        info.welcomeMessage = QStringLiteral("你好，我是%1～").arg(info.displayName);
        info.avatarPath = resolveAsset(roleId, {}, QStringLiteral("avatar.jpg"));
        info.backgroundPath = resolveAsset(roleId, {}, QStringLiteral("bg.jpg"));
        return info;
    }

    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    info.schemaVersion = root.value(QStringLiteral("schemaVersion")).toInt(1);
    info.contractVersion = root.value(QStringLiteral("contractVersion")).toInt(1);
    info.displayName = root.value(QStringLiteral("displayName")).toString(roleId);
    if (info.displayName.isEmpty())
        info.displayName = root.value(QStringLiteral("officialName")).toString(roleId);
    info.welcomeMessage = root.value(QStringLiteral("welcomeMessage")).toString();
    if (info.welcomeMessage.isEmpty())
        info.welcomeMessage = QStringLiteral("你好，我是%1～").arg(info.displayName);

    info.avatarPath = resolveAsset(roleId, root.value(QStringLiteral("avatarPath")).toString(),
                                   QStringLiteral("avatar.jpg"));
    // bg-fullscreen.jpg in pack; shell fallback bg.jpg
    QString bgRel = root.value(QStringLiteral("backgroundPath")).toString();
    info.backgroundPath = resolveAsset(roleId, bgRel, QStringLiteral("bg.jpg"));
    if (!QFileInfo::exists(info.backgroundPath) && !bgRel.isEmpty())
        info.backgroundPath = resolveAsset(roleId, {}, QStringLiteral("bg.jpg"));
    return info;
}
