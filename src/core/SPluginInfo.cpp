#include "SPluginInfo.h"
#include "utils.h"

SPluginInfo::SPluginInfo(
    const QString &_name,     // 插件名称
    const QString &_script,   // 插件执行的命令
    const QString &_iconPath, // 插件图标路径
    const int _index,         // 插件排序
    const QString &_tip,      // 插件说明
    const bool _enabled)      // 是否启用
    : name(_name)
    , tip(_tip)
    , script(_script)
    , iconPath(_iconPath)
    , index(_index)
    , enabled(_enabled)
{
    SDEBUG
    this->setIcon(iconPath);
}

SPluginInfo::SPluginInfo(
    const QString &_name,   // 插件名称
    const QString &_script, // 插件执行的命令
    const QPixmap &_icon,   // 插件图标
    const int _index,       // 插件排序
    const QString &_tip,    // 插件说明
    const bool _enabled)    // 是否启用
    : name(_name)
    , tip(_tip)
    , script(_script)
    , icon(_icon)
    , index(_index)
    , enabled(_enabled)
{
    SDEBUG
    this->iconPath = ""; // TODO: Save icon to file and set iconPath.
}

void SPluginInfo::setIndex(int i)
{
    SDEBUG
    if (i < 0 /* i > max */)
    {
        qWarning() << "Setting index with invalid value";
        return;
    }
    this->index = i;
}

void SPluginInfo::setIcon(const QString &path)
{
    SDEBUG
    this->saveIcon(path);
    this->icon = QPixmap(path); // TODO: change path to iconPath
}

void SPluginInfo::setIcon(const QPixmap &icon)
{
    SDEBUG
    this->saveIcon(icon);
    this->icon = icon;
    /* TODO: 
        1. detect icon file exist
        2. create icon file or replace it
        3. refresh this->iconPath
    */
}

/* Private Functions */

SPluginInfo::~SPluginInfo()
{
    
}

void SPluginInfo::saveIcon(const QString &path) 
{
    SDEBUG
    /* TODO:
        1. detect path exist
        2. detect private path exist, if not exist, create it.
        3. copy path to private path.
        4. set iconPath as private icon path.
    */
}

void SPluginInfo::saveIcon(const QPixmap &icon)
{
    SDEBUG
    /* TODO:
        1. detect private path exist, if not exist, create it.
        2. save icon to private path
        3. set iconPath as private icon path.
    */
}
