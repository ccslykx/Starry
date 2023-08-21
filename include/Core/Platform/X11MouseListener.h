#pragma once

#include <QElapsedTimer>
#include "AbstractMouseListener.h"

class X11MouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static X11MouseListener* instance();
    void startListen() override;
    void stopListen() override;

public slots:
    static void handleRecordEvent(void *data);

private:
    X11MouseListener();
    ~X11MouseListener();
    void init();
    static void enableContext();

private:
    static X11MouseListener *m_instance;

    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;
};

