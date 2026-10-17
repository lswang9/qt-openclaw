#pragma once

#include <QMainWindow>

class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class GatewayClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void appendLog(const QString &line);
    void setStatus(const QString &text);

private:
    void setupUi();

    QLineEdit *m_urlEdit = nullptr;
    QLineEdit *m_tokenEdit = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QLabel *m_statusLabel = nullptr;
    GatewayClient *m_client = nullptr;
};
