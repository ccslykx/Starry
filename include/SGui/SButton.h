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

#include <QPushButton>

#ifndef S_BUTTON_STYLE
    #define S_BUTTON_STYLE "border-style: outset; border-width: 1px; border-radius:8px;"
#endif

class SButton : public QPushButton
{
    Q_OBJECT
public:
    SButton(const QString &text = "", QWidget *parent = (QWidget*)nullptr);
    void setPixmap(const QPixmap &pixmap);
};
