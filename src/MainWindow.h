#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QString>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class GatewayChatClient;
class LocalGatewayProcess;
class SplashPage;
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
    void onStartChatClicked();
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
    void refreshSplash();
    void appendChatBubble(const QString &who, const QString &text, bool fromUser);
    void ensureApiKeyOrPrompt();
    void updateLogVisibility();
    QString plainChatError(const QString &raw) const;
    void showSplash();
    void showChat();

    QStackedWidget *m_stack = nullptr;
    SplashPage *m_splash = nullptr;

    QWidget *m_chatPage = nullptr;
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
    QPixmap m_avatarPixmap;
    QString m_welcomeMessage;
};
