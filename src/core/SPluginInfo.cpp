#include "SPluginInfo.h"
#include "SConfig.h"
#include "utils.h"

SPluginInfo::SPluginInfo(
    const QString &_name,     
    const QString &_script,   
    const QString &_iconPath, 
    const int _index,         
    const QString &_tip,      
    const bool _iconEnabled,
    const bool _nameEnabled)  
    : name(_name)
    , tip(_tip)
    , script(_script)
    , iconPath(_iconPath)
    , index(_index)
    , iconEnabled(_iconEnabled)
    , nameEnabled(_nameEnabled)
{
    SDEBUG
    icon = QPixmap(iconPath);
}

SPluginInfo::SPluginInfo(
    const QString &_name,   
    const QString &_script, 
    const QPixmap &_icon,   
    const int _index,       
    const QString &_tip,    
    const bool _iconEnabled,
    const bool _nameEnabled)
    : name(_name)
    , tip(_tip)
    , script(_script)
    , icon(_icon)
    , index(_index)
    , iconEnabled(_iconEnabled)
    , nameEnabled(_nameEnabled)
{
    SDEBUG
}

/* Private Functions */

SPluginInfo::~SPluginInfo()
{
    
}