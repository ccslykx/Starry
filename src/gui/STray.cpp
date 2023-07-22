#include <QTimer>
#include <QDebug>

#include "STray.h"
#include "utils.h"

STray*      STray::m_instance = nullptr;
SPopup*     STray::m_popup = nullptr;
SSettings*  STray::m_settings = nullptr;
QClipboard* STray::m_clipboard = nullptr;
// SInvoker*   STray::m_invoker = nullptr;

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
    QApplication *parent = (QApplication*) this->parent();
    parent->quit();
}

STray::STray(QApplication *app)
{
    SDEBUG
    this->setParent(app);
    m_pressPos = QCursor::pos();
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

    // QObject::connect(enable, &QAction::triggered, this, &STray::setEnable); /* TODO */
    QObject::connect(enable, &QAction::triggered, m_popup, &SPopup::showPopup); /* TODO */
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
        QObject::connect(m_settings, &SSettings::saveOnClose, m_popup, &SPopup::update);
    }
    if (!m_popup)
    {
        m_popup = SPopup::instance();
        m_popup->update();
    }
    if (!m_clipboard)
    {
        m_clipboard = QGuiApplication::clipboard();
    }
    if (!m_waitSelectionChangeTimer)
    {
        m_waitSelectionChangeTimer = new QTimer(this);
        m_waitSelectionChangeTimer->setInterval(100); // TODO: change with SConfig
        QObject::connect(m_waitSelectionChangeTimer, &QTimer::timeout, this, [this] () {
            if (this->m_selectionChanged)
            {
                this->m_popup->showPopup();
                this->m_selectionChanged = false;
            }
            this->m_waitSelectionChangeTimer->stop();
        });
    }
    if (!m_waitB1ReleaseTimer)
    {
        m_waitB1ReleaseTimer = new QTimer(this);
        m_waitB1ReleaseTimer->setInterval(100); // TODO: change with SConfig
        QObject::connect(m_waitB1ReleaseTimer, &QTimer::timeout, this, [this] () {
            if (this->m_B1Released)
            {
                this->m_popup->showPopup();
                this->m_B1Released = false;
            }
            this->m_waitB1ReleaseTimer->stop();
        });
    }
}

void STray::initSettings()
{
    SDEBUG
}

void STray::connectMouseListener()
{
    /* TODO */
}

void STray::waitForSelectionChange()
{
    if (m_waitSelectionChangeTimer->isActive())
    {
        m_waitSelectionChangeTimer->stop();
    }
    m_waitSelectionChangeTimer->start();
}

void STray::waitForB1Release()
{
    if (m_waitB1ReleaseTimer->isActive())
    {
        m_waitB1ReleaseTimer->stop();
    }
    m_waitB1ReleaseTimer->start();
}