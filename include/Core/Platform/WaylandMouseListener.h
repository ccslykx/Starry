#pragma once

#include <QElapsedTimer>
#include "AbstractMouseListener.h"

class WaylandMouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static WaylandMouseListener* instance();
    void startListen() override;
    void stopListen() override;


private:
    WaylandMouseListener();
    ~WaylandMouseListener();
    static void handleEvent(void *ev);
    
private:
    static WaylandMouseListener *m_instance;

    static void *m_libinput;
    static void *m_udev;
    static bool m_running;

    static double m_x;
    static double m_y;
    static bool m_pressed;

    static QElapsedTimer    *m_doubleClickTimer;    
};