/**
 * @file SPluginInfo.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
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
#include "SPluginItem.h"

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
        const bool _enabled = false,    // 是否启用
        SPluginItem *pluginItem = nullptr);
    SPluginInfo(
        const QString &_name,           // 插件名称
        const QString &_script,         // 插件执行的命令
        const QPixmap &_icon,           // 插件图标
        const int _index = 0,           // 插件排序
        const QString &_tip = "",       // 插件说明
        const bool _enabled = false,    // 是否启用
        SPluginItem *_pluginItem = nullptr);
    ~SPluginInfo();

    void setIndex(int);
    void setIcon(const QString &path);
    void setIcon(const QPixmap &icon);

public slots:
    void refreshItem();

private:
    void saveIcon(const QString &path);
    void saveIcon(const QPixmap &icon);

public:
    QString name;       // 插件名称
    QString tip;        // 插件说明
    QString script;     // 插件执行命令
    QString iconPath;   // 插件图标路径
    QPixmap icon;       // 插件图标
    int     index;      // 插件排序
    bool    enabled;    // 是否启用

    SPluginItem *pluginItem = nullptr;
};