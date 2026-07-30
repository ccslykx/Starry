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

#include <QEvent>
#include <QPushButton>

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
    void setPixmap(const QPixmap &pixmap);

signals:
    void switchOn();
    void switchOff();

private:
    void updateAppearance();
    void refreshStyle();
    void changeEvent(QEvent *event) override;

private:
    bool    m_isOn;
    QString m_on;
    QString m_off;
    bool    m_darkStyle = false;
    bool    m_styleInitialized = false;
};
