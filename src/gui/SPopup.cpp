#include <QTimer>
#include <QScreen>

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

void SPopup::addItem(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    SPopupItem *item = SPopupItem::create(info, this);
    addItem(item);

    QObject::connect(info, &SPluginInfo::needDelete, [this] (SPluginInfo *info) {
        deleteItem(info->popupItem);
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
    if (m_layout)
    {
        delete m_layout;
    }
    m_layout = nullptr;
    if (m_instance) 
    {
        delete m_instance;
    }
    m_instance = nullptr;
}

void SPopup::initGui()
{
    if (!m_layout)
    {
        m_layout = new QHBoxLayout(this);
        m_layout->setAlignment(Qt::AlignTop);
    }
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(ICON_TIMEOUT);
        QObject::connect(m_timer, &QTimer::timeout, this, &QWidget::hide);
    }

    this->setMinimumSize(ICON_LENGTH, ICON_LENGTH);

    this->setAttribute(Qt::WA_TranslucentBackground);
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

    QRect screenGeometry = this->screen()->virtualGeometry();

    if (loc.x() + this->width() > screenGeometry.width())
    {
        loc.setX(screenGeometry.width() - this->width());
    }
    if (loc.y() + this->height() > screenGeometry.height())
    {
        loc.setY(screenGeometry.height() - this->height());
    }

    this->setGeometry(loc.x(), loc.y(), this->width(), this->height());
}
