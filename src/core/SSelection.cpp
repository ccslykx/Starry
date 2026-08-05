#include "SSelection.h"
#include "utils.h"

#ifndef _WIN32
#   include <QGuiApplication>
#endif
#include <QProcessEnvironment>

#ifdef __linux__
#elif __APPLE__
#   include "SConfig.h"

#   include <ApplicationServices/ApplicationServices.h>
#elif _WIN32
#   include "Platform/WinSelectionProvider.h"
#endif

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
    QString tmp = QString();
#ifdef __linux__
    QString dpEnv = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
    if (dpEnv.toUpper() == "X11")
    {
        tmp = m_clipboard->text(QClipboard::Mode::Selection);
    } else if (dpEnv.toUpper() == "WAYLAND")
    {
        tmp = getSelection_linux();
    }
#elif __APPLE__ && TARGET_OS_MAC /* Need Test */
        tmp = getSelection_mac();
#elif _WIN32
    if (!WinSelectionProvider::tryQuerySelectedText(tmp))
    {
        return;
    }
#endif
    if (tmp.isEmpty())
    {
        m_selection.clear();
        return;
    }
    m_selection = tmp;
    emit selectionChanged();
}

QString SSelection::selection()
{
    return m_selection;
}

/* private functions */

SSelection::SSelection()
{
#ifndef _WIN32
    if (!m_clipboard)
    {
        m_clipboard = QGuiApplication::clipboard();
    }
#endif
}

QString SSelection::getSelection_win()
{
    SDEBUG
    QString selection;
#ifdef _WIN32
    WinSelectionProvider::tryQuerySelectedText(selection);
#endif
    return selection;
}

QString SSelection::getSelection_linux()
{
    SDEBUG
#ifdef __linux__
    if (m_clipboard && m_clipboard->supportsSelection())
    {
        return m_clipboard->text(QClipboard::Mode::Selection);
    }
    qWarning() << "The current Wayland compositor does not expose primary selection through Qt";
#endif
    return {};
}

QString SSelection::getSelection_mac()
{
    SDEBUG
    QString res = QString();
#ifdef __APPLE__
    /* Get Selection through accessibility APIs */
    // Ref: https://stackoverflow.com/questions/76009610/get-selected-text-when-in-any-application-on-macos
    AXUIElementRef systemWideElement = AXUIElementCreateSystemWide();
    CFTypeRef selectedTextValue = nullptr;
    AXError errorCode = AXUIElementCopyAttributeValue(systemWideElement, kAXFocusedUIElementAttribute, &selectedTextValue);
    if (errorCode != kAXErrorSuccess)
    {
        qDebug() << "errorCode:" << errorCode;
        CFRelease(systemWideElement);
        return res;
    }
    AXUIElementRef selectedTextElement = (AXUIElementRef)selectedTextValue;
    CFStringRef selectedTextString = nullptr;
    AXError textErrorCode = AXUIElementCopyAttributeValue(selectedTextElement, kAXSelectedTextAttribute, (CFTypeRef *) &selectedTextString);
    if (textErrorCode != kAXErrorSuccess)
    {
        qDebug() << "textErrorCode:" << textErrorCode;
        CFRelease(selectedTextValue);
        CFRelease(systemWideElement);
        return res;
    }
    if (selectedTextString)
    {
        res = QString::fromCFString(selectedTextString);
        if (SConfig::config()->debugModeEnabled())
        {
            qDebug() << "Selected text:" << res;
        }
        CFRelease(selectedTextString);
    }
    CFRelease(selectedTextValue);
    CFRelease(systemWideElement);
#endif

    return res;
}
