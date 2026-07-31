#include <QSettings>
#include <QDir>
#include <QLocale>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

#include "SConfig.h"
#include "utils.h"

namespace
{
const QString DEBUG_MODE_KEY = QStringLiteral("debugModeEnabled");
const QString LANGUAGE_KEY = QStringLiteral("language");

QString defaultLanguageCode()
{
    const QLocale locale = QLocale::system();
    switch (locale.language())
    {
    case QLocale::Chinese:
        return locale.territory() == QLocale::Taiwan
                || locale.territory() == QLocale::HongKong
                || locale.territory() == QLocale::Macao
            ? QStringLiteral("zh_TW")
            : QStringLiteral("zh_CN");
    case QLocale::German:
        return QStringLiteral("de");
    case QLocale::French:
        return QStringLiteral("fr");
    case QLocale::Japanese:
        return QStringLiteral("ja");
    default:
        return QStringLiteral("en");
    }
}

QString normalizedLanguageCode(QString code)
{
    code.replace(QLatin1Char('-'), QLatin1Char('_'));
    if (code.compare(QStringLiteral("zh_TW"), Qt::CaseInsensitive) == 0
        || code.compare(QStringLiteral("zh_HK"), Qt::CaseInsensitive) == 0
        || code.compare(QStringLiteral("zh_MO"), Qt::CaseInsensitive) == 0)
    {
        return QStringLiteral("zh_TW");
    }
    if (code.startsWith(QStringLiteral("zh"), Qt::CaseInsensitive))
    {
        return QStringLiteral("zh_CN");
    }
    for (const QString &language : {
             QStringLiteral("en"),
             QStringLiteral("de"),
             QStringLiteral("fr"),
             QStringLiteral("ja")})
    {
        if (code.startsWith(language, Qt::CaseInsensitive))
        {
            return language;
        }
    }
    return defaultLanguageCode();
}
}

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
    const QString iconPath = QDir::cleanPath(path + QDir::separator() + "icons");
    
    // Detect config dir exists
    QDir configDir(_path), iconDir(iconPath);
    if (configDir.exists() && iconDir.exists())
    {
        return true; // If exist, return true;
    } 

    // If not exists
    if (makepath) 
    {
        if (!configDir.mkpath(_path))
        {
            qWarning() << "Create config file path failed:" << path;
            return false;
        }
        if (!iconDir.mkpath(iconPath))
        {
            qWarning() << "Create icon file path failed:" << path;
        }
        return true;
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
    QString settingPath = QDir::cleanPath(m_configPath + QDir::separator() + "starry.conf");
    qDebug() << "SConfig::saveToFile: settingPath:" << settingPath;
    QSettings s(settingPath, QSettings::NativeFormat);
    // Save plugins
    s.remove(QString("STARRY_PLUGINS"));
    s.beginGroup(QString("STARRY_PLUGINS"));
    for (SPluginInfo *info : getSPluginInfos())
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

    QString conf = QDir::cleanPath(path + QDir::separator() + "starry.conf");
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
    QVector<SPluginInfo*> pluginInfos;
    pluginInfos.reserve(plugins.size());
    for (QString pName : plugins)
    {
        s.beginGroup(pName);
        QString tip = s.value(QString("tip")).toString();
        QString script = s.value(QString("script")).toString();
        QString iconPath = s.value(QString("iconPath")).toString();
        bool validIndex = false;
        int index = s.value(QString("index")).toInt(&validIndex);
        bool iconEnabled = s.value(QString("iconEnabled")).toBool();
        bool nameEnabled = s.value(QString("nameEnabled")).toBool();
        s.endGroup();

        if (!validIndex || index < 0)
        {
            qWarning() << pName << "has an invalid plugin index; it will be reordered";
            index = static_cast<int>(pluginInfos.size());
        }
        if (iconPath.isEmpty() || !QFileInfo::exists(iconPath))
        {
            qWarning() << pName + "'s icon file not found, use default icon";
            iconPath = ":/default_icon.png";
        }
        SPluginInfo *info = new SPluginInfo(pName, script, iconPath, index, tip, iconEnabled, nameEnabled);
        pluginInfos.push_back(info);
    }
    s.endGroup();
    std::stable_sort(pluginInfos.begin(), pluginInfos.end(), [] (const SPluginInfo *lhs, const SPluginInfo *rhs) {
        return lhs->index < rhs->index;
    });
    for (qsizetype i = 0; i < pluginInfos.size(); ++i)
    {
        SPluginInfo *info = pluginInfos.at(i);
        info->index = static_cast<int>(i);
        if (addPlugin(info, ReadFromFile))
        {
            emit readPlugin(info);
        }
        else
        {
            delete info;
        }
    }

    // Settings
    s.beginGroup(QString("STARRY_SETTINGS"));
    const QStringList settings = s.childKeys();
    bool debugMode = false;
    QString language = defaultLanguageCode();
    for (const QString &key : settings)
    {
        const QVariant value = s.value(key);
        if (key == DEBUG_MODE_KEY)
        {
            debugMode = value.toBool();
        }
        else if (key == LANGUAGE_KEY)
        {
            language = normalizedLanguageCode(value.toString());
        }
        else
        {
            settingMap.insert(key, value);
        }
    }
    s.endGroup();
    setDebugModeEnabled(debugMode);
    setLanguageCode(language);
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

bool SConfig::debugModeEnabled() const
{
    return settingMap.value(DEBUG_MODE_KEY, false).toBool();
}

void SConfig::setDebugModeEnabled(bool enabled)
{
    if (debugModeEnabled() == enabled)
    {
        return;
    }
    settingMap.insert(DEBUG_MODE_KEY, enabled);
    emit debugModeChanged(enabled);
}

QString SConfig::languageCode() const
{
    return normalizedLanguageCode(
        settingMap.value(LANGUAGE_KEY, defaultLanguageCode()).toString());
}

void SConfig::setLanguageCode(const QString &code)
{
    const QString normalized = normalizedLanguageCode(code);
    if (languageCode() == normalized)
    {
        return;
    }
    settingMap.insert(LANGUAGE_KEY, normalized);
    emit languageChanged(normalized);
}

bool SConfig::isPluginNameValid(const QString &name) const
{
    if (name.isEmpty() || name != name.trimmed() || name == "." || name == "..")
    {
        return false;
    }

    const QString invalidCharacters = QStringLiteral("<>:\"/\\|?*");
    for (const QChar character : invalidCharacters)
    {
        if (name.contains(character))
        {
            return false;
        }
    }
    return true;
}

bool SConfig::isPluginNameAvailable(const QString &name, const SPluginInfo *exclude) const
{
    if (!isPluginNameValid(name))
    {
        return false;
    }
    for (const SPluginInfo *info : pInfoMap)
    {
        if (info != exclude && info->name.compare(name, Qt::CaseInsensitive) == 0)
        {
            return false;
        }
    }
    return true;
}

bool SConfig::addPlugin(SPluginInfo *info, AddMode mode)
{
    SDEBUG
    if (!info)
    {
        return false;
    }
    if (!isPluginNameAvailable(info->name))
    {
        qWarning() << "Invalid or duplicate plugin name:" << info->name;
        return false;
    }
    if (mode == AddMode::NewCreate)
    {
        info->index = static_cast<int>(pInfoMap.size());
        info->iconPath = QDir::cleanPath(m_configPath + QDir::separator() + "icons" + QDir::separator() + info->name + ".png");
        qDebug() << "iconPath:" << info->iconPath;
        if (!savePluginIcon(info))
        {
            return false;
        }
    }
    pInfoMap.insert(info->name, info);
    QObject::connect(info, &SPluginInfo::needDelete, this, &SConfig::deletePlugin);
    QObject::connect(info, &SPluginInfo::iconChanged, this, &SConfig::savePluginIcon);
    return true;
}

bool SConfig::renamePlugin(SPluginInfo *info, const QString &newName)
{
    if (!info || !isPluginNameAvailable(newName, info))
    {
        return false;
    }

    auto current = std::find_if(pInfoMap.begin(), pInfoMap.end(), [info] (SPluginInfo *candidate) {
        return candidate == info;
    });
    if (current == pInfoMap.end())
    {
        qWarning() << "Cannot rename a plugin that is not registered";
        return false;
    }
    if (info->name == newName)
    {
        return true;
    }

    const QString oldName = info->name;
    const QString oldIconPath = info->iconPath;
    pInfoMap.erase(current);
    info->name = newName;
    pInfoMap.insert(newName, info);

    info->iconPath = QDir::cleanPath(m_configPath + QDir::separator() + "icons" + QDir::separator() + newName + ".png");
    if (!savePluginIcon(info))
    {
        pInfoMap.remove(newName);
        info->name = oldName;
        info->iconPath = oldIconPath;
        pInfoMap.insert(oldName, info);
        return false;
    }
    if (!oldIconPath.startsWith(":/")
        && oldIconPath.compare(info->iconPath, Qt::CaseInsensitive) != 0)
    {
        QFile::remove(oldIconPath);
    }
    emit info->nameChanged(info);
    return true;
}

void SConfig::deletePlugin(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    auto current = std::find_if(pInfoMap.begin(), pInfoMap.end(), [info] (SPluginInfo *candidate) {
        return candidate == info;
    });
    if (current == pInfoMap.end())
    {
        qWarning() << info->name << "doesn't exist!";
        return;
    }
    pInfoMap.erase(current);
    normalizePluginIndexes();
    info->deleteLater();
}

bool SConfig::savePluginIcon(SPluginInfo *info)
{
    if (!info)
    {
        return false;
    }

    QSaveFile iconFile(info->iconPath);
    if (!iconFile.open(QIODevice::WriteOnly)
        || !info->icon.save(&iconFile, "PNG", 100)
        || !iconFile.commit())
    {
        qWarning() << "Icon failed save to path: " << info->iconPath;
        iconFile.cancelWriting();
        return false;
    }
    return true;
}

SPluginInfo* SConfig::getSPluginInfo(const QString &name)
{
    SDEBUG
    return pInfoMap.value(name, nullptr);
}

QVector<SPluginInfo*> SConfig::getSPluginInfos()
{
    SDEBUG
    QVector<SPluginInfo*> plugins = pInfoMap.values();
    std::stable_sort(plugins.begin(), plugins.end(), [] (const SPluginInfo *lhs, const SPluginInfo *rhs) {
        if (lhs->index == rhs->index)
        {
            return lhs->name.compare(rhs->name, Qt::CaseInsensitive) < 0;
        }
        return lhs->index < rhs->index;
    });
    return plugins;
}

void SConfig::normalizePluginIndexes()
{
    const QVector<SPluginInfo*> plugins = getSPluginInfos();
    for (qsizetype i = 0; i < plugins.size(); ++i)
    {
        plugins.at(i)->index = static_cast<int>(i);
    }
}

QString SConfig::version()
{
    SDEBUG
    return QStringLiteral(STARRY_VERSION_STRING);
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
        m_configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        qDebug() << "m_configPath: " << m_configPath << '\n';
        detectConfigPath(m_configPath, true);
    }
    settingMap = QHash<QString, QVariant>();
    settingMap.insert(DEBUG_MODE_KEY, false);
    settingMap.insert(LANGUAGE_KEY, defaultLanguageCode());
    pInfoMap = QHash<QString, SPluginInfo*>();
}

// SConfig::~SConfig()
// {

// }
