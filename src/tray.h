/**
 * @file tray.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include "popup.h"
#include "settings.h"
#include "x11Connector.h"

#include <QSystemTrayIcon>
#include <QApplication>
#include <QMenu>
#include <QClipboard>

class PopupTray : public QSystemTrayIcon
{
    Q_OBJECT
public:
    static PopupTray* instance(QApplication*);
    /**
     * @brief 在设置保存后刷新Popups
     * 
     */
    void refreshPopups();

    /* menu functions */
    void exitTray();
    void settings();
    void enable();

private:
    explicit PopupTray(QApplication*);
    ~PopupTray();
    void init();
    void connectX11();
    /**
     * @brief 在鼠标划词松开后，等待`selectionChange`信号。如果未发出该信号，
     * 可能是鼠标发生了拖拽行为（而非选词）
     */
    void waitForSelectionChange();
    /**
     * @brief 如果`selection`发生了变化，可能是用键盘选择了其他内容，也可能是
     * 使用了中文输入法。如果是选择了其他内容，一定会发生`B1Released`事件（包括
     * 双击选择），因此只有发生`B1Released`后才可能调用`showPopup()`。
     */
    void waitForB1Release();

private:
    QApplication        *m_parent;
    static PopupTray    *m_instance;

    static Popup        *m_popup;
    static Settings     *m_settings;
    static QClipboard   *m_clipboard;
    static X11Connector *m_x11Connector;
    
    QMenu               *m_menu = nullptr;
    
    QPoint              m_pressPos;
    QPoint              m_releasePos;
    bool                m_selectionChanged = false;
    bool                m_B1Released = false;

    QTimer              *m_waitSelectionChangeTimer = nullptr; // 在B1Released后等待QClipboard::selectionChanged信号
    QTimer              *m_waitB1ReleaseTimer = nullptr; // 在selectionChanged信号发出后等待B1Released信号
};