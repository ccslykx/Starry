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

#include <QLabel>

#ifndef S_SWITCHER_STYLE
    #define S_SWITCHER_STYLE "border-style: outset; border-width: 1px; border-radius:8px;"
#endif

class SSwitcher : public QLabel
{
    Q_OBJECT
public:
    SSwitcher(const QString &on, const QString &off, bool status, 
        QWidget *parent = (QWidget*)nullptr);
    bool isOn();
    void setStatus(bool);

signals:
    void switchOn();
    void switchOff();

private:
    void switchStatus();
    void mouseReleaseEvent(QMouseEvent *ev);

private:
    bool    m_isOn;
    QString m_on;
    QString m_off;
};