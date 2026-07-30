#include <QTimer>
#include <QScreen>
#include <QGuiApplication>

#include <algorithm>

#include "SPopup.h"
#include "SConfig.h"
#include "utils.h"

SPopup* SPopup::m_instance = nullptr;

SPopup* SPopup::instance()
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SPopup;
    }
    return m_instance;
}

void SPopup::update()
{
    std::stable_sort(m_items.begin(), m_items.end(), [] (SPopupItem *lhs, SPopupItem *rhs) {
        const SPluginInfo *lhsInfo = lhs ? lhs->pluginInfo() : nullptr;
        const SPluginInfo *rhsInfo = rhs ? rhs->pluginInfo() : nullptr;
        if (!lhsInfo || !rhsInfo)
        {
            return lhsInfo != nullptr;
        }
        return lhsInfo->index < rhsInfo->index;
    });

    for (SPopupItem *item : m_items)
    {
        m_layout->removeWidget(item);
    }
    for (SPopupItem *item : m_items)
    {
        m_layout->addWidget(item);
    }
    adjustSize();
    QWidget::update();
}

void SPopup::addItem(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    SPopupItem *item = SPopupItem::create(info, this);
    addItem(item);

    QObject::connect(info, &SPluginInfo::needDelete, this, [this] (SPluginInfo *info) {
        deleteItem(info->popupItem);
    });
    QObject::connect(info, &SPluginInfo::indexChanged, this, [this] {
        update();
    });
}

void SPopup::addItem(SPopupItem *item)
{   
    SDEBUG
    if (!item)
    {
        return;
    }
    m_items.push_back(item);
    m_layout->addWidget(item);
    update();

    QObject::connect(item, &SPopupItem::clicked, this, &SPopup::hide);
}

void SPopup::deleteItem(SPopupItem *item)
{
    SDEBUG
    if (!item)
    {
        return;
    }
    m_layout->removeWidget(item);
    m_items.removeOne(item);
    SPopupItem::remove(item);
}

void SPopup::showPopup()
{
    if (this->isVisible()) // 如果当前已经弹出，则先隐藏，再弹出
    { 
        this->setVisible(false);
        if (m_timer->isActive())
        {
            m_timer->stop();
        }
    }

    this->adjustGeometry(QCursor::pos());
    this->setVisible(true);

    // 计时自动隐藏
    m_timer->start();
}

/* private functions */

SPopup::SPopup()
{
    SDEBUG
    m_config = SConfig::config();
    initGui();
}

SPopup::~SPopup()
{
    SDEBUG
    m_layout = nullptr;
    m_instance = nullptr;
}

void SPopup::initGui()
{
    if (!m_layout)
    {
        m_layout = new QHBoxLayout(this);
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->setSpacing(4);
        m_layout->setContentsMargins(0, 0, 0, 0);
    }
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(ICON_TIMEOUT);
        QObject::connect(m_timer, &QTimer::timeout, this, &QWidget::hide);
    }

    this->setMinimumSize(ICON_LENGTH, ICON_LENGTH);

    this->setAttribute(Qt::WA_TranslucentBackground, true);
    // https://stackoverflow.com/questions/966688/show-window-in-qt-without-stealing-focus
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::Popup | Qt::Window | Qt::WindowStaysOnTopHint); // Qt::Tool is important flag to make it work. I mean not stealing focus. 但对x11下Gnome44 无效
#ifdef __linux__
    QString dpEnv = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
    if (dpEnv.toUpper() == "X11")
    {
        this->setWindowFlag(Qt::X11BypassWindowManagerHint); // 在x11上不占用focus
    }
#elif __APPLE__ && TARGET_OS_MAC/* Need Test */
	this->setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif
}

void SPopup::adjustGeometry(QPoint loc)
{
    this->adjustSize();

    QScreen *targetScreen = QGuiApplication::screenAt(loc);
    if (!targetScreen)
    {
        targetScreen = QGuiApplication::primaryScreen();
    }
    if (!targetScreen)
    {
        this->move(loc);
        return;
    }

    const QRect available = targetScreen->availableGeometry();
    const int maxX = qMax(available.left(), available.right() - this->width() + 1);
    const int maxY = qMax(available.top(), available.bottom() - this->height() + 1);
    loc.setX(qBound(available.left(), loc.x(), maxX));
    loc.setY(qBound(available.top(), loc.y(), maxY));

    this->setGeometry(QRect(loc, this->size()));
}
