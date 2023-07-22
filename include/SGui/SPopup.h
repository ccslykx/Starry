/**
 * @file SPopup.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief `SPopup`类是划词后的弹窗，弹窗的每一个按钮都是一个`SPopupItem`，
 *        `SPopup`可以创建和删除`SPopupItem`。
 * @version 0.1
 * @date 2023-06-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QWidget>
#include <QHBoxLayout>

#include "SPopupItem.h"
#include "SPluginInfo.h"
#include "SConfig.h"

/* TODO: use SConfig instead of using macros */
#define ICON_BASE_SIZE  32 // px
#define ICON_SCALE      1.0
#define ICON_LENGTH     int(ICON_BASE_SIZE * ICON_SCALE)
#define ICON_TIMEOUT    5000 // ms

class SPopup : public QWidget
{
    Q_OBJECT;

public:
    static SPopup* instance();
    void update();
    void addItem(SPluginInfo*);
    void addItem(SPopupItem*);
    void deleteItem(SPopupItem*);
    void clearItems();
    void showPopup();

private:
    explicit SPopup();
    ~SPopup();

    void initGui();
    void adjustGeometry(QPoint);

private:
    static SPopup           *m_instance;
    SConfig                 *m_config = nullptr;
    QHBoxLayout             *m_layout = nullptr;
    QTimer                  *m_timer = nullptr; // 计时自动消失
    QVector<SPopupItem*>    m_items;
};