#pragma once

// #ifdef __win32

#include "AbstractMouseListener.h"

#include <QObject>

#include "Windows"

class WinMouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static WinMouseListener* instance();
    void startListen() override;
    void stopListen() override;

private:
    WinMouseListener();
    ~WinMouseListener();
    void init();

private:
    static HHOOK    *m_hook;

};

// #endif