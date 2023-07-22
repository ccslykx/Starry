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

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
    #define SDEBUG qDebug() << "[FILE:" << __FILE__ << ", LINE:" << __LINE__ << ", FUNC:" << Q_FUNC_INFO << "]";
#endif