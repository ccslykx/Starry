/*
    References:
    https://wayland.freedesktop.org/libinput/doc/latest/api/
    https://stackoverflow.com/questions/56746659/how-do-i-configure-libinput-devices-from-c-code

*/

#include <fcntl.h>
#include <unistd.h>
#include <libudev.h>
#include <libinput.h>
#include <poll.h>
#include <array>
#include <cerrno>

#include <QtConcurrent>
#include "WaylandMouseListener.h"

WaylandMouseListener* WaylandMouseListener::m_instance = nullptr;

std::atomic_bool WaylandMouseListener::m_running = false;

double WaylandMouseListener::m_x = 0;
double WaylandMouseListener::m_y = 0;
bool WaylandMouseListener::m_pressed = false;

QElapsedTimer* WaylandMouseListener::m_doubleClickTimer = nullptr;


static int open_restricted(const char *path, int flags, void *user_data)
{
    Q_UNUSED(user_data)
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
    Q_UNUSED(user_data)
    close(fd);
}

const static struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

void WaylandMouseListener::handleEvent(void *ev)
{
    if (!ev) 
    {
        return;
    }
    libinput_event_type type = libinput_event_get_type((libinput_event*) ev);
    if ((type == libinput_event_type::LIBINPUT_EVENT_POINTER_MOTION) && m_pressed)
    {
        m_x += libinput_event_pointer_get_dx_unaccelerated(libinput_event_get_pointer_event((libinput_event*) ev));
        m_y += libinput_event_pointer_get_dy_unaccelerated(libinput_event_get_pointer_event((libinput_event*) ev));
        qDebug() << "(x =" << m_x << ", y =" << m_y << ")";
        return;
    }
    else if (type != libinput_event_type::LIBINPUT_EVENT_POINTER_BUTTON)
    {
        return;
    }
    /* Button
        272: left
        273: right
        274: mid
        275: mouse 4
        276: mouse 5
    */
    libinput_event_pointer *pointerEvent = libinput_event_get_pointer_event((libinput_event *)ev);
    if (!pointerEvent)
    {
        return;
    }
    uint32_t button = libinput_event_pointer_get_button(pointerEvent);
    if (button != 272)
    {
        return;
    }
    /* libinput_button_state
        0: released
        1: pressed
    */
    libinput_button_state state = libinput_event_pointer_get_button_state(pointerEvent);
    if (state == 1) // pressed
    {
        if (m_pressed)
        {
            return;
        }

        m_pressed = true;
        qDebug() << "B1 Pressed";
        emit WaylandMouseListener::instance()->B1Pressed({0, 0, m_pressed});
    } else if (state == 0) { // released
        if (!m_pressed)
        {
            return;
        }

        m_pressed = false;
        qDebug() << "B1 Released";
        emit WaylandMouseListener::instance()->B1Released({(int) m_x, (int) m_y, m_pressed});

        if ((int) m_x == 0 && (int) m_y == 0)
        {
            if (!m_doubleClickTimer->isValid())
            {
                qWarning() << "m_doubleClickTimer is not Valid";
                return;
            }

            if (!m_doubleClickTimer->hasExpired(250)) // 双击检测-结束
            {                                         // 如果释放距离上次不超过250ms
                qDebug() << "B1 Double Clicked with elapsed" << m_doubleClickTimer->elapsed();
                emit WaylandMouseListener::instance()->B1DoubleClicked({0, 0, m_pressed});
            }
            m_doubleClickTimer->restart();
            return;
        }

        m_x = 0;
        m_y = 0;
    }
}

WaylandMouseListener* WaylandMouseListener::instance()
{
    if (!m_instance)
    {
        m_instance = new WaylandMouseListener;
    }
    return m_instance;
}

void WaylandMouseListener::startListen()
{
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
    {
        return;
    }
    m_future = QtConcurrent::run([] {
        struct libinput_event *ev;
        struct udev *localUdev = udev_new();
        if (!localUdev)
        {
            qWarning() << "udev_new failed";
            m_running.store(false);
            return;
        }
        
        struct libinput *localLibinput = libinput_udev_create_context(&interface, nullptr, localUdev);
        if (!localLibinput)
        {
            qWarning() << "libinput_udev_create_context failed";
            udev_unref(localUdev);
            m_running.store(false);
            return;
        }
        if (libinput_udev_assign_seat(localLibinput, "seat0") != 0)
        {
            qWarning() << "libinput_udev_assign_seat failed";
            libinput_unref(localLibinput);
            udev_unref(localUdev);
            m_running.store(false);
            return;
        }

// https://github.com/JoseExposito/touchegg/blob/686bff369ddfc7122180325dcbdc20a069396bd9/src/gesture-gatherer/libinput-gesture-gatherer.cpp
        int fd = libinput_get_fd(localLibinput);
        if (fd == -1)
        {
            qWarning() << "libinput_get_fd failed";
            libinput_unref(localLibinput);
            udev_unref(localUdev);
            m_running.store(false);
            return;
        }
        constexpr int pollTimeout = 100;
        std::array<struct pollfd, 1> pollFds{{fd, POLLIN, 0}};

        while (m_running.load())
        {
            const int pollResult = poll(pollFds.data(), pollFds.size(), pollTimeout);
            if (pollResult == 0)
            {
                continue;
            }
            if (pollResult < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                qWarning() << "Polling libinput failed with errno:" << errno;
                break;
            }

            int dpErrno = libinput_dispatch(localLibinput);
            if (dpErrno)
            {
                qWarning() << "libinput_dispatch error with errno:" << dpErrno;
            }

            while((ev = libinput_get_event(localLibinput)))
            {
                handleEvent(ev);
                libinput_event_destroy(ev);
                libinput_dispatch(localLibinput);
            }
        }
        libinput_unref(localLibinput);
        udev_unref(localUdev);
        m_running.store(false);
    });
}

void WaylandMouseListener::stopListen()
{
    m_running.store(false);
    m_future.waitForFinished();
    m_pressed = false;
    m_x = 0;
    m_y = 0;
}

/* private functions */

WaylandMouseListener::WaylandMouseListener()
{
    if (!m_doubleClickTimer)
    {
        m_doubleClickTimer = new QElapsedTimer;
        m_doubleClickTimer->start();
    }
}

WaylandMouseListener::~WaylandMouseListener()
{
    stopListen();
    m_instance = nullptr;
}
