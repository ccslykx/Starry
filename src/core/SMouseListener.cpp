#include <QObject>
#include <QCursor>
#include <QProcessEnvironment>
#include <QGuiApplication>
#include <QClipboard>

#include <math.h>

#include "SMouseListener.h"
#include "utils.h"

#ifdef __linux__
    #include "X11MouseListener.h"    
    #include "WaylandMouseListener.h"
#elif __APPLE__ && TARGET_OS_MAC /* Need Test */
    #include "MacMouseListener.h"
#elif _WIN32
    #include "WinMouseListener.h"
#endif

SMouseListener* SMouseListener::m_instance = nullptr;

SMouseListener* SMouseListener::instance()
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SMouseListener;
    }
    return m_instance;
}

AbstractMouseListener* SMouseListener::listener()
{
    SDEBUG
    if (!m_listener)
    {
        init();
    }
    return m_listener;
}

void SMouseListener::startListen()
{
    SDEBUG
    if (!m_listener)
    {
        qWarning() << "No supported mouse listener is available";
        return;
    }
    if (m_listening)
    {
        return;
    }
    m_listener->startListen();
    m_listening = true;
}

void SMouseListener::stopListen()
{
    SDEBUG
    if (!m_listener || !m_listening)
    {
        return;
    }
    m_listener->stopListen();
    m_waitSelectionChangeTimer->stop();
    m_waitB1ReleaseTimer->stop();
    m_selectionChanged = false;
    m_B1Released = false;
    m_listening = false;
}

bool SMouseListener::isListening()
{
    return m_listening;
}

/* private functions */

SMouseListener::SMouseListener()
{
    SDEBUG
    init();
    m_pressPos = QCursor::pos();
}

SMouseListener::~SMouseListener()
{
    SDEBUG
    stopListen();
    m_instance = nullptr;
}

void SMouseListener::init()
{
    if (m_listener)
    {
        m_listener->stopListen();
        m_listener->deleteLater();
    }
    SDEBUG
#ifdef __linux__
 qDebug() << "Defined __linux__";
     QString dpEnv = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
     const QString platformName = QGuiApplication::platformName().toUpper();
     if (dpEnv.toUpper() == "X11" || platformName == "XCB")
     {
 qDebug() << "X11";
         m_listener = X11MouseListener::instance();
     } else if (dpEnv.toUpper() == "WAYLAND" || platformName.contains("WAYLAND"))
     {
 qDebug() << "Wayland";
        m_listener = WaylandMouseListener::instance();
     } else
     {
        qWarning() << "Unsupported Linux display session:" << dpEnv << platformName;
     }
#elif __APPLE__ && TARGET_OS_MAC /* Need Test */
    m_listener = MacMouseListener::instance();
#elif _WIN32
qDebug() << "Defined _WIN32";
    m_listener = WinMouseListener::instance();
#endif
    // Signals
    if (m_listener)
    {
        QObject::connect(m_listener, &AbstractMouseListener::B1Pressed, this, &SMouseListener::onB1Pressed);
        QObject::connect(m_listener, &AbstractMouseListener::B1Released, this, &SMouseListener::onB1Released);
        QObject::connect(m_listener, &AbstractMouseListener::B1DoubleClicked, this, &SMouseListener::onB1DoubleClicked);
    }

    if (!m_selection)
    {
        m_selection = SSelection::instance();
    }

    // Setup Timers
    if (!m_waitSelectionChangeTimer)
    {
        m_waitSelectionChangeTimer = new QTimer(this);
        m_waitSelectionChangeTimer->setInterval(100); // TODO: change with SConfig
    }
    if (!m_waitB1ReleaseTimer)
    {
        m_waitB1ReleaseTimer = new QTimer(this);
        m_waitB1ReleaseTimer->setInterval(100); // TODO: change with SConfig
    }
    QObject::connect(m_waitSelectionChangeTimer, &QTimer::timeout, this, [this] () {
        m_selection->refresh();
        if (this->m_selectionChanged)
        {
            emit this->canShow();
            this->m_selectionChanged = false;
        }
        this->m_waitSelectionChangeTimer->stop();
    });
    QObject::connect(m_waitB1ReleaseTimer, &QTimer::timeout, this, [this] () {
        if (this->m_B1Released)
        {
            emit this->canShow();
            this->m_B1Released = false;
        }
        this->m_waitB1ReleaseTimer->stop();
    });
    QObject::connect(m_selection, &SSelection::selectionChanged, this, [this](){
        // qDebug() << "selectionChanged";
        this->m_selectionChanged = true;

        if (!this->m_B1Released) // if B1 is pressed
        {
            this->waitForB1Release();
        }
    });
}

void SMouseListener::waitForSelectionChange()
{
    if (!m_listening)
    {
        return;
    }

    if (m_waitSelectionChangeTimer->isActive())
    {
        m_waitSelectionChangeTimer->stop();
    }
    m_waitSelectionChangeTimer->start();
}

void SMouseListener::waitForB1Release()
{
    if (!m_listening)
    {
        return;
    }

    if (m_waitB1ReleaseTimer->isActive())
    {
        m_waitB1ReleaseTimer->stop();
    }
    m_waitB1ReleaseTimer->start();
}

void SMouseListener::onB1Pressed(MouseStatus status)
{
    if (!m_listening)
    {
        return;
    }
    emit B1Pressed(status);
    m_pressPos = QPoint(status.x, status.y);
    m_B1Released = false;
    m_selectionChanged = false; // 解决中文输入法导致selectionChanged的影响
}

void SMouseListener::onB1Released(MouseStatus status)
{
    if (!m_listening)
    {
        return;
    }
    emit B1Released(status);
    m_releasePos = QPoint(status.x, status.y);
    m_B1Released = true;
    // m_selection->refresh(); // Detect SelectionChanged

    double offset = sqrt(pow((double) m_releasePos.x() - (double) m_pressPos.x(), 2) + pow((double) m_releasePos.y() - (double) m_pressPos.y(), 2));
    if (offset >= 5.0) // 鼠标位移量足够，认为是划词动作
    {
        waitForSelectionChange();
    }
}

void SMouseListener::onB1DoubleClicked(MouseStatus status)
{
    if (!m_listening)
    {
        return;
    }
    emit B1DoubleClicked(status);
    waitForSelectionChange();
}
