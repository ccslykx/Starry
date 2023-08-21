#include <QtConcurrent>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/record.h>

#include "X11MouseListener.h"
#include "utils.h"

static void callback(XPointer ptr, XRecordInterceptData *data)
{
    Q_UNUSED(ptr)
    X11MouseListener* instance = X11MouseListener::instance();
    instance->handleRecordEvent(static_cast<void*>(data));
}

X11MouseListener*   X11MouseListener::m_instance = nullptr;
XRecordContext      context = 0;
Display*            display = nullptr;

MouseStatus         X11MouseListener::lastMouseStatus = {-1, -1, False};
MouseStatus         X11MouseListener::currMouseStatus = {-1, -1, False};

QElapsedTimer*      X11MouseListener::m_doubleClickTimer = nullptr;

X11MouseListener* X11MouseListener::instance()
{
    if (!m_instance)
    {
        m_instance = new X11MouseListener;
    }
    return m_instance;
}

void X11MouseListener::startListen()
{
    SDEBUG
    QtConcurrent::run(enableContext);
}

void X11MouseListener::stopListen()
{
    SDEBUG
    if (context == 0)
    {
        return;
    }

    Display* display = XOpenDisplay(nullptr);
    if (!display)
    {
        qWarning() << "Could not open X display";
        return;
    }

    XRecordDisableContext(display, context);
    XFlush(display);
    XSync(display, false);
    XRecordFreeContext(display, context);
    context = 0;
    XCloseDisplay(display);
}

void X11MouseListener::handleRecordEvent(void *_data)
{
    XRecordInterceptData *data = static_cast<XRecordInterceptData*>(_data);
    if (data->category == XRecordFromServer)
    {
        xEvent *event = (xEvent*)data->data;
        
#define bEvent event->u.u
#define bPointer event->u.keyButtonPointer

        /*  event.u.u.type: 鼠标事件
            event.u.u.detail: 按键
        */

        switch (bEvent.type)
        {
        case ButtonPress:
            if (bEvent.detail == Button1) // 仅对左键做处理
            {
                if (lastMouseStatus.isPressed) 
                {
                    break;
                }

                currMouseStatus.x = bPointer.rootX;
                currMouseStatus.y = bPointer.rootY;
                currMouseStatus.isPressed = True;
                lastMouseStatus = currMouseStatus;
                qDebug() << "B1 Pressed: (" << currMouseStatus.x << ", " << currMouseStatus.y << ")";
                emit X11MouseListener::instance()->B1Pressed(currMouseStatus);
            }
            break;

        case ButtonRelease:
            if (bEvent.detail == 1) // 仅对左键做处理
            {
                if (!lastMouseStatus.isPressed) 
                {
                    break;
                }

                currMouseStatus.x = bPointer.rootX;
                currMouseStatus.y = bPointer.rootY;
                currMouseStatus.isPressed = False;

                MouseMotion motion = getMouseMotivation(lastMouseStatus, currMouseStatus);
                lastMouseStatus = currMouseStatus; // 更新鼠标状态
                
                qDebug() << "B1 Released: (" << currMouseStatus.x << ", " << currMouseStatus.y << ")";
                emit X11MouseListener::instance()->B1Released(currMouseStatus);

                if (motion == NotMoved)
                {
                    if (!m_doubleClickTimer->isValid())
                    {
                        qWarning() << "m_doubleClickTimer is not Valid";
                        break;
                    }
                    
                    if (!m_doubleClickTimer->hasExpired(250)) // 双击检测-结束
                    { // 如果释放距离上次不超过250ms
                        qDebug() << "B1 Double Clicked with elapsed" << m_doubleClickTimer->elapsed();
                        emit X11MouseListener::instance()->B1DoubleClicked(currMouseStatus);
                    }
                    m_doubleClickTimer->restart();
                    break; 
                }
            }
            break;
        
        default:
            break;
        }
        fflush(stdout);
        XRecordFreeData(data);

#undef bEvent
#undef bPointer
    }
}

X11MouseListener::X11MouseListener()
{
    SDEBUG
    init();
    
    if (!m_doubleClickTimer)
    {
        m_doubleClickTimer = new QElapsedTimer;
        m_doubleClickTimer->start();
    }
}

X11MouseListener::~X11MouseListener()
{
    SDEBUG
    if (m_instance)
    {
        delete m_instance;
    }
    m_instance = nullptr;
}

void X11MouseListener::init()
{
    SDEBUG
    display = XOpenDisplay(NULL);
    if (!display) 
    {
        fprintf(stderr, "Could not open display\n");
        return;
    }

    /*  The client set may be one of the following constants: 
            XRecordCurrentClients
            XRecordFutureClients
            XRecordAllClients
    */ 
    XRecordClientSpec clients = XRecordAllClients;

    /*  The XRecordAllocRange function allocates and returns an XRecordRange structure. 
        The structure is initialized to specify no protocol. The function returns NULL 
        if the structure allocation fails. The application can free the structure by 
        calling XFree */
    XRecordRange* range = XRecordAllocRange();

    if (range == NULL)
    {
        fprintf(stderr, "Could not alloc range\n");
        return;
    }

    memset(range, 0, sizeof(XRecordRange));
    // range is 4-6, ButtonPress, ButtonRelease, MotionNotify
    range->device_events.first = ButtonPress; // 4; ButtonRelease 5
    range->device_events.last = ButtonRelease; // 6

    /*  XRecordCreateContext returns zero if the request failed. 
        XRecordCreateContext can generate BadIDChoice BadMatch and BadValue errors. */
    context = XRecordCreateContext(display, 0, &clients, 1, &range, 1);
    if (context == 0)
    {
        fprintf(stderr, "XRecordCreateContext failed\n");
        return;
    }

    XFree(range);
    XSync(display, True); // 不知道为啥要写True； https://tronche.com/gui/x/xlib/event-handling/XSync.html
}

void X11MouseListener::enableContext()
{
    SDEBUG
    Status ret = XRecordEnableContext(display, context,  callback, nullptr);
    XCloseDisplay(display);
    display = nullptr;   
}

