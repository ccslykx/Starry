/* https://github.com/mahuifa/QtGlobalEvent/blob/master/GlobalMouseKey/globalmouseevent_win.cpp */

#include "WinMouseListener.h"
#include "utils.h"

#include <QCursor>
#include <QDebug>

WinMouseListener* WinMouseListener::m_instance = nullptr;
HHOOK WinMouseListener::m_hook = nullptr;

MouseStatus         WinMouseListener::lastMouseStatus = {-1, -1, false};
MouseStatus         WinMouseListener::currMouseStatus = {-1, -1, false};

QElapsedTimer* WinMouseListener::m_doubleClickTimer = nullptr;

WinMouseListener* WinMouseListener::instance()
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new WinMouseListener;
    }
    return m_instance;
}

void WinMouseListener::startListen()
{
    SDEBUG
    if (m_hook)
    {
        return;
    }
    m_hook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandleW(nullptr), 0);
}

void WinMouseListener::stopListen()
{
    SDEBUG
    if (m_hook)
    {
        bool ret = UnhookWindowsHookEx(m_hook);
        if (ret)
        {
            m_hook = nullptr;
        }
    }
}

/* private functions */

WinMouseListener::WinMouseListener()
{
    SDEBUG
    if (!m_doubleClickTimer)
    {
        m_doubleClickTimer = new QElapsedTimer;
        m_doubleClickTimer->start();
    }
}

LRESULT CALLBACK WinMouseListener::LowLevelMouseProc (int nCode, WPARAM wParam, LPARAM lParam)
{
    QPoint point = QCursor::pos();

    // Ref: X11MouseListener.cpp X11MouseListener::handleRecordEvent
    switch (wParam)
    {
    case WM_LBUTTONDOWN:
    {
        if (lastMouseStatus.isPressed)
        {
            break;
        }
        currMouseStatus.x = point.x();
        currMouseStatus.y = point.y();
        currMouseStatus.isPressed = true;
        lastMouseStatus = currMouseStatus;
        emit WinMouseListener::instance()->B1Pressed(currMouseStatus);
        break;
    }
    case WM_LBUTTONUP:
    {
        if (!lastMouseStatus.isPressed)
        {
            break;
        }
        currMouseStatus.x = point.x();
        currMouseStatus.y = point.y();
        currMouseStatus.isPressed = false;

        MouseMotion motion = getMouseMotivation(lastMouseStatus, currMouseStatus);
        lastMouseStatus = currMouseStatus;

        emit WinMouseListener::instance()->B1Released(currMouseStatus);

        if (motion == NotMoved)
        {
            qDebug() << "Not moved";
            if (!m_doubleClickTimer->isValid())
            {
                qWarning() << "m_doubleClickTimer is not Valid";
                break;
            }
            if (!m_doubleClickTimer->hasExpired(250)) /* TODO: Set from SConfig */
            {
                qDebug() << "B1 Double Clicked with elapsed" << m_doubleClickTimer->elapsed();
                emit WinMouseListener::instance()->B1DoubleClicked(currMouseStatus);
            }
            m_doubleClickTimer->restart();
            break;
        }
        break;
    }
    default:
        break;
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

