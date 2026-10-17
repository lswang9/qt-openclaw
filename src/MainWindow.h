#pragma once

#include <QMainWindow>
#include <QUrl>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QWebEngineView;
class LocalGatewayProcess;

/** Minimal shell: Gateway start/stop + embed Control UI in QWebEngineView. */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /** Bring window to front when a second instance launches. */
    void activateFromSecondInstance();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSettingsClicked();
    void onReloadUiClicked();
    void appendLog(const QString &line);
    void setStatus(const QString &text);
    void onGatewayStarted();
    void onGatewayStopped();

private:
    void setupUi();
    void updateLogVisibility();
    void loadControlUi();
    void ensureApiKeyOrPrompt();

    QLabel *m_statusLabel = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPushButton *m_reloadButton = nullptr;
    QWebEngineView *m_webView = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    LocalGatewayProcess *m_gateway = nullptr;
};
