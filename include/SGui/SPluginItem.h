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
    static void remove(SPluginItem*);

    void refresh(); // Call this function when m_info changed
    void setIndexToInfo(size_t index);
    SPluginInfo* pluginInfo(); 

private:
    explicit SPluginItem(SPluginInfo *pluginInfo, QWidget *parent = (QWidget*)nullptr);
    ~SPluginItem();

    void initGui(); 

private:
    SPluginInfo     *m_info = nullptr;

    SButton         *m_deleteButton = nullptr;
    SSwitcher       *m_iconSwitcher = nullptr;
    SSwitcher       *m_nameSwitcher = nullptr;
    QLabel          *m_tipLabel = nullptr;
    SButton         *m_editButton = nullptr;
};