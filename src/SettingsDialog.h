#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QComboBox;

/**
 * Settings for Gateway / OpenClaw model config (DeepSeek key written into
 * ~/.openclaw on Save). Qt chat talks to the local Gateway — not api.deepseek.com.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    static bool hasApiKey();
    static QString apiKey();
    static QString model();
    static QString role();
    static QString gatewayToken();

    /** Persist current fields and sync into OpenClaw config. Returns summary log. */
    QString saveAndSyncOpenClaw(QString *errorOut = nullptr);

private slots:
    void onSave();

private:
    void load();

    QLineEdit *m_apiKeyEdit = nullptr;
    QLineEdit *m_modelEdit = nullptr;
    QComboBox *m_roleCombo = nullptr;
    QLineEdit *m_gatewayTokenEdit = nullptr;
};
