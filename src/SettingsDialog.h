#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QLineEdit;
class GatewayChatClient;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    static bool hasApiKey();
    static QString apiKey();
    static void loadInto(GatewayChatClient *client);
    static bool showTechLog();

private slots:
    void onSave();

private:
    void load();
    QLineEdit *m_apiKeyEdit = nullptr;
    QCheckBox *m_showLogCheck = nullptr;
};
