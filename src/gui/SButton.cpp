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

#include "SButton.h"
#include "utils.h"

SButton::SButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    SDEBUG
    this->setStyleSheet(S_BUTTON_STYLE);
    this->setMinimumHeight(40);
    this->setCursor(Qt::PointingHandCursor);
    this->setAccessibleName(text);
}

void SButton::setPixmap(const QPixmap &pixmap)
{
    setIcon(QIcon(pixmap));
    setIconSize(pixmap.size());
}
