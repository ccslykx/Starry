#include "MacMouseListener.h"
#include <QDebug>
#include <QCursor>
#include <QtConcurrentRun>
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h> // 请求辅助功能权限

MacMouseListener* MacMouseListener::m_instance = nullptr;
MouseStatus MacMouseListener::lastMouseStatus = {-1, -1, false};
MouseStatus MacMouseListener::currMouseStatus = {-1, -1, false};

QElapsedTimer* MacMouseListener::m_doubleClickTimer = nullptr;

void* MacMouseListener::m_eventTap = nullptr;

MacMouseListener* MacMouseListener::instance()
{
    if (!m_instance)
    {
        m_instance = new MacMouseListener;
    }
    return m_instance;
}

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, 
    CGEventRef event, void * __nullable userInfo) 
{
    CGEventType *ptr;
    *ptr = type;
    MacMouseListener::instance()->handleCGEvent(static_cast<void*>(ptr));
    
    return event; 
}


void MacMouseListener::startListen()
{
    QtConcurrent::run(createEventTap);
}

void MacMouseListener::stopListen()
{
    if (m_eventTap)
    {
        CFMachPortRef* eventTap = static_cast<CFMachPortRef*>(m_eventTap);
        CGEventTapEnable(*eventTap, false);
        CFRelease(*eventTap);
    }
}

void MacMouseListener::handleCGEvent(void *type)
{
    QPoint point = QCursor::pos();
    MouseMotion motion;
    switch (*static_cast<CGEventType*>(type))
    {
    case CGEventType::kCGEventLeftMouseUp:
        if (!lastMouseStatus.isPressed)
        {
            break;
        }
        currMouseStatus.x = point.x();
        currMouseStatus.y = point.y();
        currMouseStatus.isPressed = false;

        motion = getMouseMotivation(lastMouseStatus, currMouseStatus);
        lastMouseStatus = currMouseStatus;

        emit MacMouseListener::instance()->B1Released(currMouseStatus);

        if (motion == NotMoved)
        {
            if (!m_doubleClickTimer->isValid())
            {
                qWarning() << "m_doubleClickTimer is not Valid";
                break;
            }
            if (!m_doubleClickTimer->hasExpired(250)) /* TODO: Set from SConfig */
            {
                qDebug() << "B1 Double Clicked with elapsed" << m_doubleClickTimer->elapsed();
                emit MacMouseListener::instance()->B1DoubleClicked(currMouseStatus);
            }
            m_doubleClickTimer->restart();
            break;
        }
        break;
        
    case CGEventType::kCGEventLeftMouseDown:
        if (lastMouseStatus.isPressed)
        {
            break;
        }
        currMouseStatus.x = point.x();
        currMouseStatus.y = point.y();
        currMouseStatus.isPressed = true;
        lastMouseStatus = currMouseStatus;
        emit MacMouseListener::instance()->B1Pressed(currMouseStatus);
        break;

    case CGEventType::kCGEventLeftMouseDragged:
        /* Maybe can do something? */
        qDebug() << "case CGEventType::kCGEventLeftMouseDragged:";
        break;
    default:
        qDebug() << "default";
        break;
    }
}

/* private function */
MacMouseListener::MacMouseListener()
{
    if (!m_doubleClickTimer)
    {
        m_doubleClickTimer = new QElapsedTimer;
        m_doubleClickTimer->start();
    }
}

void MacMouseListener::createEventTap()
{
    if (m_eventTap)
    {
        CFRelease(m_eventTap);
    }
    CGEventMask eventMask = (1 << CGEventType::kCGEventLeftMouseUp) | (1 << CGEventType::kCGEventLeftMouseDown);
    CFMachPortRef eventTap = CGEventTapCreate(
        CGEventTapLocation::kCGHIDEventTap, 
        CGEventTapPlacement::kCGHeadInsertEventTap,
        CGEventTapOptions::kCGEventTapOptionDefault, 
        eventMask, callback, nullptr);

    if (!eventTap)
    {
        qDebug() << "failed to create event tap";
        return;
    }

    m_eventTap = static_cast<void*>(eventTap);

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRunLoopRun();
}