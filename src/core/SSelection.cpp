#include "SSelection.h"

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

QString SSelection::selection()
{
    execute();
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

void SSelection::execute()
{
#ifdef __linux__
    QString dpEnv = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
    if (dpEnv.toUpper() == "X11")
    {
        m_selection = m_clipboard->text(QClipboard::Mode::Selection);
    } else if (dpEnv.toUpper() == "WAYLAND")
    {
        simulateCopy_linux();
    }
#elif __APPLE__ && TARGET_OS_MAC /* Need Test */
        /* Any better method? */
        simulateCopy_mac();
#elif _WIN32
    simulateCopy_win();
#endif
}

bool SSelection::simulateCopy_win()
{
    
}

bool SSelection::simulateCopy_linux()
{
    return false;   
}

bool SSelection::simulateCopy_mac()
{
    return false;
}