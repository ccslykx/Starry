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
#include <QHash>
#include <QListWidget>
#include <QStackedWidget>

#include "SPluginEditor.h"
#include "SPluginItem.h"
#include "SConfig.h"

class QLabel;
class QComboBox;
class QFrame;
class SPluginTask;
class SSwitcher;

class SSettings : public QWidget
{
    Q_OBJECT

public:
    static SSettings* instance(QWidget *parent = (QWidget*)nullptr);
    void initGui();
    void showAndActivate();
    void addPluginItem(SPluginItem*); // 添加到 m_pluginListWidget
    void addPluginItem(SPluginInfo*); // 添加到 m_pluginListWidget
    void deletePluginItem(SPluginItem*);
    void addMenuItem(QWidget*);

public slots:
    void showContent(int index = -1); // 根据左侧菜单显示对应的右侧内容
    void onCreatePluginClicked(); // 新建插件

signals:
    void windowClose();

private:
    explicit SSettings(QWidget *parent = (QWidget*)nullptr);
    ~SSettings();

    void closeEvent(QCloseEvent *ev) override;
    void changeEvent(QEvent *event) override;
    void refreshTheme(bool force = false,
                      Qt::ColorScheme scheme = Qt::ColorScheme::Unknown);
    void retranslateUi();
    void retranslateTaskRow(SPluginTask *task);
    void syncLanguageSelection(const QString &code);
#ifdef Q_OS_MACOS
    void refreshAccessibilityPermission();
#endif
    void refreshPluginIndex();
    void addTaskItem(SPluginTask *task);
    void removeTaskItem(SPluginTask *task);
    void updateTaskEmptyState();

private:
    static SSettings        *m_instance;

    SConfig                 *m_config = nullptr;
    SPluginEditor           *m_pluginEditor = nullptr;

    QListWidget             *m_menuListWidget = nullptr; // 菜单页
    QVector<SButton*>        m_menuButtons;
    QStackedWidget          *m_contentWidget = nullptr; // 内容页
    QWidget                 *m_generalWidget = nullptr; // 内容页-常规设置
    QLabel                  *m_generalTitleLabel = nullptr;
    QLabel                  *m_generalDescriptionLabel = nullptr;
    QLabel                  *m_languageTitleLabel = nullptr;
    QLabel                  *m_languageDescriptionLabel = nullptr;
    QFrame                  *m_languageCard = nullptr;
    QComboBox               *m_languageComboBox = nullptr;
    QLabel                  *m_selectionPopupTitleLabel = nullptr;
    QLabel                  *m_selectionPopupDescriptionLabel = nullptr;
    QFrame                  *m_selectionPopupCard = nullptr;
    SSwitcher               *m_selectionPopupSwitcher = nullptr;
    QLabel                  *m_debugTitleLabel = nullptr;
    QLabel                  *m_debugDescriptionLabel = nullptr;
    QFrame                  *m_debugModeCard = nullptr;
    SSwitcher               *m_debugModeSwitcher = nullptr;
#ifdef Q_OS_MACOS
    QFrame                  *m_accessibilityPermissionCard = nullptr;
    QLabel                  *m_accessibilityPermissionTitleLabel = nullptr;
    QLabel                  *m_accessibilityPermissionDescriptionLabel = nullptr;
    QLabel                  *m_accessibilityPermissionStatusLabel = nullptr;
    SButton                 *m_requestAccessibilityPermissionButton = nullptr;
#endif
    QWidget                 *m_pluginWidget = nullptr; // 内容页-插件
    QLabel                  *m_pluginTitleLabel = nullptr;
    QListWidget             *m_pluginListWidget = nullptr; // 内容页-插件-已有插件列表
    SButton                 *m_newPluginButton = nullptr;
    QWidget                 *m_taskWidget = nullptr; // 内容页-任务管理器
    QLabel                  *m_taskTitleLabel = nullptr;
    QLabel                  *m_taskDescriptionLabel = nullptr;
    QListWidget             *m_taskListWidget = nullptr;
    QLabel                  *m_emptyTaskLabel = nullptr;
    QHash<SPluginTask*, QListWidgetItem*> m_taskItems;
    QWidget                 *m_shortcutWidget = nullptr; // 内容页-快捷键
    QLabel                  *m_shortcutTitleLabel = nullptr;
    QListWidget             *m_shortcutListWidget = nullptr;
    QLabel                  *m_shortcutHelpLabel = nullptr;
    QWidget                 *m_aboutWidget = nullptr; // 内容页-关于
    QLabel                  *m_aboutTitleLabel = nullptr;
    QLabel                  *m_aboutContentLabel = nullptr;
    bool                     m_darkStyle = false;
    bool                     m_styleInitialized = false;
    Qt::ColorScheme          m_pendingColorScheme = Qt::ColorScheme::Unknown;
};
