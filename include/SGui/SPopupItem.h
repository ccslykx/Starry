/**
 * @file SPopupItem.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QLabel>
#include <QProcess>
#include <QMouseEvent>

#include "SPluginInfo.h"

class SPopupItem : public QLabel
{
    Q_OBJECT
public:
    static SPopupItem* create(SPluginInfo*, QWidget *parent = (QWidget*)nullptr);
    static void remove(SPopupItem*);
    void exec();
    void stop();

signals:
    void clicked();

private:
    explicit SPopupItem(SPluginInfo*, QWidget *parent = (QWidget*)nullptr);
    ~SPopupItem();
    void mouseReleaseEvent(QMouseEvent*);

private:
    SPluginInfo *m_info = nullptr;
    QProcess    *m_process = nullptr;
};