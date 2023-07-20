#include <QDebug>
#include "SPluginInfo.h"

SPluginInfo::SPluginInfo(
    const QString &_name,     // 插件名称
    const QString &_script,   // 插件执行的命令
    const QString &_iconPath, // 插件图标路径
    const int _index,         // 插件排序
    const QString &_tip,      // 插件说明
    const bool _enabled)      // 是否启用
    // SPluginItem *_pluginItem)
    : name(_name)
    , tip(_tip)
    , script(_script)
    , iconPath(_iconPath)
    , index(_index)
    , enabled(_enabled)
    // , pluginItem(_pluginItem) 
{
    this->setIcon(iconPath);
}

SPluginInfo::SPluginInfo(
    const QString &_name,   // 插件名称
    const QString &_script, // 插件执行的命令
    const QPixmap &_icon,   // 插件图标
    const int _index,       // 插件排序
    const QString &_tip,    // 插件说明
    const bool _enabled)    // 是否启用
    // SPluginItem *_pluginItem)
    : name(_name)
    , tip(_tip)
    , script(_script)
    , icon(_icon)
    , index(_index)
    , enabled(_enabled)
    // , pluginItem(_pluginItem) 
{
    this->iconPath = ""; // TODO: Save icon to file and set iconPath.
}

void SPluginInfo::setIndex(int i)
{
    if (i < 0 /* i > max */)
    {
        qWarning() << "Setting index with invalid value";
        return;
    }
    this->index = i;
}

void SPluginInfo::setIcon(const QString &path)
{
    this->saveIcon(path);
    this->icon = QPixmap(path); // TODO: change path to iconPath
}

void SPluginInfo::setIcon(const QPixmap &icon)
{
    this->saveIcon(icon);
    this->icon = icon;
    /* TODO: 
        1. detect icon file exist
        2. create icon file or replace it
        3. refreash this->iconPath
    */
}

/* Private Functions */

void SPluginInfo::saveIcon(const QString &path) 
{
    /* TODO:
        1. detect path exist
        2. detect private path exist, if not exist, create it.
        3. copy path to private path.
        4. set iconPath as private icon path.
    */
}

void SPluginInfo::saveIcon(const QPixmap &icon)
{
    /* TODO:
        1. detect private path exist, if not exist, create it.
        2. save icon to private path
        3. set iconPath as private icon path.
    */
}

// void SPluginInfo::refreshItem()
// {
//     if (this->pluginItem)
//     {
//         pluginItem->refresh();
//     }
// }