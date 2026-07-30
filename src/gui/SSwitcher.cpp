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

#include <QIcon>
#include <QSignalBlocker>

#include "SSwitcher.h"
#include "utils.h"

SSwitcher::SSwitcher(const QString &on, const QString &off, bool status, QWidget *parent)
    : QPushButton(parent), m_isOn(status), m_on(on), m_off(off)
{
    SDEBUG
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    QObject::connect(this, &QPushButton::toggled, this, [this] (bool checked) {
        m_isOn = checked;
        updateAppearance();
        emit checked ? switchOn() : switchOff();
    });
    this->setStatus(status);
}

bool SSwitcher::isOn() const
{
    SDEBUG
    return m_isOn;
}

void SSwitcher::setStatus(bool status)
{
    SDEBUG
    m_isOn = status;
    const QSignalBlocker blocker(this);
    setChecked(status);
    updateAppearance();
}

void SSwitcher::updateAppearance()
{
    if (m_isOn)
    {
        this->setText(m_on);
        this->setStyleSheet(m_onStyleSheet);
    }
    else
    {
        this->setText(m_off);
        this->setStyleSheet(m_offStyleSheet);
    }
}

void SSwitcher::setOnText(const QString &text)
{
    m_on = text;
    updateAppearance();
}

void SSwitcher::setOffText(const QString &text)
{
    m_off = text;
    updateAppearance();
}

void SSwitcher::setOnStyleSheet(const QString &styleSheet)
{
    m_onStyleSheet = styleSheet;
    updateAppearance();
}

void SSwitcher::setOffStyleSheet(const QString &styleSheet)
{
    m_offStyleSheet = styleSheet;
    updateAppearance();
}

void SSwitcher::setPixmap(const QPixmap &pixmap)
{
    setIcon(QIcon(pixmap));
    setIconSize(pixmap.size());
}
