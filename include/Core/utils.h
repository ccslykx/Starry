/**
 * @file utils.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QDebug>
#include <QString>

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#   define SDEBUG qDebug() << "[FILE:" << __FILE__ + QString(":") + QString::number(__LINE__) << "\t" << Q_FUNC_INFO << "]";
#else
#   define SDEBUG
#endif

class SUtils
{
public:
    static QString STARRY_ICON(int size)
    {
        return ":/starry_" + QString::number(size) + "x" + QString::number(size) + ".png";
    }
};

