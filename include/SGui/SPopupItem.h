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
#include <QKeyEvent>
#include <QMouseEvent>

#include "SPluginInfo.h"

class SPopupItem : public QWidget
{
    Q_OBJECT
public:
    static SPopupItem* create(SPluginInfo*, QWidget *parent = (QWidget*)nullptr);
    static void remove(SPopupItem*);
    SPluginInfo* pluginInfo();
    bool enabled() const;
    void exec();

signals:
    void clicked();
    void executionStarted(const QString &message);
    void executionFinished(bool success, const QString &message);

private:
    explicit SPopupItem(SPluginInfo*, QWidget *parent = (QWidget*)nullptr);
    ~SPopupItem();
    void initGui();
    void refreshText();
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    SPluginInfo *m_info = nullptr;
    QLabel      *m_iconLabel = nullptr;
    QLabel      *m_nameLabel = nullptr;
    bool         m_executionLocked = false;
};
