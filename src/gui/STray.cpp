#include <QTimer>
#include <QDebug>

#include "STray.h"
#define SDEBUG qDebug() << "[FILE:" << __FILE__ << ", LINE:" << __LINE__ << ", FUNC:" << Q_FUNC_INFO << "]";

STray*      STray::m_instance = nullptr;
SPopup*     STray::m_popup = nullptr;
SSettings*  STray::m_settings = nullptr;
QClipboard* STray::m_clipboard = nullptr;
SInvoker*   STray::m_invoker = nullptr;

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
/* TODO */
}

void STray::settings()
{
/* TODO */
}

void STray::exitTray()
{
/* TODO */
}

STray::STray(QApplication *app)
{
    SDEBUG
    this->setParent(app);
    m_pressPos = QCursor::pos();
    initGui();
    initSettings();
}

STray::~STray()
{
/* TODO */
}

void STray::initGui()
{
    SDEBUG
    if (!this->m_settings)
    {
        m_settings = SSettings::instance();
        // QObject::connect(m_settings, &SSettings::saveOnClose,
        //     this, &STray::refreshPopups); /* TODO */
    }

    /* TODO: setup timers */
    QMenu   *menu = new QMenu;
    QAction *enable = new QAction(menu);
    QAction *settings = new QAction(menu);
    QAction *exit = new QAction(menu);

    enable->setText(tr("Enable"));
    settings->setText(tr("Setting"));
    exit->setText(tr("Exit"));

    // QObject::connect(enable, &QAction::triggered, ) /* TODO */
    QObject::connect(settings, &QAction::triggered, m_settings, &SSettings::show);
    QObject::connect(exit, &QAction::triggered, m_parent, &QApplication::quit);

    menu->addAction(enable);
    menu->addAction(settings);
    menu->addAction(exit);

    this->setContextMenu(menu);

    /* TODO: set tray icon */
}

void STray::initSettings()
{
    
}

void STray::connectInvokers()
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