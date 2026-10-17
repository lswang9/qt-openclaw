#include "SettingsDialog.h"
#include "GatewayChatClient.h"
#include "GatewayConfig.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

namespace {
constexpr auto kOrg = "qt-openclaw";
constexpr auto kApp = "qt-openclaw";
constexpr auto kKeyApi = "deepseek/apiKey";
constexpr auto kKeyApiNew = "openclaw/deepseekApiKey";
constexpr auto kKeyShowLog = "ui/showTechLog";
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    setModal(true);
    resize(480, 260);

    auto *hint = new QLabel(
        tr("填入 DeepSeek API Key，保存后点「连上」启动 Gateway，\n"
           "窗口内嵌官方 Control UI（http://127.0.0.1:19889/）。\n"
           "内部：openclaw --profile qiling models auth paste-api-key。"),
        this);
    hint->setWordWrap(true);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("粘贴 API Key…"));

    m_showLogCheck = new QCheckBox(tr("显示技术日志（高级）"), this);

    auto *form = new QFormLayout;
    form->addRow(tr("DeepSeek API Key"), m_apiKeyEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addLayout(form);
    layout->addWidget(m_showLogCheck);
    layout->addWidget(buttons);
    load();
}

void SettingsDialog::load()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    QString api = s.value(QString::fromUtf8(kKeyApiNew)).toString();
    if (api.isEmpty())
        api = s.value(QString::fromUtf8(kKeyApi)).toString();
    m_apiKeyEdit->setText(api);
    m_showLogCheck->setChecked(s.value(QString::fromUtf8(kKeyShowLog), false).toBool());
}

void SettingsDialog::onSave()
{
    const QString api = m_apiKeyEdit->text().trimmed();
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    s.setValue(QString::fromUtf8(kKeyApi), api);
    s.setValue(QString::fromUtf8(kKeyApiNew), api);
    s.setValue(QString::fromUtf8(kKeyShowLog), m_showLogCheck->isChecked());

    if (!api.isEmpty()) {
        QString err;
        if (!GatewayConfig::configureDeepSeekViaCli(api, &err)) {
            QString detail = err.trimmed();
            if (detail.size() > 280)
                detail = detail.left(280) + QStringLiteral("…");
            QMessageBox::warning(
                this, tr("保存失败"),
                tr("暂时没法保存 Key。请确认本机服务已装好后再试。\n\n%1").arg(detail));
            qWarning("SettingsDialog save failed: %s", qPrintable(err));
            return;
        }
    }
    accept();
}

bool SettingsDialog::hasApiKey()
{
    return !apiKey().isEmpty();
}

QString SettingsDialog::apiKey()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    QString api = s.value(QString::fromUtf8(kKeyApiNew)).toString();
    if (api.isEmpty())
        api = s.value(QString::fromUtf8(kKeyApi)).toString();
    return api.trimmed();
}

bool SettingsDialog::showTechLog()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    return s.value(QString::fromUtf8(kKeyShowLog), false).toBool();
}

void SettingsDialog::loadInto(GatewayChatClient *client)
{
    if (!client)
        return;
    client->setPort(GatewayConfig::kDefaultGatewayPort);
    client->setGatewayToken(GatewayConfig::ensureGatewayToken());
}
