/**
 * @file SSwitcher.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QPushButton>

#ifndef S_SWITCHER_STYLE
    #define S_SWITCHER_STYLE "border-style: outset; border-width: 1px; border-radius:8px;"
#endif

enum SwitcherStatus
{
    On = true,
    Off = false
};

class SSwitcher : public QPushButton
{
    Q_OBJECT
public:
    SSwitcher(const QString &on = "On", const QString &off = "Off", 
        bool status = On, QWidget *parent = (QWidget*)nullptr);
    bool isOn() const;
    void setStatus(bool);
    void setOnText(const QString&);
    void setOffText(const QString&);
    void setOnStyleSheet(const QString&);
    void setOffStyleSheet(const QString&);
    void setPixmap(const QPixmap &pixmap);

signals:
    void switchOn();
    void switchOff();

private:
    void updateAppearance();

private:
    bool    m_isOn;
    QString m_on;
    QString m_off;
    QString m_onStyleSheet = S_SWITCHER_STYLE + QString("background-color: #59C837");
    QString m_offStyleSheet = S_SWITCHER_STYLE + QString("background-color: gray");
};
