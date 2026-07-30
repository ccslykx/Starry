/**
 * @file SButton.cpp
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
#include <QStyle>
#include <QStyleHints>

#include "SButton.h"
#include "utils.h"

namespace
{
QString roleName(SButton::Role role)
{
    switch (role)
    {
    case SButton::Role::Primary:
        return QStringLiteral("primary");
    case SButton::Role::Secondary:
        return QStringLiteral("secondary");
    case SButton::Role::Danger:
        return QStringLiteral("danger");
    case SButton::Role::Navigation:
        return QStringLiteral("navigation");
    case SButton::Role::Icon:
        return QStringLiteral("icon");
    case SButton::Role::DangerIcon:
        return QStringLiteral("dangerIcon");
    case SButton::Role::IconPicker:
        return QStringLiteral("iconPicker");
    }
    return QStringLiteral("secondary");
}

QString buttonStyleSheet(bool dark)
{
    if (dark)
    {
        return QStringLiteral(
            "QPushButton {"
            "  padding: 0 14px;"
            "  border: 1px solid #475467; border-radius: 8px;"
            "  background: #344054; color: #F2F4F7; font-weight: 500;"
            "}"
            "QPushButton:hover { background: #475467; border-color: #667085; }"
            "QPushButton:pressed { background: #1D2939; }"
            "QPushButton:focus { border: 2px solid #FB923C; }"
            "QPushButton:disabled { background: #1D2939; color: #667085; border-color: #344054; }"

            "QPushButton[buttonRole=\"primary\"] { background: #FB923C; color: #431407; border-color: #FB923C; }"
            "QPushButton[buttonRole=\"primary\"]:hover { background: #FDBA74; border-color: #FDBA74; }"
            "QPushButton[buttonRole=\"primary\"]:pressed { background: #F97316; border-color: #F97316; }"
            "QPushButton[buttonRole=\"primary\"]:focus { border: 2px solid #FED7AA; }"
            "QPushButton[buttonRole=\"primary\"]:disabled { background: #7C4A2B; color: #D6B8A6; border-color: #7C4A2B; }"

            "QPushButton[buttonRole=\"danger\"] { background: #3B1C1E; color: #FDA29B; border-color: #7A271A; }"
            "QPushButton[buttonRole=\"danger\"]:hover { background: #D92D20; color: #FFFFFF; border-color: #D92D20; }"
            "QPushButton[buttonRole=\"danger\"]:pressed { background: #B42318; border-color: #B42318; }"
            "QPushButton[buttonRole=\"danger\"]:focus { border: 2px solid #FDA29B; }"

            "QPushButton[buttonRole=\"navigation\"] {"
            "  padding: 0 12px; text-align: left; background: transparent;"
            "  color: #D0D5DD; border: 1px solid transparent; border-radius: 7px;"
            "}"
            "QPushButton[buttonRole=\"navigation\"]:hover { background: #344054; color: #FFFFFF; }"
            "QPushButton[buttonRole=\"navigation\"][selected=\"true\"] {"
            "  background: #9A3412; color: #FFF7ED;"
            "  border: 1px solid #F97316; border-left: 3px solid #FDBA74;"
            "}"

            "QPushButton[buttonRole=\"icon\"], QPushButton[buttonRole=\"dangerIcon\"] {"
            "  min-width: 36px; max-width: 36px; padding: 0;"
            "}"
            "QPushButton[buttonRole=\"icon\"] { background: transparent; border-color: transparent; }"
            "QPushButton[buttonRole=\"icon\"]:hover { background: #344054; border-color: #475467; }"
            "QPushButton[buttonRole=\"dangerIcon\"] { background: #3B1C1E; color: #FDA29B; border-color: #7A271A; }"
            "QPushButton[buttonRole=\"dangerIcon\"]:hover { background: #D92D20; color: #FFFFFF; border-color: #D92D20; }"

            "QPushButton[buttonRole=\"iconPicker\"] {"
            "  padding: 0; background: #1D2939; border: 1px dashed #667085; border-radius: 10px;"
            "}"
            "QPushButton[buttonRole=\"iconPicker\"]:hover { background: #9A3412; border-color: #FDBA74; }"
            "QPushButton[buttonRole=\"iconPicker\"][dragActive=\"true\"] {"
            "  background: #9A3412; border: 2px solid #FDBA74;"
            "}");
    }

    return QStringLiteral(
        "QPushButton {"
        "  padding: 0 14px;"
        "  border: 1px solid #D0D5DD; border-radius: 8px;"
        "  background: #FFFFFF; color: #344054; font-weight: 500;"
        "}"
        "QPushButton:hover { background: #F9FAFB; border-color: #98A2B3; }"
        "QPushButton:pressed { background: #F2F4F7; }"
        "QPushButton:focus { border: 2px solid #FB923C; }"
        "QPushButton:disabled { background: #F2F4F7; color: #98A2B3; border-color: #EAECF0; }"

        "QPushButton[buttonRole=\"primary\"] { background: #F97316; color: #FFFFFF; border-color: #F97316; }"
        "QPushButton[buttonRole=\"primary\"]:hover { background: #EA580C; border-color: #EA580C; }"
        "QPushButton[buttonRole=\"primary\"]:pressed { background: #C2410C; border-color: #C2410C; }"
        "QPushButton[buttonRole=\"primary\"]:focus { border: 2px solid #FDBA74; }"
        "QPushButton[buttonRole=\"primary\"]:disabled { background: #FED7AA; color: #FFFFFF; border-color: #FED7AA; }"

        "QPushButton[buttonRole=\"danger\"] { background: #FEF3F2; color: #B42318; border-color: #FECDCA; }"
        "QPushButton[buttonRole=\"danger\"]:hover { background: #D92D20; color: #FFFFFF; border-color: #D92D20; }"
        "QPushButton[buttonRole=\"danger\"]:pressed { background: #B42318; border-color: #B42318; }"
        "QPushButton[buttonRole=\"danger\"]:focus { border: 2px solid #F97066; }"

        "QPushButton[buttonRole=\"navigation\"] {"
        "  padding: 0 12px; text-align: left; background: transparent;"
        "  color: #475467; border: 1px solid transparent; border-radius: 7px;"
        "}"
        "QPushButton[buttonRole=\"navigation\"]:hover { background: #F2F4F7; color: #1D2939; }"
        "QPushButton[buttonRole=\"navigation\"][selected=\"true\"] {"
        "  background: #FFF7ED; color: #C2410C;"
        "  border: 1px solid #FED7AA; border-left: 3px solid #F97316;"
        "}"

        "QPushButton[buttonRole=\"icon\"], QPushButton[buttonRole=\"dangerIcon\"] {"
        "  min-width: 36px; max-width: 36px; padding: 0;"
        "}"
        "QPushButton[buttonRole=\"icon\"] { background: transparent; border-color: transparent; }"
        "QPushButton[buttonRole=\"icon\"]:hover { background: #F2F4F7; border-color: #D0D5DD; }"
        "QPushButton[buttonRole=\"dangerIcon\"] { background: #FEF3F2; color: #B42318; border-color: #FECDCA; }"
        "QPushButton[buttonRole=\"dangerIcon\"]:hover { background: #D92D20; color: #FFFFFF; border-color: #D92D20; }"

        "QPushButton[buttonRole=\"iconPicker\"] {"
        "  padding: 0; background: #F9FAFB; border: 1px dashed #98A2B3; border-radius: 10px;"
        "}"
        "QPushButton[buttonRole=\"iconPicker\"]:hover { background: #FFF7ED; border-color: #F97316; }"
        "QPushButton[buttonRole=\"iconPicker\"][dragActive=\"true\"] {"
        "  background: #FFF7ED; border: 2px solid #F97316;"
        "}");
}
}

SButton::SButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    SDEBUG
    setProperty("buttonRole", roleName(m_role));
    setProperty("selected", false);
    refreshStyle();
    this->setMinimumHeight(36);
    this->setCursor(Qt::PointingHandCursor);
    this->setAccessibleName(text);
    if (QStyleHints *styleHints = QGuiApplication::styleHints())
    {
        QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, this,
                         [this] (Qt::ColorScheme scheme) {
            m_pendingColorScheme = scheme;
            refreshStyle(true, scheme);
        });
    }
}

SButton::Role SButton::role() const
{
    return m_role;
}

void SButton::setRole(Role role)
{
    if (m_role == role)
    {
        return;
    }
    m_role = role;
    setCheckable(role == Role::Navigation);
    if (isCheckable())
    {
        setChecked(m_selected);
    }
    setProperty("buttonRole", roleName(role));
    refreshPolish();
}

bool SButton::isSelected() const
{
    return m_selected;
}

void SButton::setSelected(bool selected)
{
    if (m_selected == selected)
    {
        if (isCheckable() && isChecked() != selected)
        {
            setChecked(selected);
        }
        return;
    }
    m_selected = selected;
    setProperty("selected", selected);
    if (isCheckable())
    {
        setChecked(selected);
    }
    refreshPolish();
}

void SButton::setPixmap(const QPixmap &pixmap)
{
    setIcon(QIcon(pixmap));
    setIconSize(pixmap.size());
}

void SButton::changeEvent(QEvent *event)
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

void SButton::refreshStyle(bool force, Qt::ColorScheme scheme)
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
    setStyleSheet(buttonStyleSheet(dark));
}

void SButton::refreshPolish()
{
    style()->unpolish(this);
    style()->polish(this);
    update();
}
