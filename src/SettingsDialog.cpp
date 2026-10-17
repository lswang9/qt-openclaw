#include "SettingsDialog.h"
#include "DeepSeekClient.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>

namespace {
constexpr auto kOrg = "qt-openclaw";
constexpr auto kApp = "qt-openclaw";
constexpr auto kKeyApi = "deepseek/apiKey";
constexpr auto kKeyBase = "deepseek/baseUrl";
constexpr auto kKeyModel = "deepseek/model";
constexpr auto kDefaultBase = "https://api.deepseek.com";
constexpr auto kDefaultModel = "deepseek-flash";
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Model settings — DeepSeek"));
    setModal(true);
    resize(480, 220);

    auto *hint = new QLabel(
        tr("Enter your DeepSeek API key. Chat uses the OpenAI-compatible endpoint "
           "directly (not via Gateway) for this MVP.\n"
           "Get a key at https://platform.deepseek.com — stored locally in QSettings only."),
        this);
    hint->setWordWrap(true);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("sk-…"));

    m_baseUrlEdit = new QLineEdit(this);
    m_baseUrlEdit->setPlaceholderText(QString::fromUtf8(kDefaultBase));

    m_modelEdit = new QLineEdit(this);
    m_modelEdit->setPlaceholderText(QString::fromUtf8(kDefaultModel));

    auto *form = new QFormLayout;
    form->addRow(tr("API key"), m_apiKeyEdit);
    form->addRow(tr("Base URL"), m_baseUrlEdit);
    form->addRow(tr("Model"), m_modelEdit);

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
    m_apiKeyEdit->setText(s.value(QString::fromUtf8(kKeyApi)).toString());
    m_baseUrlEdit->setText(s.value(QString::fromUtf8(kKeyBase), QString::fromUtf8(kDefaultBase)).toString());
    m_modelEdit->setText(s.value(QString::fromUtf8(kKeyModel), QString::fromUtf8(kDefaultModel)).toString());
}

void SettingsDialog::onSave()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    s.setValue(QString::fromUtf8(kKeyApi), m_apiKeyEdit->text().trimmed());
    s.setValue(QString::fromUtf8(kKeyBase),
               m_baseUrlEdit->text().trimmed().isEmpty()
                   ? QString::fromUtf8(kDefaultBase)
                   : m_baseUrlEdit->text().trimmed());
    s.setValue(QString::fromUtf8(kKeyModel),
               m_modelEdit->text().trimmed().isEmpty()
                   ? QString::fromUtf8(kDefaultModel)
                   : m_modelEdit->text().trimmed());
    accept();
}

void SettingsDialog::loadInto(DeepSeekClient *client)
{
    if (!client)
        return;
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    client->setApiKey(s.value(QString::fromUtf8(kKeyApi)).toString());
    client->setBaseUrl(s.value(QString::fromUtf8(kKeyBase), QString::fromUtf8(kDefaultBase)).toString());
    client->setModel(s.value(QString::fromUtf8(kKeyModel), QString::fromUtf8(kDefaultModel)).toString());
}

bool SettingsDialog::hasApiKey()
{
    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    return !s.value(QString::fromUtf8(kKeyApi)).toString().trimmed().isEmpty();
}
