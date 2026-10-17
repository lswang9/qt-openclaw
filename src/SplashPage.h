#pragma once

#include <QWidget>
#include <QPixmap>
#include <QString>

class QLabel;
class QPushButton;

/** First screen: fullscreen bg + round avatar + 已就绪 + welcome + 开始聊天 + gear. */
class SplashPage : public QWidget
{
    Q_OBJECT
public:
    explicit SplashPage(QWidget *parent = nullptr);

    void setRole(const QString &displayName, const QString &welcome,
                 const QPixmap &avatar, const QPixmap &background);

signals:
    void startChatClicked();
    void settingsClicked();
    void avatarClicked();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void refreshBackground();
    static QPixmap makeCircular(const QPixmap &src, int size, const QColor &ring);

    QLabel *m_bgLabel = nullptr;
    QWidget *m_vignette = nullptr;
    QPushButton *m_avatarButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_welcomeLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPixmap m_bgPixmap;
};
