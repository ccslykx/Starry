/**
 * @file SConfig.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QSettings>
#include <QHash>
#include "SPluginInfo.h"
#include "SPluginItem.h"

#define STARRY_VERSION_MAJOR 1
#define STARRY_VERSION_MINOR 0
#define STARRY_VERSION_PATCH 0
#define STARRY_VERSION       QString::number(STARRY_VERSION_MAJOR) + '.' + QString::number(STARRY_VERSION_MINOR) + '.' + QString::number(STARRY_VERSION_PATCH)

class SConfig : public QObject
{
    Q_OBJECT
public:
    static SConfig* config(const QString &path = "");

    /**
     * @brief Detect existance of config path (Dir).
     * 
     * @param path 
     * @param makepath true for making new path when path not exist.
     * @return true for path exist
     * @return false for path not exist
     */
    bool detectConfigPath(const QString &path = "", bool makepath = false);
    void saveToFile(const QString &path = ""); // save configs to file
    void readFromFile(const QString &path = ""); // read configs from file
    void setConfigFilePath(const QString &path);
    
    void addSetting(const QString &key, QVariant value);      // 添加设置项
    void editSetting(const QString &key, QVariant newValue);  // 编辑设置项
    QVariant getSetting(const QString &key);  // 获取设置项
    void deleteSetting(const QString &key);   // 删除设置项

    void addPlugin(SPluginInfo*);     // 添加插件配置信息
    void addPlugin(SPluginItem*);     // 添加插件配置信息
    void deletePlugin(SPluginInfo*);  // 删除插件配置信息

    QString version();
    int major();
    int minor();
    int patch();

private:
    explicit SConfig(const QString &path = "");
    // ~SConfig();

private:
    static SConfig                      *m_instance;
    QString                             m_configPath = ""; // Dir path, not file
    QHash<QString, QVariant>            settingMap;     // 设置项哈希表
    QHash<QString, SPluginInfo*>        pInfoMap;       // 插件信息哈希表
};