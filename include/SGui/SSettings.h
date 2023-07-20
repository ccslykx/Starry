/**
 * @file SSetting.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>

#include <memory>

#include "SPluginEditor.h"
#include "SPluginItem.h"

class SSettings : public QWidget
{
    Q_OBJECT

public:
    static SSettings* instance(QWidget *parent = (QWidget*)nullptr);
    void initGui();
    void addPluginItem(SPluginItem*); // 添加到 m_pluginListWidget
    void addPluginItem(SPluginInfo*); // 添加到 m_pluginListWidget
    void deletePluginItem(SPluginItem*);
    void addMenuItem(QLabel*);
    void readPlugins(); // 从SConfig读取插件设置

public slots:
    void showContent(int index = -1); // 根据左侧菜单显示对应的右侧内容
    void onCreatePluginClicked(); // 新建插件
    void onDeletePluginClicked(SPluginItem*);
    void onEditPluginClicked(SPluginItem*);
    void onPluginEdited(SPluginItem*);
    void onPluginCreated(SPluginInfo*);

signals:
    void saveOnClose();

private:
    explicit SSettings(QWidget *parent = (QWidget*)nullptr);
    ~SSettings();

    void closeEvent(QCloseEvent *ev);
    void refreashPluginIndex();

private:
    static SSettings        *m_instance;

    QListWidget             *m_menuListWidget = nullptr; // 菜单页
    QStackedWidget          *m_contentWidget = nullptr; // 内容页
    QWidget                 *m_pluginWidget = nullptr; // 内容页-插件
    QListWidget             *m_pluginListWidget = nullptr; // 内容页-插件-已有插件列表
    QListWidget             *m_shortcutWidget = nullptr; // 内容页-快捷键
    QWidget                 *m_aboutWidget = nullptr; // 内容页-关于

    SPluginEditor           *m_pluginEditor = nullptr;
};
