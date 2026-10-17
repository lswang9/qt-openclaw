#pragma once

#include <QDialog>

class QLineEdit;

/**
 * Simple first-run / settings dialog for DeepSeek (OpenAI-compatible).
 * Values persist in QSettings under org/app qt-openclaw. Never commit secrets.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    static void loadInto(class DeepSeekClient *client);
    static bool hasApiKey();

private slots:
    void onSave();

private:
    void load();

    QLineEdit *m_apiKeyEdit = nullptr;
    QLineEdit *m_baseUrlEdit = nullptr;
    QLineEdit *m_modelEdit = nullptr;
};
