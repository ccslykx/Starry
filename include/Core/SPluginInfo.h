/**
 * @file SPluginInfo.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 所有插件的设置项、显示项都基于`SPluginInfo`类的信息，任何类修改（而非创建）该类的
 *      信息，均需要手动发送该类的相应信号通知其他控件做出响应。该类的创建信号通过
 *      `SPluginEditor`发出。
 *      All plugin settings and display items are based on the information of 
 *      this(`SPluginInfo`) class. All need to manually send the corresponding 
 *      signal of this class after edited (but not created) datas of this class
 *      to notify other class to respond. The `created` signal emitted by 
 *      `SPluginEditor`。
 * @version 0.1
 * @date 2023-06-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>

#include "SConfig.h"
#include "SPluginItem.h"
#include "SPopupItem.h"

class SPluginItem;
class SPopupItem;

class SPluginInfo : public QObject
{
    Q_OBJECT
public:
    SPluginInfo(
        const QString &_name,           // 插件名称
        const QString &_script,         // 插件执行的命令
        const QString &_iconPath = "",  // 插件图标路径
        const int _index = 0,           // 插件排序
        const QString &_tip = "",       // 插件说明
        const bool _enabled = false);   // 是否启用

    SPluginInfo(
        const QString &_name,           // 插件名称
        const QString &_script,         // 插件执行的命令
        const QPixmap &_icon,           // 插件图标
        const int _index = 0,           // 插件排序
        const QString &_tip = "",       // 插件说明
        const bool _enabled = false);   // 是否启用

signals:
    void edited(SPluginInfo *_ = nullptr); // name, script, tip
    void indexChanged(SPluginInfo *_ = nullptr);
    void iconChanged(SPluginInfo *_ = nullptr);
    void switchOn(SPluginInfo *_ = nullptr);
    void switchOff(SPluginInfo *_ = nullptr);
    void needDelete(SPluginInfo *_ = nullptr);

private:
    friend class SConfig;
    ~SPluginInfo();

public:
    QString name;       // 插件名称
    QString tip;        // 插件说明
    QString script;     // 插件执行命令
    QString iconPath;   // 插件图标路径
    QPixmap icon;       // 插件图标
    int     index;      // 插件排序
    bool    enabled;    // 是否启用

    SPluginItem *pluginItem = nullptr;
    SPopupItem  *popupItem = nullptr;
};