#pragma once

#include <atomic>
#include <QElapsedTimer>
#include <QFuture>
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

    static std::atomic_bool m_running;

    static double m_x;
    static double m_y;
    static bool m_pressed;

    static QElapsedTimer    *m_doubleClickTimer;    
    QFuture<void>           m_future;
};
