#pragma once

#include "AbstractMouseListener.h"
#include <QObject>
#include <QElapsedTimer>
#include <CoreGraphics/CoreGraphics.h>

class MacMouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static MacMouseListener* instance();
    virtual void startListen() override;
    virtual void stopListen() override;
    static void handleCGEvent(CGEventType);

private:
    MacMouseListener();
    static bool createEventTap();

private:
    static MacMouseListener *m_instance;
    
    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;

    static CFMachPortRef     m_eventTap;
    static CFRunLoopRef     m_runLoop;
    static CFRunLoopSourceRef m_runLoopSource;
};
