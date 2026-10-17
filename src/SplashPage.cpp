#include "SplashPage.h"

#include <QIcon>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

SplashPage::SplashPage(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(960, 640);

    m_bgLabel = new QLabel(this);
    m_bgLabel->setScaledContents(false);
    m_bgLabel->setAlignment(Qt::AlignCenter);
    m_bgLabel->lower();

    m_vignette = new QWidget(this);
    m_vignette->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_vignette->setStyleSheet(QStringLiteral("background: rgba(14, 18, 17, 120);"));

    m_settingsButton = new QPushButton(this);
    m_settingsButton->setFixedSize(40, 40);
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    m_settingsButton->setToolTip(tr("设置"));
    m_settingsButton->setText(QStringLiteral("⚙"));
    m_settingsButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgba(255,255,255,28); color: white; border: none;"
        " border-radius: 20px; font-size: 18px; }"
        "QPushButton:hover { background: rgba(255,255,255,48); }"));
    connect(m_settingsButton, &QPushButton::clicked, this, &SplashPage::settingsClicked);

    m_avatarButton = new QPushButton(this);
    m_avatarButton->setFixedSize(180, 180);
    m_avatarButton->setCursor(Qt::PointingHandCursor);
    m_avatarButton->setFlat(true);
    m_avatarButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; padding: 0; }"
        "QPushButton:hover { background: rgba(61,139,122,40); border-radius: 90px; }"));
    connect(m_avatarButton, &QPushButton::clicked, this, &SplashPage::avatarClicked);

    m_statusLabel = new QLabel(tr("✓  已就绪"), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(QStringLiteral(
        "color: #f5f5f7; font-size: 15px; font-weight: 600;"
        " background: rgba(28, 36, 34, 160); padding: 8px 18px; border-radius: 16px;"));

    m_welcomeLabel = new QLabel(this);
    m_welcomeLabel->setAlignment(Qt::AlignCenter);
    m_welcomeLabel->setWordWrap(true);
    m_welcomeLabel->setStyleSheet(QStringLiteral(
        "color: white; font-size: 28px; font-weight: 600; letter-spacing: 1px;"));

    m_startButton = new QPushButton(tr("开始聊天  ›"), this);
    m_startButton->setCursor(Qt::PointingHandCursor);
    m_startButton->setMinimumWidth(220);
    m_startButton->setFixedHeight(48);
    m_startButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #2a5f55; color: white; border: 1px solid #3D8B7A;"
        " border-radius: 24px; font-size: 17px; font-weight: 600; padding: 0 28px; }"
        "QPushButton:hover { background: #347a6c; }"
        "QPushButton:pressed { background: #1f4a42; }"));
    connect(m_startButton, &QPushButton::clicked, this, &SplashPage::startChatClicked);

    auto *center = new QVBoxLayout;
    center->setSpacing(18);
    center->addStretch(2);
    center->addWidget(m_avatarButton, 0, Qt::AlignHCenter);
    center->addWidget(m_statusLabel, 0, Qt::AlignHCenter);
    center->addWidget(m_welcomeLabel, 0, Qt::AlignHCenter);
    center->addSpacing(8);
    center->addWidget(m_startButton, 0, Qt::AlignHCenter);
    center->addStretch(3);

    auto *topBar = new QHBoxLayout;
    topBar->addStretch();
    topBar->addWidget(m_settingsButton);
    topBar->setContentsMargins(16, 12, 16, 0);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addLayout(topBar);
    root->addLayout(center, 1);
}

void SplashPage::setRole(const QString &displayName, const QString &welcome,
                         const QPixmap &avatar, const QPixmap &background)
{
    Q_UNUSED(displayName);
    m_welcomeLabel->setText(welcome.isEmpty() ? tr("你好～") : welcome);
    m_bgPixmap = background;
    if (!avatar.isNull()) {
        const QPixmap circ = makeCircular(avatar, 180, QColor(QStringLiteral("#3D8B7A")));
        m_avatarButton->setIcon(QIcon(circ));
        m_avatarButton->setIconSize(QSize(180, 180));
        m_avatarButton->setText(QString());
    } else {
        m_avatarButton->setIcon(QIcon());
        m_avatarButton->setText(QStringLiteral("•"));
    }
    refreshBackground();
}

void SplashPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshBackground();
}

void SplashPage::refreshBackground()
{
    if (!m_bgLabel)
        return;
    m_bgLabel->setGeometry(rect());
    if (m_vignette)
        m_vignette->setGeometry(rect());
    if (m_bgPixmap.isNull()) {
        m_bgLabel->setStyleSheet(QStringLiteral("background: #0e1211;"));
        m_bgLabel->clear();
    } else {
        const QSize sz = size();
        if (sz.width() >= 2 && sz.height() >= 2)
            m_bgLabel->setPixmap(m_bgPixmap.scaled(sz, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    m_bgLabel->lower();
    if (m_vignette) {
        m_vignette->raise();
        // Content above vignette
        for (QObject *c : children()) {
            auto *w = qobject_cast<QWidget *>(c);
            if (w && w != m_bgLabel && w != m_vignette)
                w->raise();
        }
    }
}

QPixmap SplashPage::makeCircular(const QPixmap &src, int size, const QColor &ring)
{
    QPixmap out(size, size);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const int ringW = 4;
    p.setPen(QPen(ring, ringW));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(1, 1, size - 2, size - 2));

    QPainterPath clip;
    const qreal inset = ringW + 1;
    clip.addEllipse(QRectF(inset, inset, size - 2 * inset, size - 2 * inset));
    p.setClipPath(clip);
    const QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    p.drawPixmap((size - scaled.width()) / 2, (size - scaled.height()) / 2, scaled);
    return out;
}
