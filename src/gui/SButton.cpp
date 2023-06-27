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

#include <QMouseEvent>
#include "SButton.h"

SButton::SButton(const QString &text, QWidget *parent)
{
    this->setText(text);
    this->setParent(parent);
}

void SButton::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev != nullptr && ev->button() == Qt::LeftButton)
    {
        emit clicked();
    }
}