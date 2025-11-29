#pragma once

#include "AbstractMouseListener.h"
#include <QObject>
#include <QElapsedTimer>
#include <QFuture>
#include <CoreGraphics/CoreGraphics.h>

class MacMouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static MacMouseListener* instance();
    virtual void startListen() override;
    virtual void stopListen() override;
    static void handleCGEvent(int);

private:
    MacMouseListener();
    static void createEventTap();

private:
    static MacMouseListener *m_instance;
    
    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;

    static void             *m_eventTap;
    static CFRunLoopRef     m_runLoop;

    QFuture<void>           m_future;
};
