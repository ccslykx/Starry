#pragma once

#include "AbstractMouseListener.h"
#include <QObject>
#include <QElapsedTimer>

class MacMouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static MacMouseListener* instance();
    virtual void startListen() override;
    virtual void stopListen() override;
    static void handleCGEvent(void*);

private:
    MacMouseListener();
    static void createEventTap();

private:
    static MacMouseListener *m_instance;
    
    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;

    static void *m_eventTap;
};
