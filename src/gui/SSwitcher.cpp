/**
 * @file SSwitcher.cpp
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <QMouseEvent>

#include "SSwitcher.h"
#include "utils.h"

SSwitcher::SSwitcher(const QString &on, const QString &off, bool status, QWidget *parent)
    : m_on(on), m_off(off), m_isOn(status)
{
    SDEBUG
    this->setParent(parent);
    this->setAlignment(Qt::AlignCenter);
    this->setStatus(status);
}

bool SSwitcher::isOn()
{
    SDEBUG
    return m_isOn;
}

void SSwitcher::setStatus(bool status)
{
    SDEBUG
    m_isOn = status;
    if (m_isOn)
    {
        this->setText(m_on);
        this->setStyleSheet(S_SWITCHER_STYLE + QString("background-color: #59C837"));
    }
    else
    {
        this->setText(m_off);
        this->setStyleSheet(S_SWITCHER_STYLE + QString("background-color: gray"));
    }
}

void SSwitcher::switchStatus()
{
    SDEBUG
    setStatus(!m_isOn);
}

void SSwitcher::mouseReleaseEvent(QMouseEvent *ev)
{
    SDEBUG
    if (ev != nullptr && ev->button() == Qt::LeftButton)
    {
        switchStatus();
        emit m_isOn ? switchOn() : switchOff();
    }
}