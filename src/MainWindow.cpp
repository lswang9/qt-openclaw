#include "MainWindow.h"
#include "CharacterSelectDialog.h"
#include "GatewayChatClient.h"
#include "GatewayConfig.h"
#include "LocalGatewayProcess.h"
#include "RoleLoader.h"
#include "SettingsDialog.h"

#include <QCloseEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr auto kOrg = "qt-openclaw";
constexpr auto kApp = "qt-openclaw";
constexpr auto kKeyRole = "ui/roleId";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chat(new GatewayChatClient(this))
    , m_gateway(new LocalGatewayProcess(this))
{
    m_gateway->setPort(GatewayConfig::kDefaultGatewayPort);
    setupUi();
    updateLogVisibility();

    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    const QString roleId = s.value(QString::fromUtf8(kKeyRole),
                                   QString::fromUtf8(RoleLoader::kDefaultRoleId)).toString();
    applyRole(roleId, true);
    SettingsDialog::loadInto(m_chat);

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(m_switchRoleButton, &QPushButton::clicked, this, &MainWindow::onSwitchRoleClicked);

    connect(m_chat, &GatewayChatClient::assistantMessage, this, &MainWindow::onAssistantMessage);
    connect(m_chat, &GatewayChatClient::errorOccurred, this, &MainWindow::onChatError);
    connect(m_chat, &GatewayChatClient::requestStarted, this, &MainWindow::onChatBusyChanged);
    connect(m_chat, &GatewayChatClient::requestFinished, this, &MainWindow::onChatBusyChanged);
    connect(m_chat, &GatewayChatClient::logMessage, this, &MainWindow::appendLog);

    connect(m_gateway, &LocalGatewayProcess::logMessage, this, &MainWindow::appendLog);
    connect(m_gateway, &LocalGatewayProcess::statusChanged, this, &MainWindow::setStatus);
    connect(m_gateway, &LocalGatewayProcess::started, this, &MainWindow::onGatewayStarted);
    connect(m_gateway, &LocalGatewayProcess::stopped, this, [this](int, QProcess::ExitStatus) {
        onGatewayStopped();
    });

    if (!SettingsDialog::hasApiKey())
        appendLog(tr("[settings] DeepSeek Key not configured yet"));
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
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    refreshBackground();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_chatChrome && event->type() == QEvent::Resize)
        refreshBackground();
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("器灵 Agent"));
    resize(920, 680);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    m_chatChrome = new QWidget(central);
    m_chatChrome->installEventFilter(this);
    m_chatChrome->setMinimumHeight(420);

    m_bgLabel = new QLabel(m_chatChrome);
    m_bgLabel->setScaledContents(false);
    m_bgLabel->setAlignment(Qt::AlignCenter);
    m_bgLabel->lower();

    auto *overlay = new QWidget(m_chatChrome);
    overlay->setAttribute(Qt::WA_TranslucentBackground);

    m_statusLabel = new QLabel(tr("未连接"), overlay);
    m_statusLabel->setStyleSheet(QStringLiteral(
        "color: #30d158; background: rgba(0,0,0,140); padding: 4px 10px; border-radius: 8px;"));

    m_roleNameLabel = new QLabel(overlay);
    m_roleNameLabel->setStyleSheet(QStringLiteral(
        "color: white; font-size: 18px; font-weight: 700;"
        " background: rgba(0,0,0,120); padding: 4px 12px; border-radius: 8px;"));

    m_avatarLabel = new QLabel(overlay);
    m_avatarLabel->setFixedSize(72, 72);
    m_avatarLabel->setStyleSheet(QStringLiteral(
        "border-radius: 12px; border: 2px solid rgba(255,255,255,180); background: rgba(0,0,0,80);"));
    m_avatarLabel->setScaledContents(true);

    m_chatList = new QListWidget(overlay);
    m_chatList->setWordWrap(true);
    m_chatList->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; border: none; color: white; }"
        "QListWidget::item { margin: 6px; padding: 8px 12px; border-radius: 12px; }"
        "QListWidget::item:selected { background: rgba(255,255,255,40); }"));

    m_chatInput = new QPlainTextEdit(overlay);
    m_chatInput->setPlaceholderText(tr("说点什么…"));
    m_chatInput->setMaximumHeight(72);
    m_chatInput->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { background: rgba(28,28,30,210); color: white; border-radius: 18px;"
        " padding: 10px 14px; border: 1px solid rgba(255,255,255,40); }"));

    m_sendButton = new QPushButton(tr("发送"), overlay);
    m_connectButton = new QPushButton(tr("连上"), overlay);
    m_switchRoleButton = new QPushButton(tr("换角色"), overlay);
    m_settingsButton = new QPushButton(tr("设置"), overlay);
    for (QPushButton *b : {m_sendButton, m_connectButton, m_switchRoleButton, m_settingsButton}) {
        b->setStyleSheet(QStringLiteral(
            "QPushButton { background: #0a84ff; color: white; border: none; border-radius: 10px;"
            " padding: 8px 14px; font-weight: 600; }"
            "QPushButton:hover { background: #409cff; }"
            "QPushButton:disabled { background: #555; }"));
    }

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_statusLabel);
    topRow->addStretch();
    topRow->addWidget(m_roleNameLabel);
    topRow->addWidget(m_avatarLabel);
    topRow->addWidget(m_connectButton);
    topRow->addWidget(m_switchRoleButton);
    topRow->addWidget(m_settingsButton);

    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(m_chatInput, 1);
    inputRow->addWidget(m_sendButton);

    auto *overlayLayout = new QVBoxLayout(overlay);
    overlayLayout->setContentsMargins(16, 16, 16, 16);
    overlayLayout->addLayout(topRow);
    overlayLayout->addWidget(m_chatList, 1);
    overlayLayout->addLayout(inputRow);

    auto *chromeLayout = new QVBoxLayout(m_chatChrome);
    chromeLayout->setContentsMargins(0, 0, 0, 0);
    chromeLayout->addWidget(overlay);

    m_logView = new QPlainTextEdit(central);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(2000);
    m_logView->setMaximumHeight(140);
    m_logView->setPlaceholderText(tr("技术日志…"));
    m_logView->setStyleSheet(QStringLiteral("font-size: 11px; color: #888;"));
    m_logView->hide();

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chatChrome, 1);
    layout->addWidget(m_logView);
}

void MainWindow::updateLogVisibility()
{
    if (m_logView)
        m_logView->setVisible(SettingsDialog::showTechLog());
}

QString MainWindow::plainChatError(const QString &raw) const
{
    const QString lower = raw.toLower();
    if (lower.contains(QLatin1String("api key")) || lower.contains(QLatin1String("token"))
        || raw.contains(QStringLiteral("Key")))
        return tr("还没准备好，请先在「设置」里填入 API Key。");
    if (lower.contains(QLatin1String("connection")) || lower.contains(QLatin1String("refused"))
        || lower.contains(QLatin1String("timed out")) || raw.contains(QStringLiteral("连不上")))
        return tr("暂时连不上，请先点「连上」，稍后再试。");
    if (raw.contains(QStringLiteral("choices")) || raw.contains(QStringLiteral("空消息")))
        return tr("没有收到回复，请稍后再试。");
    return tr("出了点问题，请稍后再试。");
}

void MainWindow::applyRole(const QString &roleId, bool resetChat)
{
    const RoleInfo role = RoleLoader::loadRole(roleId);
    m_roleId = role.id;
    m_roleDisplayName = role.displayName;

    QSettings s(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
    s.setValue(QString::fromUtf8(kKeyRole), m_roleId);

    m_roleNameLabel->setText(m_roleDisplayName);
    if (resetChat) {
        m_chat->clearHistory();
        m_chatList->clear();
        appendChatBubble(m_roleDisplayName, role.welcomeMessage, false);
    }

    QPixmap av(role.avatarPath);
    if (!av.isNull())
        m_avatarLabel->setPixmap(av.scaled(72, 72, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    else
        m_avatarLabel->setText(m_roleDisplayName.left(1));

    m_bgPixmap = QPixmap(role.backgroundPath);
    refreshBackground();
    m_chatInput->setPlaceholderText(tr("跟%1说点什么…").arg(m_roleDisplayName));
}

void MainWindow::refreshBackground()
{
    if (!m_bgLabel || !m_chatChrome)
        return;
    m_bgLabel->setGeometry(m_chatChrome->rect());
    if (m_bgPixmap.isNull()) {
        m_bgLabel->setStyleSheet(QStringLiteral("background: #1c1c1e;"));
        m_bgLabel->clear();
        return;
    }
    const QSize sz = m_chatChrome->size();
    if (sz.width() < 2 || sz.height() < 2)
        return;
    m_bgLabel->setPixmap(m_bgPixmap.scaled(sz, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_bgLabel->lower();
}

void MainWindow::ensureApiKeyOrPrompt()
{
    if (!SettingsDialog::hasApiKey()) {
        SettingsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            SettingsDialog::loadInto(m_chat);
            updateLogVisibility();
        }
    }
}

void MainWindow::onConnectClicked()
{
    ensureApiKeyOrPrompt();
    if (!SettingsDialog::hasApiKey()) {
        QMessageBox::information(this, tr("需要 API Key"),
                                 tr("请先在「设置」里填入 DeepSeek API Key。"));
        appendLog(tr("[chat] need DeepSeek Key before connect"));
        return;
    }
    QString err;
    if (!GatewayConfig::configureDeepSeekViaCli(SettingsDialog::apiKey(), &err))
        appendLog(tr("[config] %1").arg(err));
    SettingsDialog::loadInto(m_chat);

    if (m_gateway->isRunning()) {
        setStatus(tr("已就绪"));
        return;
    }
    const QString cli = m_gateway->resolvedCliPath();
    if (cli.isEmpty()) {
        appendLog(tr("[gateway] %1").arg(m_gateway->resolveError()));
        QMessageBox::warning(this, tr("无法启动"),
                             tr("暂时无法启动本地服务。请确认已完成安装后再试。"));
        return;
    }
    m_gateway->start();
}

void MainWindow::onGatewayStarted()
{
    setStatus(tr("已就绪"));
    m_connectButton->setText(tr("已连上"));
}

void MainWindow::onGatewayStopped()
{
    setStatus(tr("未连接"));
    m_connectButton->setText(tr("连上"));
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        SettingsDialog::loadInto(m_chat);
        updateLogVisibility();
        appendLog(tr("[settings] Key saved"));
    } else {
        updateLogVisibility();
    }
}

void MainWindow::onSwitchRoleClicked()
{
    CharacterSelectDialog dlg(m_roleId, this);
    if (dlg.exec() == QDialog::Accepted && dlg.selectedRoleId() != m_roleId)
        applyRole(dlg.selectedRoleId(), true);
}

void MainWindow::onSendClicked()
{
    ensureApiKeyOrPrompt();
    if (!SettingsDialog::hasApiKey()) {
        QMessageBox::information(this, tr("需要 API Key"),
                                 tr("请先在「设置」里填入 DeepSeek API Key。"));
        appendLog(tr("[chat] aborted: no API key"));
        return;
    }
    if (!m_gateway->isRunning()) {
        onConnectClicked();
        if (!m_gateway->isRunning()) {
            QMessageBox::information(this, tr("请先连上"),
                                     tr("请先点「连上」，连上后再说话。"));
            return;
        }
    }

    SettingsDialog::loadInto(m_chat);
    const QString text = m_chatInput->toPlainText().trimmed();
    if (text.isEmpty())
        return;
    appendChatBubble(tr("你"), text, true);
    m_chatInput->clear();
    m_chat->sendUserMessage(text);
    onChatBusyChanged();
}

void MainWindow::onAssistantMessage(const QString &text)
{
    appendChatBubble(m_roleDisplayName, text, false);
}

void MainWindow::onChatError(const QString &message)
{
    appendLog(QStringLiteral("[chat] %1").arg(message));
    appendChatBubble(tr("提示"), plainChatError(message), false);
}

void MainWindow::onChatBusyChanged()
{
    m_sendButton->setEnabled(!m_chat->isBusy());
}

void MainWindow::appendChatBubble(const QString &who, const QString &text, bool fromUser)
{
    const QString prefix = fromUser ? QStringLiteral("→ ") : QStringLiteral("← ");
    auto *item = new QListWidgetItem(QStringLiteral("%1[%2] %3").arg(prefix, who, text));
    item->setTextAlignment(fromUser ? Qt::AlignRight : Qt::AlignLeft);
    m_chatList->addItem(item);
    m_chatList->scrollToBottom();
}

void MainWindow::appendLog(const QString &line)
{
    if (m_logView)
        m_logView->appendPlainText(line);
}

void MainWindow::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}
