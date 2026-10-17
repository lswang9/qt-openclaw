#pragma once

#include <QMainWindow>

class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QRadioButton;
class QListWidget;
class QGroupBox;
class GatewayClient;
class LocalGatewayProcess;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onModeChanged();
    void onStartGatewayClicked();
    void onStopGatewayClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onSendClicked();
    void onSettingsClicked();
    void onAssistantMessage(const QString &text);
    void onChatError(const QString &message);
    void onChatBusyChanged();
    void appendLog(const QString &line);
    void setStatus(const QString &text);
    void onGatewayStarted();
    void onGatewayStopped();

private:
    void setupUi();
    void updateModeUi();
    void updateGatewayButtons();
    void appendChatBubble(const QString &who, const QString &text);
    void ensureApiKeyOrPrompt();
    void applySettingsToClient();
    bool syncOpenClawFromSettings(QString *errorOut = nullptr);
    bool isLocalMode() const;

    QRadioButton *m_localRadio = nullptr;
    QRadioButton *m_remoteRadio = nullptr;
    QGroupBox *m_localGroup = nullptr;
    QGroupBox *m_remoteGroup = nullptr;
    QPushButton *m_startGatewayButton = nullptr;
    QPushButton *m_stopGatewayButton = nullptr;
    QLabel *m_gatewayStatusLabel = nullptr;
    QLineEdit *m_urlEdit = nullptr;
    QLineEdit *m_tokenEdit = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QListWidget *m_chatList = nullptr;
    QPlainTextEdit *m_chatInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QLabel *m_statusLabel = nullptr;

    GatewayClient *m_client = nullptr;
    LocalGatewayProcess *m_gateway = nullptr;
};
