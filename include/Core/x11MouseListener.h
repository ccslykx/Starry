#pragma once

#include <QElapsedTimer>
#include "AbstractMouseListener.h"

#include <X11/Xlib.h>
#include <X11/extensions/record.h>
/* 给X11的宏定义污染擦PP */
#ifdef None
#   undef None
#endif
#ifdef KeyPress
#   undef KeyPress
#endif
#ifdef KeyRelease
#   undef KeyRelease
#endif
#ifdef FocusIn
#   undef FocusIn
#endif
#ifdef FocusOut
#   undef FocusOut
#endif
#ifdef FontChange
#   undef FontChange
#endif
#ifdef Expose
#   undef Expose
#endif

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