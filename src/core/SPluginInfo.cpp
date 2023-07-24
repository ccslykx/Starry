#include "SPluginInfo.h"
#include "SConfig.h"
#include "utils.h"

SPluginInfo::SPluginInfo(
    const QString &_name,     // 插件名称
    const QString &_script,   // 插件执行的命令
    const QString &_iconPath, // 插件图标路径
    const int _index,         // 插件排序
    const QString &_tip,      // 插件说明
    const bool _enabled,      // 是否启用
    ConstructMode _mode)
    : name(_name)
    , tip(_tip)
    , script(_script)
    , iconPath(_iconPath)
    , index(_index)
    , enabled(_enabled)
{
    SDEBUG
    if (_mode == ConstructMode::ReadFromFile)
    {
        icon = QPixmap(iconPath);
    }
}

SPluginInfo::SPluginInfo(
    const QString &_name,   // 插件名称
    const QString &_script, // 插件执行的命令
    const QPixmap &_icon,   // 插件图标
    const int _index,       // 插件排序
    const QString &_tip,    // 插件说明
    const bool _enabled,    // 是否启用
    ConstructMode _mode)
    : name(_name)
    , tip(_tip)
    , script(_script)
    , icon(_icon)
    , index(_index)
    , enabled(_enabled)
{
    SDEBUG
    if (_mode == ConstructMode::NewCreate)
    {
        SConfig *config = SConfig::config();
        iconPath = config->configPath() + "/icons/" + name + ".png";
        qDebug() << name << ":" << iconPath;
        if (!icon.save(iconPath, "png", 100))
        {
            qWarning() << "Icon save failed";
        }
    }
}

/* Private Functions */

SPluginInfo::~SPluginInfo()
{
    
}