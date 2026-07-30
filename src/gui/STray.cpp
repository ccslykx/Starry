#include "STray.h"
#include "utils.h"

STray*          STray::m_instance = nullptr;
SPluginEditor*  STray::m_editor = nullptr;
SConfig*        STray::m_config = nullptr;
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
    if (!m_mouseListener)
    {
        return;
    }
    if (enable == m_mouseListener->isListening())
    {
        return;
    }
    if (enable)
    {
        m_mouseListener->startListen();
    }
    else
    {
        m_mouseListener->stopListen();
    }
}

void STray::settings()
{
    SDEBUG
    if (m_settings)
    {
        m_settings->show();
    }
}

void STray::exitTray()
{
    SDEBUG
    if (m_mouseListener)
    {
        m_mouseListener->stopListen();
    }
    if (m_config)
    {
        m_config->saveToFile(m_config->configPath());
    }
    emit exiting();
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
    if (m_editor) { m_editor->deleteLater(); }
    if (m_popup) { m_popup->deleteLater(); }
    if (m_config) { m_config->deleteLater(); }
    if (m_settings) { m_settings->deleteLater(); }
    if (m_mouseListener) { m_mouseListener->stopListen(); m_mouseListener->deleteLater(); }
    m_editor = nullptr;
    m_popup = nullptr;
    m_config = nullptr;
    m_settings = nullptr;
    m_mouseListener = nullptr;
    m_instance = nullptr;
}

void STray::initGui()
{
    SDEBUG
    QMenu   *menu = new QMenu;
    QAction *enable = new QAction(menu);
    QAction *settings = new QAction(menu);
    QAction *exit = new QAction(menu);

    enable->setText(tr(m_mouseListener->isListening() ? "Disable" : "Enable"));
    settings->setText(tr("Setting"));
    exit->setText(tr("Exit"));

    QObject::connect(enable, &QAction::triggered, this, [this, enable]()
    {
        this->setEnable(!m_mouseListener->isListening());
        enable->setText(m_mouseListener->isListening() ? "Disable" : "Enable");
    });
    QObject::connect(settings, &QAction::triggered, this, &STray::settings);
    QObject::connect(exit, &QAction::triggered, this, &STray::exitTray);

    menu->addAction(enable);
    menu->addAction(settings);
    menu->addSeparator();
    menu->addAction(exit);

    this->setContextMenu(menu);

    QIcon trayIcon(SUtils::STARRY_ICON(32));
    this->setIcon(trayIcon);
}

void STray::initServices()
{
    SDEBUG
    if (!m_editor)
    {
        m_editor = SPluginEditor::editor();
    }
    if (!m_config)
    {
        m_config = SConfig::config();
    }
    if (!m_settings)
    {
        m_settings = SSettings::instance();
    }
    if (!m_popup)
    {
        m_popup = SPopup::instance();
    }
    if (!m_mouseListener)
    {
        m_mouseListener = SMouseListener::instance();
        m_mouseListener->startListen();
    }

    QObject::connect(m_editor, &SPluginEditor::created, [this] (SPluginInfo *info) {
        if (!this->m_config->addPlugin(info, AddMode::NewCreate))
        {
            qWarning() << "Plugin creation was rejected:" << info->name;
            info->deleteLater();
            return;
        }
        this->m_settings->addPluginItem(info);
        this->m_popup->addItem(info);
    });
    QObject::connect(m_config, &SConfig::readPlugin, [this] (SPluginInfo *info) {
        this->m_settings->addPluginItem(info);
        this->m_popup->addItem(info);
    });
    QObject::connect(m_mouseListener, &SMouseListener::canShow, m_popup, &SPopup::showPopup);
}

void STray::initSettings()
{
    SDEBUG
    m_config->readFromFile(m_config->configPath());
}
