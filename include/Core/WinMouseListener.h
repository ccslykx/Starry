#pragma once

//#ifdef _WIN32

#include "AbstractMouseListener.h"

#include <QObject>
#include <QElapsedTimer>

#include <Windows.h>

class WinMouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static WinMouseListener* instance();
    void startListen() override;
    void stopListen() override;

private:
    WinMouseListener();
    static LRESULT CALLBACK LowLevelMouseProc (int nCode, WPARAM wParam, LPARAM lParam);

private:
    static WinMouseListener *m_instance;
    static HHOOK            m_hook;

    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;
};

//#endif
