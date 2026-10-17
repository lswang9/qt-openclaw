#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class GatewayChatClient;
class LocalGatewayProcess;
struct RoleInfo;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onConnectClicked();
    void onSendClicked();
    void onSettingsClicked();
    void onSwitchRoleClicked();
    void onAssistantMessage(const QString &text);
    void onChatError(const QString &message);
    void onChatBusyChanged();
    void appendLog(const QString &line);
    void setStatus(const QString &text);
    void onGatewayStarted();
    void onGatewayStopped();

private:
    void setupUi();
    void applyRole(const QString &roleId, bool resetChat);
    void refreshBackground();
    void appendChatBubble(const QString &who, const QString &text, bool fromUser);
    void ensureApiKeyOrPrompt();

    QWidget *m_chatChrome = nullptr;
    QLabel *m_bgLabel = nullptr;
    QLabel *m_avatarLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_roleNameLabel = nullptr;
    QListWidget *m_chatList = nullptr;
    QPlainTextEdit *m_chatInput = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_switchRoleButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPlainTextEdit *m_logView = nullptr;

    GatewayChatClient *m_chat = nullptr;
    LocalGatewayProcess *m_gateway = nullptr;
    QString m_roleId;
    QString m_roleDisplayName;
    QPixmap m_bgPixmap;
};
