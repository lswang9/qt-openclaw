#include "MainWindow.h"
#include "GatewayConfig.h"
#include "LocalGatewayProcess.h"
#include "SettingsDialog.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gateway(new LocalGatewayProcess(this))
{
    m_gateway->setPort(GatewayConfig::kDefaultGatewayPort);
    setupUi();
    updateLogVisibility();

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(m_reloadButton, &QPushButton::clicked, this, &MainWindow::onReloadUiClicked);

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

void MainWindow::activateFromSecondInstance()
{
    show();
    raise();
    activateWindow();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_gateway && m_gateway->isRunning())
        m_gateway->stop();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("器灵 Agent"));
    resize(1100, 760);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    m_statusLabel = new QLabel(tr("未连接"), central);
    m_statusLabel->setStyleSheet(QStringLiteral(
        "color: #30d158; background: rgba(0,0,0,140); padding: 6px 12px; border-radius: 8px;"
        " font-weight: 600;"));

    m_connectButton = new QPushButton(tr("连上"), central);
    m_disconnectButton = new QPushButton(tr("断开"), central);
    m_settingsButton = new QPushButton(tr("设置"), central);
    m_reloadButton = new QPushButton(tr("刷新控制台"), central);
    m_disconnectButton->setEnabled(false);
    m_reloadButton->setEnabled(false);

    const QString btnCss = QStringLiteral(
        "QPushButton { background: #0a84ff; color: white; border: none; border-radius: 10px;"
        " padding: 10px 18px; font-size: 15px; font-weight: 600; min-height: 36px; }"
        "QPushButton:hover { background: #409cff; }"
        "QPushButton:disabled { background: #555; }");
    for (QPushButton *b : {m_connectButton, m_disconnectButton, m_settingsButton, m_reloadButton})
        b->setStyleSheet(btnCss);
    m_connectButton->setMinimumWidth(120);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_statusLabel);
    topRow->addStretch();
    topRow->addWidget(m_connectButton);
    topRow->addWidget(m_disconnectButton);
    topRow->addWidget(m_reloadButton);
    topRow->addWidget(m_settingsButton);

    m_webView = new QWebEngineView(central);
    m_webView->setUrl(QUrl(QStringLiteral("about:blank")));

    m_logView = new QPlainTextEdit(central);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(2000);
    m_logView->setMaximumHeight(120);
    m_logView->setPlaceholderText(tr("技术日志…"));
    m_logView->setStyleSheet(QStringLiteral("font-size: 11px; color: #888;"));
    m_logView->hide();

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    layout->addLayout(topRow);
    layout->addWidget(m_webView, 1);
    layout->addWidget(m_logView);
}

void MainWindow::updateLogVisibility()
{
    if (m_logView)
        m_logView->setVisible(SettingsDialog::showTechLog());
}

void MainWindow::ensureApiKeyOrPrompt()
{
    if (!SettingsDialog::hasApiKey()) {
        SettingsDialog dlg(this);
        dlg.exec();
        updateLogVisibility();
    }
}

void MainWindow::loadControlUi()
{
    const QUrl url{QUrl(GatewayConfig::controlUiUrl())};
    appendLog(tr("[ui] loading Control UI %1").arg(url.toString()));
    m_webView->setUrl(url);
    m_reloadButton->setEnabled(true);
}

void MainWindow::onConnectClicked()
{
    ensureApiKeyOrPrompt();
    if (!SettingsDialog::hasApiKey()) {
        QMessageBox::information(this, tr("需要 API Key"),
                                 tr("请先在「设置」里填入 DeepSeek API Key。"));
        return;
    }

    QString err;
    if (!GatewayConfig::configureDeepSeekViaCli(SettingsDialog::apiKey(), &err))
        appendLog(tr("[config] %1").arg(err));

    if (!GatewayConfig::ensureQilingBrandPlugin(&err))
        appendLog(tr("[theme] %1").arg(err));
    else
        appendLog(tr("[theme] qiling-brand install/enable ok"));

    if (m_gateway->isRunning()) {
        setStatus(tr("已就绪"));
        loadControlUi();
        return;
    }

    if (m_gateway->resolvedCliPath().isEmpty()) {
        appendLog(tr("[gateway] %1").arg(m_gateway->resolveError()));
        QMessageBox::warning(this, tr("无法启动"),
                             tr("暂时无法启动本地 Gateway。请确认 openclaw 已安装。"));
        return;
    }
    m_gateway->start();
}

void MainWindow::onDisconnectClicked()
{
    if (m_gateway && m_gateway->isRunning())
        m_gateway->stop();
    m_webView->setUrl(QUrl(QStringLiteral("about:blank")));
}

void MainWindow::onReloadUiClicked()
{
    if (m_gateway && m_gateway->isRunning())
        loadControlUi();
}

void MainWindow::onGatewayStarted()
{
    setStatus(tr("已就绪"));
    m_connectButton->setText(tr("已连上"));
    m_connectButton->setEnabled(false);
    m_disconnectButton->setEnabled(true);
    QTimer::singleShot(800, this, &MainWindow::loadControlUi);
}

void MainWindow::onGatewayStopped()
{
    setStatus(tr("未连接"));
    m_connectButton->setText(tr("连上"));
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);
    m_reloadButton->setEnabled(false);
    m_webView->setUrl(QUrl(QStringLiteral("about:blank")));
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        appendLog(tr("[settings] saved"));
    updateLogVisibility();
}

void MainWindow::appendLog(const QString &line)
{
    if (m_logView)
        m_logView->appendPlainText(line);
}

void MainWindow::setStatus(const QString &text)
{
    if (m_statusLabel)
        m_statusLabel->setText(text);
}
