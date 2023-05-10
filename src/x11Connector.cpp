#include <QtConcurrent>

#include "x11Connector.h"

#include <stdio.h>
#include <X11/Xlib.h> // 链接添加 x11
#include <X11/Xatom.h>
#include <X11/Xproto.h> // xEvent

// 业务逻辑参考同时参考了https://juejin.cn/post/7175744686437924922

static void callback(XPointer ptr, XRecordInterceptData *data)
{
    Q_UNUSED(ptr)
    X11Connector* instance = X11Connector::instance();
    instance->handleRecordEvent(data);
}

/* X11Connector */

X11Connector*   X11Connector::m_instance = nullptr;

XRecordContext  X11Connector::m_context = 0;
Display*        X11Connector::m_display = nullptr;

MouseStatus     X11Connector::lastMouseStatus = {-1, -1, False};
MouseStatus     X11Connector::currMouseStatus = {-1, -1, False};

QElapsedTimer*  X11Connector::m_doubleClickTimer = nullptr;

X11Connector* X11Connector::instance()
{
    if (!m_instance)
    {
        m_instance = new X11Connector;
    }
    
    return m_instance;
}

void X11Connector::run()
{
    QtConcurrent::run(enableContext);
    return;
}

void X11Connector::stop()
// 参考：https://juejin.cn/post/7175744686437924922
{
    if (m_context == 0) 
    {
        return;
    }
    Display* display = XOpenDisplay(nullptr);         // 这里需要单独建立一个连接来关闭监听，否则XRecordEnableContext不会退出
    if(!display)
    {
        qWarning() << "Could not open display";
        return;
    }
    XRecordDisableContext(display, m_context);
    XFlush(display);
    XSync(display, false);

    XRecordFreeContext(display, m_context);           // 释放监听上下文，否则XRecordEnableContext不会退出
    m_context = 0;
    XCloseDisplay(display);
}

void X11Connector::init()
{
    m_display = XOpenDisplay(NULL);
    if (!m_display) 
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
    m_context = XRecordCreateContext(m_display, 0, &clients, 1, &range, 1);
    if (m_context == 0)
    {
        fprintf(stderr, "XRecordCreateContext failed\n");
        return;
    }

    XFree(range);
    XSync(m_display, True); // 不知道为啥要写True； https://tronche.com/gui/x/xlib/event-handling/XSync.html


    /* ------------------------------------------------------ */

    // Display *display_datalink;
    // display_datalink = XOpenDisplay(NULL);
    // if (!display_datalink)
    // {
    //     fprintf(stderr, "Could not open display for data link\n");
    //     return;
    // }

    // XRecordInterceptData *xrid = NULL;

    // if (!XRecordEnableContext(display_datalink, context, callback, (XPointer) xrid))
    // {
    //     fprintf(stderr, "XRecordEnableContextAsync error");
    //     return;
    // }
}

void X11Connector::enableContext()
// 参考：https://juejin.cn/post/7175744686437924922
{
    Status ret = XRecordEnableContext(m_display, m_context,  callback, nullptr);
    XCloseDisplay(m_display);
    m_display = nullptr;
}

void X11Connector::handleRecordEvent(XRecordInterceptData *data)
{
    // 虽然`m_doubleClickTimer`的初始化写在这里不太合适，但是避免了初始化和start、stop调用出现在不同线程的问题


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
                emit X11Connector::instance()->B1Pressed(currMouseStatus);
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
                emit X11Connector::instance()->B1Released(currMouseStatus);

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
                        emit X11Connector::instance()->B1DoubleClicked(currMouseStatus);
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

MouseMotion X11Connector::getMouseMotivation(MouseStatus startStatus, MouseStatus endStatus)
{
    if (startStatus.x == endStatus.x && startStatus.y == endStatus.y)
    {
        return NotMoved;
    }
    else if (startStatus.x < endStatus.x && startStatus.y == endStatus.y)
    {
        return Left2Right;
    }
    else if (startStatus.x > endStatus.x && startStatus.y == endStatus.y)
    {
        return Right2Left;
    }
    else if (startStatus.x == endStatus.x && startStatus.y < endStatus.y)
    {
        return Top2Bottom;
    }
    else if (startStatus.x == endStatus.x && startStatus.y > endStatus.y)
    {
        return Bottom2Top;
    }
    if (startStatus.x < endStatus.x && startStatus.y < endStatus.y)
    {
        return TopLeft2BottomRight;
    } 
    else if (startStatus.x < endStatus.x && startStatus.y > endStatus.y)
    {
        return BottomLeft2TopRight;
    }
    else if (startStatus.x > endStatus.x && startStatus.y < endStatus.y)
    {
        return TopRight2BottomLeft;
    }
    else if (startStatus.x > endStatus.x && startStatus.y > endStatus.y)
    {
        return BottomRight2TopLeft;
    }
}

X11Connector::X11Connector()
{
    init();
    
    if (!m_doubleClickTimer)
    {
        m_doubleClickTimer = new QElapsedTimer;
        m_doubleClickTimer->start();
    }
}

X11Connector::~X11Connector()
{
    if (m_instance)
    {
        delete m_instance;
    }
    m_instance = nullptr;
}