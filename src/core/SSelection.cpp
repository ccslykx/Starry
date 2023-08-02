#include "SSelection.h"
#include "utils.h"

#include <QGuiApplication>
#include <QProcessEnvironment>

SSelection* SSelection::m_instance = nullptr;
QClipboard* SSelection::m_clipboard = nullptr;

SSelection* SSelection::instance()
{
    if (!m_instance)
    {
        m_instance = new SSelection;
    }
    return m_instance;
}

void SSelection::refresh()
{
    SDEBUG
    QString tmp;
#ifdef __linux__
    QString dpEnv = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
    if (dpEnv.toUpper() == "X11")
    {
        tmp = m_clipboard->text(QClipboard::Mode::Selection);
    } else if (dpEnv.toUpper() == "WAYLAND")
    {
        tmp = simulateCopy_linux();
    }
#elif __APPLE__ && TARGET_OS_MAC /* Need Test */
        /* Any better method? */
        tmp = simulateCopy_mac();
#elif _WIN32
    tmp = simulateCopy_win();
#endif
    /*TEST*/m_selection = "SELECTION";
    if (m_selection != tmp)
    {
        m_selection = tmp;
        emit selectionChanged();
    }
}

QString SSelection::selection()
{
    return m_selection;
}

/* private functions */

SSelection::SSelection()
{
    if (!m_clipboard)
    {
        m_clipboard = QGuiApplication::clipboard();
    }
}

QString SSelection::simulateCopy_win()
{
    SDEBUG
    QString tmp = "TEST";
    return std::move(tmp); /*TODO*/
}

QString SSelection::simulateCopy_linux()
{
    SDEBUG
    return "TEST";   
}

QString SSelection::simulateCopy_mac()
{
    SDEBUG
    return "TEST";
}