#include "MainWindow.h"
#include "GatewayClient.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new GatewayClient(this))
{
    setupUi();

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_client, &GatewayClient::logMessage, this, &MainWindow::appendLog);
    connect(m_client, &GatewayClient::statusChanged, this, &MainWindow::setStatus);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    setWindowTitle(tr("qt-openclaw — OpenClaw Gateway"));
    resize(720, 480);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    m_urlEdit = new QLineEdit(central);
    m_urlEdit->setPlaceholderText(QStringLiteral("ws://127.0.0.1:18789"));
    m_urlEdit->setText(QStringLiteral("ws://127.0.0.1:18789"));

    m_tokenEdit = new QLineEdit(central);
    m_tokenEdit->setEchoMode(QLineEdit::Password);
    m_tokenEdit->setPlaceholderText(tr("Gateway token"));

    m_connectButton = new QPushButton(tr("Connect"), central);
    m_disconnectButton = new QPushButton(tr("Disconnect"), central);
    m_disconnectButton->setEnabled(false);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_connectButton);
    buttonRow->addWidget(m_disconnectButton);
    buttonRow->addStretch();

    auto *form = new QFormLayout;
    form->addRow(tr("Gateway URL"), m_urlEdit);
    form->addRow(tr("Token"), m_tokenEdit);
    form->addRow(QString(), buttonRow);

    m_logView = new QPlainTextEdit(central);
    m_logView->setReadOnly(true);
    m_logView->setPlaceholderText(tr("Log / chat placeholder…"));

    m_statusLabel = new QLabel(tr("Disconnected"), central);

    auto *layout = new QVBoxLayout(central);
    layout->addLayout(form);
    layout->addWidget(m_logView, /*stretch=*/1);
    layout->addWidget(m_statusLabel);
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
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);
}

void MainWindow::appendLog(const QString &line)
{
    m_logView->appendPlainText(line);
}

void MainWindow::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}
