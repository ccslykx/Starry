/**
 * @file SPluginItem.h
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
#include "SPluginInfo.h"
#include "SSwitcher.h"
#include "SButton.h"

class SPluginInfo;

class SPluginItem : public QWidget
{
    Q_OBJECT

public:
    static SPluginItem* create(SPluginInfo *pluginInfo, QWidget *parent = (QWidget*)nullptr);
    static SPluginItem* create(const QString &name, const QString &script, const QPixmap &icon, 
        const QString &tip, bool enable = false, QWidget *parent = (QWidget*)nullptr);
    static SPluginItem* create(const QString &name, const QString &script, const QString &iconPath, 
        const QString &tip, bool enable = false, QWidget *parent = (QWidget*)nullptr);
    static void remove(SPluginItem*);

    void refresh(); // Call this function when m_info changed
    void setIndexToInfo(size_t index);
    SPluginInfo* pluginInfo(); 
    SButton*     deleteButton();
    SButton*     editButton();

private:
    explicit SPluginItem(SPluginInfo *pluginInfo, QWidget *parent = (QWidget*)nullptr);
    explicit SPluginItem(const QString &name, const QString &script, const QPixmap &icon, 
        const QString &tip, bool enable = false, QWidget *parent = (QWidget*)nullptr);
    explicit SPluginItem(const QString &name, const QString &script, const QString &iconPath, 
        const QString &tip, bool enable = false,QWidget *parent = (QWidget*)nullptr);
    ~SPluginItem();

    void initGui(); 

private:
    SPluginInfo     *m_info = nullptr;

    SButton         *m_deleteButton = nullptr;
    QLabel          *m_iconLabel = nullptr;
    QLabel          *m_nameLabel = nullptr;
    QLabel          *m_tipLabel = nullptr;
    SSwitcher       *m_switcher = nullptr;
    SButton         *m_editButton = nullptr;
};