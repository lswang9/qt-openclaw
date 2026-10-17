#include "CharacterSelectDialog.h"
#include "RoleLoader.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

CharacterSelectDialog::CharacterSelectDialog(const QString &currentRoleId, QWidget *parent)
    : QDialog(parent)
    , m_selected(currentRoleId)
{
    setWindowTitle(tr("器灵 Agent — 换角色"));
    setModal(true);
    resize(720, 480);
    setStyleSheet(QStringLiteral(
        "QDialog { background: #1c1c1e; color: #f5f5f7; }"
        "QLabel { color: #f5f5f7; }"
        "QPushButton { background: #0a84ff; color: white; border: none; border-radius: 8px;"
        "  padding: 10px 16px; font-weight: 600; }"
        "QPushButton:hover { background: #409cff; }"
        "QFrame#card { background: #2c2c2e; border-radius: 16px; }"));

    auto *row = new QHBoxLayout;
    row->setSpacing(24);

    const QVector<RoleInfo> roles = RoleLoader::listRoles();
    QVector<RoleInfo> show = roles;
    if (show.isEmpty()) {
        show.append(RoleLoader::loadRole(QStringLiteral("babata")));
        show.append(RoleLoader::loadRole(QStringLiteral("yinyue")));
    }

    for (const RoleInfo &role : show) {
        auto *card = new QFrame(this);
        card->setObjectName(QStringLiteral("card"));
        card->setFixedWidth(280);
        auto *cv = new QVBoxLayout(card);
        cv->setContentsMargins(16, 16, 16, 16);
        cv->setSpacing(12);

        auto *avatar = new QLabel(card);
        avatar->setFixedSize(240, 240);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(QStringLiteral("border-radius: 12px; background: #3a3a3c;"));
        QPixmap pix(role.avatarPath);
        if (!pix.isNull())
            avatar->setPixmap(pix.scaled(240, 240, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        else
            avatar->setText(role.displayName);

        auto *name = new QLabel(role.displayName, card);
        name->setAlignment(Qt::AlignCenter);
        name->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));

        QString tag = (role.id == QLatin1String("yinyue"))
                          ? tr("「安静稳妥」")
                          : tr("「聪明活泼」");
        auto *sub = new QLabel(tag, card);
        sub->setAlignment(Qt::AlignCenter);
        sub->setStyleSheet(QStringLiteral("color: #a1a1a6;"));

        auto *btn = new QPushButton(
            role.id == currentRoleId ? tr("当前角色") : tr("用这个角色"), card);
        if (role.id == currentRoleId)
            btn->setEnabled(false);
        connect(btn, &QPushButton::clicked, this, [this, id = role.id]() { pick(id); });

        cv->addWidget(avatar);
        cv->addWidget(name);
        cv->addWidget(sub);
        cv->addWidget(btn);
        row->addWidget(card);
    }

    auto *layout = new QVBoxLayout(this);
    auto *hint = new QLabel(tr("已就绪"), this);
    hint->setStyleSheet(QStringLiteral("color: #30d158; font-size: 13px;"));
    layout->addWidget(hint);
    layout->addStretch();
    layout->addLayout(row);
    layout->addStretch();
}

void CharacterSelectDialog::pick(const QString &roleId)
{
    m_selected = roleId;
    accept();
}
