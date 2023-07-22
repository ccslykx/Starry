#include <QSettings>
#include <QDir>

#include "SConfig.h"
#include "utils.h"

SConfig* SConfig::m_instance = nullptr;

SConfig* SConfig::config(const QString &path)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SConfig(path);
    }
    return m_instance;
}

bool SConfig::detectConfigPath(const QString &path, bool makepath)
{
    SDEBUG
    const QString &_path = path.isEmpty() ? m_configPath : path;
    
    // Detect config dir exists
    QDir configDir(_path);
    if (configDir.exists())
    {
        return true; // If exist, return true;
    } 

    // If not exists
    if (makepath) 
    {
        if (!configDir.mkpath(_path))
        {
            qWarning() << "Create config file path failed:" << path;
        }
    }
    return false;
}

void SConfig::saveToFile(const QString &path)
{
    SDEBUG
    // If config dir not exist, make it and set to m_configpath.
    if (!detectConfigPath(path, true)) 
    {
        setConfigFilePath(path);
    }

    // Save configs
    QSettings s(m_configPath + "/starry.conf", QSettings::NativeFormat);
    
    // Save plugins
    s.remove(QString("STARRY_PLUGINS"));
    s.beginGroup(QString("STARRY_PLUGINS"));
    for (SPluginInfo *info : pInfoMap.values())
    {
        s.beginGroup(info->name);
        s.setValue(QString("tip"), info->tip);
        s.setValue(QString("script"), info->script);
        s.setValue(QString("iconPath"), info->iconPath);
        s.setValue(QString("index"), info->index);
        s.setValue(QString("enabled"), info->enabled);
        s.endGroup();
    }
    s.endGroup();

    // Save settings
    s.beginGroup(QString("STARRY_SETTINGS"));
    for (auto i = settingMap.begin(); i != settingMap.end(); ++i)
    {
        s.setValue(i.key(), i.value());
    }
    s.endGroup();
}

void SConfig::readFromFile(const QString &path)
{
    SDEBUG
    // If config dir not exist, return.
    if (!detectConfigPath(path))
    {
        qWarning() << "Config path not exist!";
        return;
    }

    QString conf = path + "/starry.conf";
    QFile f(conf);
    if (!f.exists())
    {
        qWarning() << "Config file not found!";
        return;
    }

    QSettings s(conf, QSettings::NativeFormat);
    
    // Plugins
    s.beginGroup(QString("STARRY_PLUGINS"));
    QStringList plugins = s.childGroups();
    for (QString pName : plugins)
    {
        s.beginGroup(pName);
        QString tip = s.value(QString("tip")).toString();
        QString script = s.value(QString("script")).toString();
        QString iconPath = s.value(QString("iconPath")).toString();
        int index = s.value(QString("index")).toInt();
        bool enabled = s.value(QString("enabled")).toBool();
        s.endGroup();

        if (iconPath.isEmpty())
        {
            qWarning() << pName + "'s icon file not found, use default icon";
            // iconPath = ":/default_icon.png" /* TODO */
        }
        QPixmap pixmap(iconPath);
        SPluginInfo *info = new SPluginInfo(pName, script, iconPath, index, tip, enabled);
        addPlugin(std::move(info)); /* Need Test */
    }
    s.endGroup();
    
    // Settings
    s.beginGroup(QString("STARRY_SETTINGS"));
    QStringList settings = s.childGroups();
    for (QString key : settings)
    {
        addSetting(key, s.value(key));
    }
}

void SConfig::setConfigFilePath(const QString &path)
{
    SDEBUG
    m_configPath = path;
}

void SConfig::addSetting(const QString &key, QVariant value)
{
    SDEBUG
    if (this->settingMap.contains(key)) 
    {
        qWarning() << key << "has already exist!";
        return;
    }
    settingMap.insert(key, value);
}

void SConfig::editSetting(const QString &key, QVariant newValue)
{
    SDEBUG
    if (!this->settingMap.contains(key))
    {
        qWarning() << key << "doesn't exist!";
        return;
    }
    settingMap.insert(key, newValue);
}

QVariant SConfig::getSetting(const QString &key)
{
    SDEBUG
    if (!this->settingMap.contains(key))
    {
        qWarning() << key << "doesn't exist!";
        return "";
    }
    return settingMap.value(key);
}

void SConfig::deleteSetting(const QString &key)
{
    SDEBUG
    if (this->settingMap.contains(key))
    {
        settingMap.remove(key);
    }
}

void SConfig::addPlugin(SPluginInfo *info)
{
    SDEBUG
    if (pInfoMap.contains(info->name))
    {
        qWarning() << info->name << "has already exist!";
        return;
    }
    pInfoMap.insert(info->name, info);
}

void SConfig::addPlugin(SPluginItem *item)
{
    SDEBUG
    addPlugin(item->pluginInfo()); /* Need Test */
}

void SConfig::deletePlugin(SPluginInfo *info)
{
    SDEBUG
     if (!pInfoMap.contains(info->name))
    {
        qWarning() << info->name << "doesn't exist!";
        return;
    }
    pInfoMap.remove(info->name);
}

SPluginInfo* SConfig::getSPluginInfo(const QString &name)
{
    SDEBUG
    return pInfoMap.value(name, nullptr);
}

QVector<SPluginInfo*> SConfig::getSPluginInfos()
{
    SDEBUG
    QVector<SPluginInfo*> plugins(pInfoMap.count());
    for (SPluginInfo* info : pInfoMap.values())
    {
        plugins[info->index] = info;
    }
    return plugins;
}

QString SConfig::version()
{
    SDEBUG
    return STARRY_VERSION;
}

int SConfig::major()
{
    SDEBUG
    return STARRY_VERSION_MAJOR;
}

int SConfig::minor()
{
    SDEBUG
    return STARRY_VERSION_MINOR;
}

int SConfig::patch()
{
    SDEBUG
    return STARRY_VERSION_PATCH;
}

SConfig::SConfig(const QString &path) 
    : m_configPath(path)
{
    SDEBUG
    if (this->m_configPath == "")
    {
        m_configPath = QDir::homePath() + "/.config/Starry/starry.conf";
        /* TODO: Windows & macOS */
    }
    readFromFile(m_configPath);
}

// SConfig::~SConfig()
// {

// }