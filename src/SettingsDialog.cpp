#include "SettingsDialog.h"
#include "OpenClawConfigSync.h"
#include "Persona.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>

namespace {
constexpr auto kOrg = "qt-openclaw";
constexpr auto kApp = "qt-openclaw";
constexpr auto kKeyApi = "openclaw/deepseekApiKey";
constexpr auto kKeyModel = "openclaw/model";
constexpr auto kKeyRole = "openclaw/role";
constexpr auto kKeyGwToken = "openclaw/gatewayToken";
// Legacy keys (migrate once)
constexpr auto kLegacyApi = "deepseek/apiKey";
constexpr auto kLegacyModel = "deepseek/model";
constexpr auto kDefaultModel = "deepseek-flash";
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("%1 — Gateway / 模型设置").arg(Persona::productName()));
    setModal(true);
    resize(520, 300);

    auto *hint = new QLabel(
        tr("这里的 DeepSeek API Key 会写入本地 OpenClaw Gateway 配置（~/.openclaw），"
           "供 Gateway 调用模型。\n"
           "聊天由 Qt 客户端经本地 Gateway（HTTP /v1/chat/completions）完成，"
           "不会从本客户端直连 api.deepseek.com。\n"
           "角色自称默认「巴巴塔」（可选「银月」）；「器灵」仅为产品名，不作自我介绍。"),
        this);
    hint->setWordWrap(true);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("sk-…（写入 Gateway / OpenClaw）"));

    m_modelEdit = new QLineEdit(this);
    m_modelEdit->setPlaceholderText(QString::fromUtf8(kDefaultModel));

    m_roleCombo = new QComboBox(this);
    for (const QString &r : Persona::roleNames())
        m_roleCombo->addItem(r);

    m_gatewayTokenEdit = new QLineEdit(this);
    m_gatewayTokenEdit->setEchoMode(QLineEdit::Password);
    m_gatewayTokenEdit->setPlaceholderText(tr("可选；留空则读 ~/.openclaw/openclaw.json gateway.auth.token"));

    auto *form = new QFormLayout;
    form->addRow(tr("DeepSeek API key（Gateway）"), m_apiKeyEdit);
    form->addRow(tr("Model"), m_modelEdit);
    form->addRow(tr("角色自称"), m_roleCombo);
    form->addRow(tr("Gateway token"), m_gatewayTokenEdit);

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
    QString api = s.value(QString::fromUtf8(kKeyApi)).toString();
    if (api.isEmpty())
        api = s.value(QString::fromUtf8(kLegacyApi)).toString();
    m_apiKeyEdit->setText(api);

    QString model = s.value(QString::fromUtf8(kKeyModel)).toString();
    if (model.isEmpty())
        model = s.value(QString::fromUtf8(kLegacyModel), QString::fromUtf8(kDefaultModel)).toString();
    m_modelEdit->setText(model);

    const QString role = Persona::normalizeRole(s.value(QString::fromUtf8(kKeyRole), Persona::defaultRole()).toString());
    const int idx = m_roleCombo->findText(role);
    m_roleCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    QString tok = s.value(QString::fromUtf8(kKeyGwToken)).toString();
    if (tok.isEmpty())
        tok = OpenClawConfigSync::readGatewayToken();
    m_gatewayTokenEdit->setText(tok);
}

QString SettingsDialog::saveAndSyncOpenClaw(QString *errorOut)
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    const QString api = m_apiKeyEdit->text().trimmed();
    const QString model = m_modelEdit->text().trimmed().isEmpty()
                              ? QString::fromUtf8(kDefaultModel)
                              : m_modelEdit->text().trimmed();
    const QString role = Persona::normalizeRole(m_roleCombo->currentText());
    const QString gwToken = m_gatewayTokenEdit->text().trimmed();

    s.setValue(QString::fromUtf8(kKeyApi), api);
    s.setValue(QString::fromUtf8(kKeyModel), model);
    s.setValue(QString::fromUtf8(kKeyRole), role);
    s.setValue(QString::fromUtf8(kKeyGwToken), gwToken);

    OpenClawConfigSync::Input in;
    in.apiKey = api;
    in.model = model;
    in.role = role;
    const OpenClawConfigSync::Result r = OpenClawConfigSync::apply(in);
    if (!r.ok) {
        if (errorOut)
            *errorOut = r.error;
        return {};
    }
    return r.summary;
}

void SettingsDialog::onSave()
{
    QString err;
    const QString summary = saveAndSyncOpenClaw(&err);
    if (summary.isEmpty()) {
        // Still accept local QSettings even if OpenClaw home missing — surface error via reject? prefer accept + parent logs.
        // Keep dialog open on hard failure.
        if (!err.isEmpty()) {
            // Fall through: store QSettings already done inside saveAndSync only on success path.
            // Re-save QSettings alone:
            QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
            s.setValue(QString::fromUtf8(kKeyApi), m_apiKeyEdit->text().trimmed());
            s.setValue(QString::fromUtf8(kKeyModel),
                       m_modelEdit->text().trimmed().isEmpty()
                           ? QString::fromUtf8(kDefaultModel)
                           : m_modelEdit->text().trimmed());
            s.setValue(QString::fromUtf8(kKeyRole), Persona::normalizeRole(m_roleCombo->currentText()));
            s.setValue(QString::fromUtf8(kKeyGwToken), m_gatewayTokenEdit->text().trimmed());
            setProperty("openclawSyncError", err);
        }
    } else {
        setProperty("openclawSyncSummary", summary);
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
    QString api = s.value(QString::fromUtf8(kKeyApi)).toString();
    if (api.isEmpty())
        api = s.value(QString::fromUtf8(kLegacyApi)).toString();
    return api.trimmed();
}

QString SettingsDialog::model()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    QString model = s.value(QString::fromUtf8(kKeyModel)).toString();
    if (model.isEmpty())
        model = s.value(QString::fromUtf8(kLegacyModel), QString::fromUtf8(kDefaultModel)).toString();
    return model.trimmed().isEmpty() ? QString::fromUtf8(kDefaultModel) : model.trimmed();
}

QString SettingsDialog::role()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    return Persona::normalizeRole(s.value(QString::fromUtf8(kKeyRole), Persona::defaultRole()).toString());
}

QString SettingsDialog::gatewayToken()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    QString tok = s.value(QString::fromUtf8(kKeyGwToken)).toString().trimmed();
    if (tok.isEmpty())
        tok = OpenClawConfigSync::readGatewayToken();
    return tok;
}
