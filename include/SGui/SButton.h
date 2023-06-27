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

#include <QLabel>

class SButton : public QLabel
{
    Q_OBJECT
public:
    SButton(const QString &text = "", QWidget *parent = (QWidget*)nullptr);

signals:
    void clicked();

private:
    void mouseReleaseEvent(QMouseEvent *ev);

private:
    QWidget *m_parent;
    QString m_text;
};