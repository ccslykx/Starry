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
#include <QGuiApplication>
#include <QPalette>
#include <QSignalBlocker>
#include <QStyleHints>

#include "SSwitcher.h"
#include "utils.h"

namespace
{
QString switcherStyleSheet(bool dark)
{
    if (dark)
    {
        return QStringLiteral(
            "QPushButton {"
            "  padding: 0 12px;"
            "  border: 1px solid #475467; border-radius: 18px;"
            "  background: #344054; color: #D0D5DD; font-weight: 500;"
            "}"
            "QPushButton:hover { background: #475467; border-color: #667085; }"
            "QPushButton:pressed { background: #1D2939; }"
            "QPushButton:checked { background: #214E2D; color: #EAF8E5; border-color: #59C837; }"
            "QPushButton:checked:hover { background: #2B6039; border-color: #6ED34F; }"
            "QPushButton:checked:pressed { background: #183B22; border-color: #49B02C; }"
            "QPushButton:focus { border: 2px solid #83DE67; }"
            "QPushButton:disabled { background: #1D2939; color: #667085; border-color: #344054; }");
    }
    return QStringLiteral(
        "QPushButton {"
        "  padding: 0 12px;"
        "  border: 1px solid #D0D5DD; border-radius: 18px;"
        "  background: #F2F4F7; color: #475467; font-weight: 500;"
        "}"
        "QPushButton:hover { background: #EAECF0; border-color: #98A2B3; }"
        "QPushButton:pressed { background: #D0D5DD; }"
        "QPushButton:checked { background: #EAF8E5; color: #245B16; border-color: #59C837; }"
        "QPushButton:checked:hover { background: #DCF3D5; border-color: #49B02C; }"
        "QPushButton:checked:pressed { background: #C9EDBE; border-color: #3D9424; }"
        "QPushButton:focus { border: 2px solid #83DE67; }"
        "QPushButton:disabled { background: #F2F4F7; color: #98A2B3; border-color: #EAECF0; }");
}
}

SSwitcher::SSwitcher(const QString &on, const QString &off, bool status, QWidget *parent)
    : QPushButton(parent), m_isOn(status), m_on(on), m_off(off)
{
    SDEBUG
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(36);
    refreshStyle();
    if (QStyleHints *styleHints = QGuiApplication::styleHints())
    {
        QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, this,
                         [this] (Qt::ColorScheme scheme) {
            m_pendingColorScheme = scheme;
            refreshStyle(true, scheme);
        });
    }
    QObject::connect(this, &QPushButton::toggled, this, [this] (bool checked) {
        m_isOn = checked;
        updateAppearance();
        if (checked)
        {
            emit switchOn();
        }
        else
        {
            emit switchOff();
        }
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
    QString label = m_isOn ? m_on : m_off;
    if (m_isOn && !label.isEmpty())
    {
        label.prepend(QStringLiteral("✓ "));
    }
    setText(label);
    setAccessibleDescription(m_isOn ? tr("On") : tr("Off"));
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

void SSwitcher::setPixmap(const QPixmap &pixmap)
{
    setIcon(QIcon(pixmap));
    setIconSize(pixmap.size());
}

void SSwitcher::refreshStyle(bool force, Qt::ColorScheme scheme)
{
    const bool dark = scheme == Qt::ColorScheme::Dark
        || (scheme == Qt::ColorScheme::Unknown
            && QGuiApplication::palette().color(QPalette::Window).lightness() < 128);
    if (!force && m_styleInitialized && dark == m_darkStyle)
    {
        return;
    }
    m_darkStyle = dark;
    m_styleInitialized = true;
    setStyleSheet(switcherStyleSheet(dark));
}

void SSwitcher::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);
    if (event && (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::ThemeChange))
    {
        Qt::ColorScheme scheme = m_pendingColorScheme;
        if (event->type() == QEvent::ThemeChange)
        {
            scheme = QGuiApplication::styleHints()->colorScheme();
            m_pendingColorScheme = scheme;
        }
        refreshStyle(event->type() == QEvent::ThemeChange, scheme);

        if (event->type() != QEvent::ThemeChange
            && m_pendingColorScheme != Qt::ColorScheme::Unknown)
        {
            const bool paletteIsDark =
                QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
            const bool pendingIsDark =
                m_pendingColorScheme == Qt::ColorScheme::Dark;
            if (paletteIsDark == pendingIsDark)
            {
                m_pendingColorScheme = Qt::ColorScheme::Unknown;
            }
        }
    }
}
