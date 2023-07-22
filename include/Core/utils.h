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
    #define SDEBUG qDebug() << "[FILE:" << __FILE__ << ", LINE:" << __LINE__ << ", FUNC:" << Q_FUNC_INFO << "]";
#endif

class SUtils
{
public:
    static QString STARRY_ICON(int size)
    {
        return ":/starry_" + QString::number(size) + "x" + QString::number(size) + ".png";
    }
};

