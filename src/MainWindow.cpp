#include "MainWindow.h"
#include "GatewayClient.h"
#include "LocalGatewayProcess.h"
#include "OpenClawConfigSync.h"
#include "Persona.h"
#include "SettingsDialog.h"

#include <QCloseEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new GatewayClient(this))
    , m_gateway(new LocalGatewayProcess(this))
{
    setupUi();
    applySettingsToClient();

    connect(m_localRadio, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(m_startGatewayButton, &QPushButton::clicked, this, &MainWindow::onStartGatewayClicked);
    connect(m_stopGatewayButton, &QPushButton::clicked, this, &MainWindow::onStopGatewayClicked);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    connect(m_client, &GatewayClient::logMessage, this, &MainWindow::appendLog);
    connect(m_client, &GatewayClient::statusChanged, this, &MainWindow::setStatus);
    connect(m_client, &GatewayClient::assistantMessage, this, &MainWindow::onAssistantMessage);
    connect(m_client, &GatewayClient::errorOccurred, this, &MainWindow::onChatError);
    connect(m_client, &GatewayClient::requestStarted, this, &MainWindow::onChatBusyChanged);
    connect(m_client, &GatewayClient::requestFinished, this, &MainWindow::onChatBusyChanged);

    connect(m_gateway, &LocalGatewayProcess::logMessage, this, &MainWindow::appendLog);
    connect(m_gateway, &LocalGatewayProcess::statusChanged, this, &MainWindow::setStatus);
    connect(m_gateway, &LocalGatewayProcess::started, this, &MainWindow::onGatewayStarted);
    connect(m_gateway, &LocalGatewayProcess::stopped, this, [this](int, QProcess::ExitStatus) {
        onGatewayStopped();
    });
    connect(m_gateway, &LocalGatewayProcess::statusChanged, this, [this](const QString &s) {
        if (s.contains(QStringLiteral("starting")))
            updateGatewayButtons();
    });

    updateModeUi();
    updateGatewayButtons();

    // Welcome uses role name (巴巴塔), never product name 器灵 as self-intro
    appendChatBubble(SettingsDialog::role(), Persona::welcomeMessage(SettingsDialog::role()));

    if (!SettingsDialog::hasApiKey()) {
        appendLog(tr("[settings] No DeepSeek API key yet — open Settings to configure Gateway."));
    }
}

MainWindow::~MainWindow()
{
    if (m_gateway)
        m_gateway->stop();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_gateway && m_gateway->isRunning())
        m_gateway->stop();
    m_client->disconnectFromGateway();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("%1 — OpenClaw Gateway").arg(Persona::productName()));
    resize(900, 640);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    m_localRadio = new QRadioButton(tr("Local Gateway"), central);
    m_remoteRadio = new QRadioButton(tr("Remote Gateway"), central);
    m_localRadio->setChecked(true);

    auto *modeRow = new QHBoxLayout;
    modeRow->addWidget(new QLabel(tr("Mode:"), central));
    modeRow->addWidget(m_localRadio);
    modeRow->addWidget(m_remoteRadio);
    modeRow->addStretch();
    m_settingsButton = new QPushButton(tr("Settings…"), central);
    modeRow->addWidget(m_settingsButton);

    m_localGroup = new QGroupBox(tr("Local Gateway process"), central);
    m_startGatewayButton = new QPushButton(tr("Start Gateway"), m_localGroup);
    m_stopGatewayButton = new QPushButton(tr("Stop Gateway"), m_localGroup);
    m_gatewayStatusLabel = new QLabel(tr("Stopped"), m_localGroup);
    auto *localBtnRow = new QHBoxLayout;
    localBtnRow->addWidget(m_startGatewayButton);
    localBtnRow->addWidget(m_stopGatewayButton);
    localBtnRow->addWidget(m_gatewayStatusLabel, /*stretch=*/1);
    auto *localHint = new QLabel(
        tr("Runs `openclaw gateway run --port 18789`. On Start, DeepSeek key/model/role "
           "are written into ~/.openclaw for the Gateway. Chat uses Gateway HTTP, not DeepSeek direct."),
        m_localGroup);
    localHint->setWordWrap(true);
    auto *localLayout = new QVBoxLayout(m_localGroup);
    localLayout->addWidget(localHint);
    localLayout->addLayout(localBtnRow);

    m_remoteGroup = new QGroupBox(tr("Gateway connection"), central);
    m_urlEdit = new QLineEdit(m_remoteGroup);
    m_urlEdit->setPlaceholderText(QStringLiteral("ws://127.0.0.1:18789"));
    m_urlEdit->setText(QStringLiteral("ws://127.0.0.1:18789"));
    m_tokenEdit = new QLineEdit(m_remoteGroup);
    m_tokenEdit->setEchoMode(QLineEdit::Password);
    m_tokenEdit->setPlaceholderText(tr("Gateway token (optional; else from openclaw.json)"));
    m_connectButton = new QPushButton(tr("Connect"), m_remoteGroup);
    m_disconnectButton = new QPushButton(tr("Disconnect"), m_remoteGroup);
    m_disconnectButton->setEnabled(false);
    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_connectButton);
    buttonRow->addWidget(m_disconnectButton);
    buttonRow->addStretch();
    auto *form = new QFormLayout;
    form->addRow(tr("Gateway URL"), m_urlEdit);
    form->addRow(tr("Token"), m_tokenEdit);
    form->addRow(QString(), buttonRow);
    m_remoteGroup->setLayout(form);

    auto *chatGroup = new QGroupBox(
        tr("Chat（经本地 Gateway，非 DeepSeek 直连）"), central);
    m_chatList = new QListWidget(chatGroup);
    m_chatList->setWordWrap(true);
    m_chatInput = new QPlainTextEdit(chatGroup);
    m_chatInput->setPlaceholderText(tr("跟%1说点什么…").arg(SettingsDialog::role()));
    m_chatInput->setMaximumHeight(80);
    m_sendButton = new QPushButton(tr("Send"), chatGroup);
    auto *chatInputRow = new QHBoxLayout;
    chatInputRow->addWidget(m_chatInput, /*stretch=*/1);
    chatInputRow->addWidget(m_sendButton);
    auto *chatLayout = new QVBoxLayout(chatGroup);
    chatLayout->addWidget(m_chatList, /*stretch=*/1);
    chatLayout->addLayout(chatInputRow);

    m_logView = new QPlainTextEdit(central);
    m_logView->setReadOnly(true);
    m_logView->setPlaceholderText(tr("Gateway / connection log…"));
    m_logView->setMaximumBlockCount(2000);

    m_statusLabel = new QLabel(tr("Ready"), central);

    auto *splitter = new QSplitter(Qt::Vertical, central);
    splitter->addWidget(chatGroup);
    splitter->addWidget(m_logView);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(central);
    layout->addLayout(modeRow);
    layout->addWidget(m_localGroup);
    layout->addWidget(m_remoteGroup);
    layout->addWidget(splitter, /*stretch=*/1);
    layout->addWidget(m_statusLabel);
}

void MainWindow::applySettingsToClient()
{
    const QString role = SettingsDialog::role();
    m_client->setSystemPrompt(Persona::systemPrompt(role));
    m_client->setBackendModel(SettingsDialog::model());
    m_client->setGatewayToken(SettingsDialog::gatewayToken());
    m_client->setGatewayHttpBase(OpenClawConfigSync::gatewayHttpBase(m_gateway->port()));
    if (m_tokenEdit && m_tokenEdit->text().trimmed().isEmpty())
        m_tokenEdit->setText(SettingsDialog::gatewayToken());
    if (m_chatInput)
        m_chatInput->setPlaceholderText(tr("跟%1说点什么…").arg(role));
    m_gateway->setDeepSeekApiKey(SettingsDialog::apiKey());
}

bool MainWindow::syncOpenClawFromSettings(QString *errorOut)
{
    if (!SettingsDialog::hasApiKey()) {
        if (errorOut)
            *errorOut = tr("No API key in Settings");
        return false;
    }
    OpenClawConfigSync::Input in;
    in.apiKey = SettingsDialog::apiKey();
    in.model = SettingsDialog::model();
    in.role = SettingsDialog::role();
    const OpenClawConfigSync::Result r = OpenClawConfigSync::apply(in);
    if (!r.ok) {
        if (errorOut)
            *errorOut = r.error;
        return false;
    }
    appendLog(QStringLiteral("[openclaw-config]\n%1").arg(r.summary));
    applySettingsToClient();
    return true;
}

bool MainWindow::isLocalMode() const
{
    return m_localRadio && m_localRadio->isChecked();
}

void MainWindow::onModeChanged()
{
    updateModeUi();
}

void MainWindow::updateModeUi()
{
    const bool local = isLocalMode();
    m_localGroup->setEnabled(local);
    if (local) {
        m_urlEdit->setText(m_gateway->defaultUrl());
        m_urlEdit->setReadOnly(true);
    } else {
        m_urlEdit->setReadOnly(false);
    }
}

void MainWindow::updateGatewayButtons()
{
    const bool running = m_gateway && m_gateway->isRunning();
    m_startGatewayButton->setEnabled(!running);
    m_stopGatewayButton->setEnabled(running);
    m_gatewayStatusLabel->setText(running ? tr("Running") : tr("Stopped"));
}

void MainWindow::onStartGatewayClicked()
{
    const QString cli = m_gateway->resolvedCliPath();
    if (cli.isEmpty()) {
        QMessageBox::warning(this, tr("OpenClaw not found"),
                             m_gateway->resolveError()
                                 + QStringLiteral("\n\n")
                                 + tr("Tip (CN): use npmmirror when installing — see README."));
        return;
    }

    QString err;
    if (SettingsDialog::hasApiKey()) {
        if (!syncOpenClawFromSettings(&err))
            appendLog(tr("[openclaw-config] sync failed: %1").arg(err));
    } else {
        appendLog(tr("[openclaw-config] skipped (no API key) — chat needs Settings first"));
    }

    m_urlEdit->setText(m_gateway->defaultUrl());
    m_gateway->setDeepSeekApiKey(SettingsDialog::apiKey());
    m_gateway->start();
    updateGatewayButtons();
    if (m_gateway->isRunning())
        onGatewayStarted();
}

void MainWindow::onStopGatewayClicked()
{
    m_gateway->stop();
    updateGatewayButtons();
}

void MainWindow::onGatewayStarted()
{
    updateGatewayButtons();
    m_gatewayStatusLabel->setText(tr("Running"));
    // Auto-wire chat client to local gateway HTTP
    m_client->setGatewayHttpBase(OpenClawConfigSync::gatewayHttpBase(m_gateway->port()));
    m_client->connectToGateway(m_gateway->defaultUrl(),
                               m_tokenEdit->text().trimmed().isEmpty()
                                   ? SettingsDialog::gatewayToken()
                                   : m_tokenEdit->text());
}

void MainWindow::onGatewayStopped()
{
    updateGatewayButtons();
    m_gatewayStatusLabel->setText(tr("Stopped"));
}

void MainWindow::onConnectClicked()
{
    const QString url = m_urlEdit->text().trimmed();
    const QString token = m_tokenEdit->text().trimmed().isEmpty()
                              ? SettingsDialog::gatewayToken()
                              : m_tokenEdit->text();
    m_client->connectToGateway(url, token);
    m_connectButton->setEnabled(false);
    m_disconnectButton->setEnabled(true);
}

void MainWindow::onDisconnectClicked()
{
    m_client->disconnectFromGateway();
    if (isLocalMode() && m_gateway->isRunning()) {
        m_gateway->stop();
        updateGatewayButtons();
    }
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);
}

void MainWindow::ensureApiKeyOrPrompt()
{
    if (!SettingsDialog::hasApiKey()) {
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            const QString summary = dlg.property("openclawSyncSummary").toString();
            const QString syncErr = dlg.property("openclawSyncError").toString();
            if (!summary.isEmpty())
                appendLog(QStringLiteral("[openclaw-config]\n%1").arg(summary));
            if (!syncErr.isEmpty())
                appendLog(tr("[openclaw-config] %1").arg(syncErr));
            applySettingsToClient();
        }
    }
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString summary = dlg.property("openclawSyncSummary").toString();
        const QString syncErr = dlg.property("openclawSyncError").toString();
        if (!summary.isEmpty())
            appendLog(QStringLiteral("[openclaw-config]\n%1").arg(summary));
        if (!syncErr.isEmpty())
            appendLog(tr("[openclaw-config] %1").arg(syncErr));
        applySettingsToClient();
        appendLog(tr("[settings] Saved; DeepSeek key synced into OpenClaw Gateway config."));
        // Refresh welcome bubble role label on next clear — update placeholder only
        m_chatInput->setPlaceholderText(tr("跟%1说点什么…").arg(SettingsDialog::role()));
    }
}

void MainWindow::onSendClicked()
{
    ensureApiKeyOrPrompt();
    if (!SettingsDialog::hasApiKey()) {
        appendLog(tr("[chat] aborted: no API key for Gateway model config"));
        return;
    }

    // Ensure OpenClaw has latest key/persona before chat
    QString err;
    syncOpenClawFromSettings(&err);

    const QString text = m_chatInput->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    // Ensure HTTP base points at gateway (even without explicit Connect)
    if (!m_client->isConnected()) {
        m_client->connectToGateway(m_urlEdit->text().trimmed().isEmpty()
                                       ? m_gateway->defaultUrl()
                                       : m_urlEdit->text(),
                                   m_tokenEdit->text().trimmed().isEmpty()
                                       ? SettingsDialog::gatewayToken()
                                       : m_tokenEdit->text());
    }

    appendChatBubble(tr("You"), text);
    m_chatInput->clear();
    m_client->sendUserMessage(text);
    onChatBusyChanged();
}

void MainWindow::onAssistantMessage(const QString &text)
{
    appendChatBubble(SettingsDialog::role(), text);
}

void MainWindow::onChatError(const QString &message)
{
    appendLog(QStringLiteral("[chat] %1").arg(message));
    appendChatBubble(tr("Error"), message);
}

void MainWindow::onChatBusyChanged()
{
    m_sendButton->setEnabled(!m_client->isBusy());
}

void MainWindow::appendChatBubble(const QString &who, const QString &text)
{
    m_chatList->addItem(QStringLiteral("[%1] %2").arg(who, text));
    m_chatList->scrollToBottom();
}

void MainWindow::appendLog(const QString &line)
{
    m_logView->appendPlainText(line);
}

void MainWindow::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}
