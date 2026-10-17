#include "SettingsDialog.h"
#include "GatewayChatClient.h"
#include "GatewayConfig.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

namespace {
constexpr auto kOrg = "qt-openclaw";
constexpr auto kApp = "qt-openclaw";
constexpr auto kKeyApi = "deepseek/apiKey";
constexpr auto kKeyApiNew = "openclaw/deepseekApiKey";
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置 — 器灵 Agent"));
    setModal(true);
    resize(500, 240);

    auto *hint = new QLabel(
        tr("DeepSeek API Key 通过官方 CLI（models auth paste-api-key）写入独立目录 ~/.qiling-agent。\n"
           "本壳不手写 openclaw.json / .env。聊天走本机 Gateway 端口 %1。\n"
           "与官方默认 18789 / ~/.openclaw 隔离。")
            .arg(GatewayConfig::kDefaultGatewayPort),
        this);
    hint->setWordWrap(true);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("sk-…"));

    auto *form = new QFormLayout;
    form->addRow(tr("DeepSeek API key"), m_apiKeyEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addLayout(form);
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
}

void SettingsDialog::onSave()
{
    const QString api = m_apiKeyEdit->text().trimmed();
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    s.setValue(QString::fromUtf8(kKeyApi), api);
    s.setValue(QString::fromUtf8(kKeyApiNew), api);

    if (!api.isEmpty()) {
        QString err;
        if (!GatewayConfig::configureDeepSeekViaCli(api, &err)) {
            QMessageBox::warning(
                this, tr("配置 Key 失败"),
                err + QStringLiteral("\n\n")
                    + tr("若本机已有指向 ~/.openclaw 的 Gateway 服务，paste-api-key 可能报 divergent state。\n"
                         "请用本 App「连上」以 OPENCLAW_HOME=~/.qiling-agent 启动本地进程，或先停掉冲突服务。"));
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

void SettingsDialog::loadInto(GatewayChatClient *client)
{
    if (!client)
        return;
    client->setPort(GatewayConfig::kDefaultGatewayPort);
    client->setGatewayToken(GatewayConfig::ensureGatewayToken());
}
