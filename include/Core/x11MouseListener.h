#pragma once

#include <QObject> // 要最先引入
#include <QElapsedTimer>
#include <X11/extensions/record.h>

#include "AbstractMouseListener.h"

class X11MouseListener : public AbstractMouseListener
{
    Q_OBJECT

public:
    static X11MouseListener* instance();
    void startListen() override;
    void stopListen() override;

public slots:
    static void handleRecordEvent(XRecordInterceptData *data);

private:
    X11MouseListener();
    ~X11MouseListener();
    void init();
    static void enableContext();

private:
    static X11MouseListener *m_instance;

    static XRecordContext   m_context;
    static Display          *m_display;

    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;
};