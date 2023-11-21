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

#include <QtConcurrent>
#include "WaylandMouseListener.h"

WaylandMouseListener* WaylandMouseListener::m_instance = nullptr;

void* WaylandMouseListener::m_libinput = nullptr;
void* WaylandMouseListener::m_udev = nullptr;
bool WaylandMouseListener::m_running = false;

double WaylandMouseListener::m_x = 0;
double WaylandMouseListener::m_y = 0;
bool WaylandMouseListener::m_pressed = false;

QElapsedTimer* WaylandMouseListener::m_doubleClickTimer = nullptr;


static int open_restricted(const char *path, int flags, void *user_data)
{
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
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
    uint32_t button = libinput_event_pointer_get_button((libinput_event_pointer *)ev);
    if (button != 272)
    {
        return;
    }
    /* libinput_button_state
        0: released
        1: pressed
    */
    libinput_button_state state = libinput_event_pointer_get_button_state((libinput_event_pointer *)ev);
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
    m_running = true;
    QtConcurrent::run([](){
        struct libinput_event *ev;
        m_udev = (void*) udev_new();
        
        m_libinput = (void*) libinput_udev_create_context(&interface, NULL, (struct udev*) m_udev);
        if (!m_libinput)
        {
            qWarning() << "libinput_udev_create_context failed";
        }
        if (libinput_udev_assign_seat((struct libinput*) m_libinput, "seat0") != 0)
        {
            qWarning() << "libinput_udev_assign_seat failed";
        }

// https://github.com/JoseExposito/touchegg/blob/686bff369ddfc7122180325dcbdc20a069396bd9/src/gesture-gatherer/libinput-gesture-gatherer.cpp
        int fd = libinput_get_fd((struct libinput*) m_libinput);
        if (fd == -1)
        {
            qWarning() << "libinput_get_fd failed";
        }
        int pollTimeout = -1;
        std::array<struct pollfd, 1> pollFds{{fd, POLLIN, 0}};

        while (m_running && (poll(pollFds.data(), pollFds.size(), pollTimeout) >= 0) )
        {
            int dpErrno = libinput_dispatch((struct libinput*) m_libinput);
            if (dpErrno)
            {
                qWarning() << "libinput_dispatch error with errno:" << dpErrno;
            }

            while((ev = libinput_get_event((struct libinput*) m_libinput)))
            {
                handleEvent(ev);
                libinput_event_destroy(ev);
                libinput_dispatch((struct libinput*) m_libinput);
            }
        }
    
    });
}

void WaylandMouseListener::stopListen()
{
    if (m_libinput)
    {
        libinput_unref((struct libinput*) m_libinput);
    }
    if (m_udev)
    {
        udev_unref((struct udev*) m_udev);
    }
    m_running = false;
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
    if (m_instance)
    {
        delete m_instance;
    }
    m_instance = nullptr;
}
