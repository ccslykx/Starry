#pragma once

#include "AbstractMouseListener.h"

#include <QObject>
#include <QElapsedTimer>

class WinMouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static WinMouseListener* instance();
    void startListen() override;
    void stopListen() override;
    static void handleLowLevelMouseProc(unsigned long long);

private:
    WinMouseListener();

private:
    static WinMouseListener *m_instance;

    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;
};

