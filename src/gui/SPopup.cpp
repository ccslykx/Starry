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

void SPopup::update()
{
    SDEBUG
    clearItems();

    QVector<SPluginInfo*> infos = m_config->getSPluginInfos();
    int count = infos.count();
    if (count <= 0)
    {
        return;
    }

    for (SPluginInfo *info : infos)
    {
        if (info->enabled)
        {
            addItem(info);
        }
    }
}

void SPopup::addItem(SPluginInfo *info)
{
    SDEBUG
    SPopupItem *item = SPopupItem::create(info, this);
    addItem(item);
}

void SPopup::addItem(SPopupItem *item)
{   
    SDEBUG
    m_items.push_back(item);
    m_layout->addWidget(item);

    QObject::connect(item, &SPopupItem::clicked, this, &SPopup::hide);
}

void SPopup::deleteItem(SPopupItem *item)
{
    SDEBUG
    m_layout->removeWidget(item);
    m_items.removeOne(item);
    SPopupItem::remove(item);
}

void SPopup::clearItems()
{
    SDEBUG
    if (m_layout)
    {
        delete m_layout;
        m_layout = new QHBoxLayout(this);
    }
    for (SPopupItem *item : m_items)
    {
        SPopupItem::remove(item);
    }
    m_items.clear();
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
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint); // Qt::Tool is important flag to make it work. I mean not stealing focus. 但对x11下Gnome44 无效
    this->setWindowFlags(Qt::Popup | Qt::Window); // 不在任务栏显示
    this->setWindowFlags(Qt::X11BypassWindowManagerHint); // 在x11上不占用focus
    this->setStyleSheet("border-style: outset; border-width: 1px; border-radius:8px;");
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