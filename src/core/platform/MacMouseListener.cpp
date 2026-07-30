#include "MacMouseListener.h"
#include <QDebug>
#include <QCursor>
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h> // 请求辅助功能权限

MacMouseListener* MacMouseListener::m_instance = nullptr;
MouseStatus MacMouseListener::lastMouseStatus = {-1, -1, false};
MouseStatus MacMouseListener::currMouseStatus = {-1, -1, false};

QElapsedTimer* MacMouseListener::m_doubleClickTimer = nullptr;

CFMachPortRef MacMouseListener::m_eventTap = nullptr;
CFRunLoopRef MacMouseListener::m_runLoop = nullptr;
CFRunLoopSourceRef MacMouseListener::m_runLoopSource = nullptr;

MacMouseListener* MacMouseListener::instance()
{
    if (!m_instance)
    {
        m_instance = new MacMouseListener;
    }
    return m_instance;
}

CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
    CGEventRef event, void *userInfo)
{
    Q_UNUSED(proxy)
    Q_UNUSED(userInfo)
    MacMouseListener::instance()->handleCGEvent(type);
    
    return event; 
}


void MacMouseListener::startListen()
{
    if (m_eventTap)
    {
        CGEventTapEnable(m_eventTap, true);
        return;
    }
    createEventTap();
}

void MacMouseListener::stopListen()
{
    if (m_eventTap)
    {
        CGEventTapEnable(m_eventTap, false);
    }
    if (m_runLoop && m_runLoopSource)
    {
        CFRunLoopRemoveSource(m_runLoop, m_runLoopSource, kCFRunLoopCommonModes);
    }
    if (m_runLoopSource)
    {
        CFRelease(m_runLoopSource);
        m_runLoopSource = nullptr;
    }
    if (m_eventTap)
    {
        CFRelease(m_eventTap);
        m_eventTap = nullptr;
    }
    m_runLoop = nullptr;
}

void MacMouseListener::handleCGEvent(CGEventType type)
{
    QPoint point = QCursor::pos();
    MouseMotion motion;
    switch (type)
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

bool MacMouseListener::createEventTap()
{
    const CGEventMask eventMask = CGEventMaskBit(kCGEventLeftMouseUp)
        | CGEventMaskBit(kCGEventLeftMouseDown);
    m_eventTap = CGEventTapCreate(
        CGEventTapLocation::kCGHIDEventTap,
        CGEventTapPlacement::kCGHeadInsertEventTap,
        CGEventTapOptions::kCGEventTapOptionDefault,
        eventMask, callback, nullptr);

    if (!m_eventTap)
    {
        qWarning() << "Failed to create the macOS event tap. Accessibility permission may be missing.";
        return false;
    }

    m_runLoop = CFRunLoopGetMain();
    m_runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_eventTap, 0);
    if (!m_runLoopSource)
    {
        qWarning() << "Failed to create the macOS event-tap run-loop source";
        CFRelease(m_eventTap);
        m_eventTap = nullptr;
        m_runLoop = nullptr;
        return false;
    }
    CFRunLoopAddSource(m_runLoop, m_runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(m_eventTap, true);
    return true;
}
