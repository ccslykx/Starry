#include <QSettings>
#include <QDir>
#include <QStandardPaths>

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
    
    if (path.isEmpty())
    {
        qWarning() << "SConfig::saveToFile: path is empty, saving to m_configPath: " << m_configPath;
        saveToFile(m_configPath);
        return;
    }
    // If config dir not exist, make it and set to m_configpath.
    if (!detectConfigPath(path, true)) 
    {
        qWarning() << "SConfig::saveToFile: path is not a config file, setting config file path to " << path;
        setConfigFilePath(path);
    }

    // Save configs
    QString settingPath = QDir::cleanPath(m_configPath + "/starry.conf");
    qDebug() << "SConfig::saveToFile: settingPath:" << settingPath;
    QSettings s(settingPath, QSettings::NativeFormat);
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
        s.setValue(QString("iconEnabled"), info->iconEnabled);
        s.setValue(QString("nameEnabled"), info->nameEnabled);
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
    if (path.isEmpty())
    {
        readFromFile(m_configPath);
        return;
    }
    // If config dir not exist, return.
    if (!detectConfigPath(path))
    {
        qWarning() << "Config path not exist!";
        return;
    }

    QString conf = QDir::cleanPath(path + "/starry.conf");
    // QFile f(conf);
    // if (!f.exists())
    // {
    //     qWarning() << "Config file" << conf << "not found!";
    //     return;
    // }
    // f.close();

    QSettings s(conf, QSettings::Format::NativeFormat);
    
    // Plugins
    s.beginGroup(QString("STARRY_PLUGINS"));
    QStringList plugins = s.childGroups();
    qDebug() << "plugins.count: " << plugins.count();
    for (auto p : plugins) qDebug() << p << '\n';
    for (QString pName : plugins)
    {
        s.beginGroup(pName);
        QString tip = s.value(QString("tip")).toString();
        QString script = s.value(QString("script")).toString();
        QString iconPath = s.value(QString("iconPath")).toString();
        int index = s.value(QString("index")).toInt();
        bool iconEnabled = s.value(QString("iconEnabled")).toBool();
        bool nameEnabled = s.value(QString("nameEnabled")).toBool();
        s.endGroup();

        if (iconPath.isEmpty() || !QFileInfo::exists(iconPath))
        {
            qWarning() << pName + "'s icon file not found, use default icon";
            iconPath = ":/default_icon.png";
        }
        SPluginInfo *info = new SPluginInfo(pName, script, iconPath, index, tip, iconEnabled, nameEnabled);
        addPlugin(info, ReadFromFile); /* Need Test */
        emit readPlugin(info);
    }
    s.endGroup();
    
    // Settings
    s.beginGroup(QString("STARRY_SETTINGS"));
    QStringList settings = s.childGroups();
    for (QString key : settings)
    {
        addSetting(key, s.value(key));
    }
    s.endGroup();
}

void SConfig::setConfigFilePath(const QString &path)
{
    SDEBUG
    m_configPath = path;
}

QString SConfig::configPath()
{
    return m_configPath;
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

void SConfig::addPlugin(SPluginInfo *info, AddMode mode)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    if (pInfoMap.contains(info->name))
    {
        qWarning() << info->name << "has already exist!";
        return;
    }
    if (mode == AddMode::NewCreate)
    {
        info->iconPath = m_configPath + "/icons/" + info->name + ".png";
        savePluginIcon(info);
    }
    pInfoMap.insert(info->name, info);
    QObject::connect(info, &SPluginInfo::needDelete, this, &SConfig::deletePlugin);
    QObject::connect(info, &SPluginInfo::nameChanged, [this] (SPluginInfo *info) {
        if (QFileInfo::exists(info->iconPath)) // Remove previous icon file.
        {
            QFile::remove(info->iconPath);
        }
        info->iconPath = m_configPath + "/icons/" + info->name + ".png";
        this->savePluginIcon(info);
    });
    QObject::connect(info, &SPluginInfo::iconChanged, this, &SConfig::savePluginIcon);
}

void SConfig::deletePlugin(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    if (!pInfoMap.contains(info->name))
    {
        qWarning() << info->name << "doesn't exist!";
        return;
    }
    pInfoMap.remove(info->name);
    info->deleteLater();
}

void SConfig::savePluginIcon(SPluginInfo *info)
{
    if (QFileInfo::exists(info->iconPath))
    {
        qWarning() << "Icon file already exists, replace it.";
        QFile::remove(info->iconPath);
    }
    if (!info->icon.save(info->iconPath, "png", 100))
    {
        qWarning() << "Icon save failed";
    }
}

SPluginInfo* SConfig::getSPluginInfo(const QString &name)
{
    SDEBUG
    return pInfoMap.value(name, nullptr);
}

QVector<SPluginInfo*> SConfig::getSPluginInfos()
{
    SDEBUG
    const int pCount = pInfoMap.count();
    QVector<SPluginInfo*> plugins(pCount);
    if (pCount > 0) // Only when pInfoMap is not empty,
    { 
        // read & return SPluginInfo
        for (SPluginInfo* info : pInfoMap.values())
        {
            plugins[info->index] = info;
        }
    } // otherwise, return empty vector
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
        m_configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        qDebug() << "m_configPath: " << m_configPath << '\n';
        detectConfigPath(m_configPath, true);
    }
    settingMap = QHash<QString, QVariant>();
    pInfoMap = QHash<QString, SPluginInfo*>();
}

// SConfig::~SConfig()
// {

// }