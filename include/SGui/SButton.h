/**
 * @file SButton.h
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

class SButton : public QPushButton
{
    Q_OBJECT

public:
    enum class Role
    {
        Primary,
        Secondary,
        Danger,
        Navigation,
        Icon,
        DangerIcon,
        IconPicker
    };
    Q_ENUM(Role)

    SButton(const QString &text = "", QWidget *parent = (QWidget*)nullptr);

    Role role() const;
    void setRole(Role role);
    bool isSelected() const;
    void setSelected(bool selected);
    void setPixmap(const QPixmap &pixmap);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshStyle(bool force = false,
                      Qt::ColorScheme scheme = Qt::ColorScheme::Unknown);
    void refreshPolish();

    Role m_role = Role::Secondary;
    bool m_selected = false;
    bool m_darkStyle = false;
    bool m_styleInitialized = false;
    Qt::ColorScheme m_pendingColorScheme = Qt::ColorScheme::Unknown;
};
