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
    QWidget *m_parent;
    bool    m_isOn;
    QString m_on;
    QString m_off;
};