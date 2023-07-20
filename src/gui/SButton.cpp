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

#define SDEBUG qDebug() << "[FILE:" << __FILE__ << ", LINE:" << __LINE__ << ", FUNC:" << Q_FUNC_INFO << "]";

SButton::SButton(const QString &text, QWidget *parent)
{
    SDEBUG
    this->setText(text);
    this->setParent(parent);
    this->setStyleSheet(S_BUTTON_STYLE);
    this->setAlignment(Qt::AlignCenter);
    this->setFixedHeight(48);
}

void SButton::mouseReleaseEvent(QMouseEvent *ev)
{
    SDEBUG
    if (ev != nullptr && ev->button() == Qt::LeftButton)
    {
        emit clicked();
    }
}