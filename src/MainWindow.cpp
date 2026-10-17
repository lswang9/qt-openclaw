#include "MainWindow.h"
#include "GatewayClient.h"
#include "LocalGatewayProcess.h"
#include "DeepSeekClient.h"
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
    , m_chat(new DeepSeekClient(this))
{
    setupUi();
    SettingsDialog::loadInto(m_chat);

    connect(m_localRadio, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(m_startGatewayButton, &QPushButton::clicked, this, &MainWindow::onStartGatewayClicked);
    connect(m_stopGatewayButton, &QPushButton::clicked, this, &MainWindow::onStopGatewayClicked);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    connect(m_client, &GatewayClient::logMessage, this, &MainWindow::appendLog);
    connect(m_client, &GatewayClient::statusChanged, this, &MainWindow::setStatus);

    connect(m_gateway, &LocalGatewayProcess::logMessage, this, &MainWindow::appendLog);
    connect(m_gateway, &LocalGatewayProcess::statusChanged, this, &MainWindow::setStatus);
    connect(m_gateway, &LocalGatewayProcess::started, this, &MainWindow::onGatewayStarted);
    connect(m_gateway, &LocalGatewayProcess::stopped, this, [this](int, QProcess::ExitStatus) {
        onGatewayStopped();
    });
    // Mark started when process actually leaves NotRunning after start().
    connect(m_gateway, &LocalGatewayProcess::statusChanged, this, [this](const QString &s) {
        if (s.contains(QStringLiteral("starting")))
            updateGatewayButtons();
    });

    connect(m_chat, &DeepSeekClient::assistantMessage, this, &MainWindow::onAssistantMessage);
    connect(m_chat, &DeepSeekClient::errorOccurred, this, &MainWindow::onChatError);
    connect(m_chat, &DeepSeekClient::requestStarted, this, &MainWindow::onChatBusyChanged);
    connect(m_chat, &DeepSeekClient::requestFinished, this, &MainWindow::onChatBusyChanged);

    updateModeUi();
    updateGatewayButtons();

    // First-run: nudge to set DeepSeek key (non-blocking after UI is up).
    if (!SettingsDialog::hasApiKey()) {
        appendLog(tr("[settings] No DeepSeek API key yet — open Settings to chat."));
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
    setWindowTitle(tr("qt-openclaw — OpenClaw Gateway"));
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

    // Local gateway controls
    m_localGroup = new QGroupBox(tr("Local Gateway process"), central);
    m_startGatewayButton = new QPushButton(tr("Start Gateway"), m_localGroup);
    m_stopGatewayButton = new QPushButton(tr("Stop Gateway"), m_localGroup);
    m_gatewayStatusLabel = new QLabel(tr("Stopped"), m_localGroup);
    auto *localBtnRow = new QHBoxLayout;
    localBtnRow->addWidget(m_startGatewayButton);
    localBtnRow->addWidget(m_stopGatewayButton);
    localBtnRow->addWidget(m_gatewayStatusLabel, /*stretch=*/1);
    auto *localHint = new QLabel(
        tr("Runs `openclaw gateway run --port 18789` (default ws://127.0.0.1:18789). "
           "Populate ./runtime or install openclaw on PATH — see README."),
        m_localGroup);
    localHint->setWordWrap(true);
    auto *localLayout = new QVBoxLayout(m_localGroup);
    localLayout->addWidget(localHint);
    localLayout->addLayout(localBtnRow);

    // Remote / WS stub connect
    m_remoteGroup = new QGroupBox(tr("Gateway WebSocket (stub)"), central);
    m_urlEdit = new QLineEdit(m_remoteGroup);
    m_urlEdit->setPlaceholderText(QStringLiteral("ws://127.0.0.1:18789"));
    m_urlEdit->setText(QStringLiteral("ws://127.0.0.1:18789"));
    m_tokenEdit = new QLineEdit(m_remoteGroup);
    m_tokenEdit->setEchoMode(QLineEdit::Password);
    m_tokenEdit->setPlaceholderText(tr("Gateway token (optional)"));
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

    // Chat
    auto *chatGroup = new QGroupBox(
        tr("Chat (DeepSeek direct — OpenAI-compatible API)"), central);
    m_chatList = new QListWidget(chatGroup);
    m_chatList->setWordWrap(true);
    m_chatInput = new QPlainTextEdit(chatGroup);
    m_chatInput->setPlaceholderText(tr("Type a message… (Enter to send, Shift+Enter for newline via Send button)"));
    m_chatInput->setMaximumHeight(80);
    m_sendButton = new QPushButton(tr("Send"), chatGroup);
    auto *chatInputRow = new QHBoxLayout;
    chatInputRow->addWidget(m_chatInput, /*stretch=*/1);
    chatInputRow->addWidget(m_sendButton);
    auto *chatLayout = new QVBoxLayout(chatGroup);
    chatLayout->addWidget(m_chatList, /*stretch=*/1);
    chatLayout->addLayout(chatInputRow);

    // Log
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
    // Remote fields always usable for stub connect; in local mode lock URL to default.
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
    m_urlEdit->setText(m_gateway->defaultUrl());
    m_gateway->start();
    // QProcess::start is async; poll state shortly via status hooks.
    updateGatewayButtons();
    // If start succeeded, process state becomes Running quickly.
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
}

void MainWindow::onGatewayStopped()
{
    updateGatewayButtons();
    m_gatewayStatusLabel->setText(tr("Stopped"));
}

void MainWindow::onConnectClicked()
{
    const QString url = m_urlEdit->text().trimmed();
    const QString token = m_tokenEdit->text();
    m_client->connectToGateway(url, token);
    m_connectButton->setEnabled(false);
    m_disconnectButton->setEnabled(true);
}

void MainWindow::onDisconnectClicked()
{
    m_client->disconnectFromGateway();
    if (isLocalMode() && m_gateway->isRunning()) {
        // Graceful stop on Disconnect in local mode (per MVP lifecycle).
        m_gateway->stop();
        updateGatewayButtons();
    }
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);
}

void MainWindow::ensureApiKeyOrPrompt()
{
    if (!m_chat->isConfigured()) {
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted)
            SettingsDialog::loadInto(m_chat);
    }
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        SettingsDialog::loadInto(m_chat);
        appendLog(tr("[settings] DeepSeek config saved locally (QSettings)."));
    }
}

void MainWindow::onSendClicked()
{
    ensureApiKeyOrPrompt();
    if (!m_chat->isConfigured()) {
        appendLog(tr("[chat] aborted: no API key"));
        return;
    }

    const QString text = m_chatInput->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    appendChatBubble(tr("You"), text);
    m_chatInput->clear();
    m_chat->sendUserMessage(text);
    onChatBusyChanged();
}

void MainWindow::onAssistantMessage(const QString &text)
{
    appendChatBubble(tr("Assistant"), text);
}

void MainWindow::onChatError(const QString &message)
{
    appendLog(QStringLiteral("[chat] %1").arg(message));
    appendChatBubble(tr("Error"), message);
}

void MainWindow::onChatBusyChanged()
{
    m_sendButton->setEnabled(!m_chat->isBusy());
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
