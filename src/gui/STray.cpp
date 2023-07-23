#include "STray.h"
#include "utils.h"

STray*          STray::m_instance = nullptr;
SPopup*         STray::m_popup = nullptr;
SSettings*      STray::m_settings = nullptr;
SMouseListener* STray::m_mouseListener = nullptr;

STray* STray::instance(QApplication *app)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new STray(app);
    }
    return m_instance;
}

void STray::setEnable(bool enable)
{
    SDEBUG
/* TODO */
}

void STray::settings()
{
    SDEBUG
    m_settings->show();
}

void STray::exitTray()
{
    SDEBUG
    if (m_mouseListener)
    {
        m_mouseListener->stopListen();
    }
    QApplication *parent = (QApplication*) this->parent();
    parent->quit();
}

STray::STray(QApplication *app)
{
    SDEBUG
    this->setParent(app);
    
    initServices();
    initSettings();
    initGui(); 
}

STray::~STray()
{
    SDEBUG
/* TODO */
}

void STray::initGui()
{
    SDEBUG
    QMenu   *menu = new QMenu;
    QAction *enable = new QAction(menu);
    QAction *settings = new QAction(menu);
    QAction *exit = new QAction(menu);

    enable->setText(tr("Enable"));
    settings->setText(tr("Setting"));
    exit->setText(tr("Exit"));

    QObject::connect(enable, &QAction::triggered, this, &STray::setEnable); /* TODO */
    QObject::connect(settings, &QAction::triggered, this, &STray::settings);
    QObject::connect(exit, &QAction::triggered, this, &STray::exitTray);

    menu->addAction(enable);
    menu->addAction(settings);
    menu->addAction(exit);

    this->setContextMenu(menu);

    QIcon trayIcon(SUtils::STARRY_ICON(32));
    this->setIcon(trayIcon);
}

void STray::initServices()
{
    SDEBUG
    if (!m_settings)
    {
        m_settings = SSettings::instance();
    }
    if (!m_popup)
    {
        m_popup = SPopup::instance();
        m_popup->update();
    }
    if (!m_mouseListener)
    {
        m_mouseListener = SMouseListener::instance();   
        m_mouseListener->startListen();
    }

    QObject::connect(m_settings, &SSettings::saveOnClose, m_popup, &SPopup::update);
    QObject::connect(m_mouseListener, &SMouseListener::canShow, m_popup, &SPopup::showPopup);
}

void STray::initSettings()
{
    SDEBUG
}
