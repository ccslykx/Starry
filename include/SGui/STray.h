/**
 * @file STray.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>

#include "SPopup.h"
#include "SSettings.h"
#include "SMouseListener.h"

class STray : public QSystemTrayIcon
{
    Q_OBJECT
    
public:
    static STray* instance(QApplication*);

    /* Menu Functions */
    void setEnable(bool);
    void settings();
    void exitTray();

private:
    explicit STray(QApplication*);
    ~STray();

    void initGui(); // Initialize the gui.
    void initServices();
    void initSettings(); // Initialize the settings.

private:
    static STray            *m_instance;
    static SPopup           *m_popup;
    static SSettings        *m_settings;
    static SMouseListener   *m_mouseListener;
};