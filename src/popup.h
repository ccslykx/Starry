#pragma once

#include "settings.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QIcon>
#include <QPoint>
#include <QProcess>
#include <QTimer>
#include <QMouseEvent>

#define ICON_BASE_SIZE  32 // px
#define ICON_SCALE      1.0
#define ICON_LENGTH     int(ICON_BASE_SIZE * ICON_SCALE)
#define ICON_TIMEOUT    5000 // ms

class Popup;
class PopupItem;

class Popup : public QWidget
{
    Q_OBJECT;

public:
    static Popup* instance();
    void init();
    void update(const QVector<PluginItem*>);
    void addItem(PopupItem*);
    void deleteItem(PopupItem*);
    void showPopup();

private:
    explicit Popup();
    ~Popup();

    void adjustGeometry(QPoint);

    static Popup        *m_instance;
    QHBoxLayout         *m_layout = nullptr;
    QTimer              *m_timer = nullptr;

    QVector<PopupItem*> m_items;
};

class PopupItem : public QLabel
{
    Q_OBJECT

public:
    static PopupItem* createItem(const QString &script, const QString &name, const QPixmap &icon);
    static void deleteItem(PopupItem*);
    void exec();
    void stop();

signals:
    void clicked();

private:
    explicit PopupItem(const QString &script, const QString &name, const QPixmap &icon);  
    ~PopupItem();
    void mouseReleaseEvent(QMouseEvent*);

private:
    QString     m_script;   // 脚本内容
    QString     m_name;     // 脚本名称
    QPixmap     m_icon;     // 图标

    QProcess    *m_process = nullptr;
};