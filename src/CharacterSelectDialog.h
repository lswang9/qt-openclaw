#pragma once

#include <QDialog>
#include <QString>

class CharacterSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CharacterSelectDialog(const QString &currentRoleId, QWidget *parent = nullptr);
    QString selectedRoleId() const { return m_selected; }

private:
    void pick(const QString &roleId);
    QString m_selected;
};
