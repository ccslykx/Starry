#include "SSelection.h"
#include "utils.h"

#include <QGuiApplication>
#include <QMimeData>
#include <QProcessEnvironment>
#include <QThread>

#include <memory>

#ifdef __linux__
#elif __APPLE__
#   include <ApplicationServices/ApplicationServices.h>
#elif _WIN32
#   include <Windows.h>
#   include <WinUser.h>
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
    tmp = getSelection_win();
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
    if (!m_clipboard)
    {
        m_clipboard = QGuiApplication::clipboard();
    }
}

QString SSelection::getSelection_win()
{
    SDEBUG
    QString res;

#ifdef _WIN32
    std::unique_ptr<QMimeData> clipboardBackup = std::make_unique<QMimeData>();
    if (const QMimeData *currentMimeData = m_clipboard->mimeData())
    {
        for (const QString &format : currentMimeData->formats())
        {
            clipboardBackup->setData(format, currentMimeData->data(format));
        }
    }

    // Simulate Ctrl + C
    // Ref: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput
    INPUT inputs[4] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'C';

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'C';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    const DWORD sequenceBeforeCopy = GetClipboardSequenceNumber();
    const UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (uSent != ARRAYSIZE(inputs))
    {
        qWarning() << "Simulate 'Ctrl + C' failed" << HRESULT_FROM_WIN32(GetLastError());
        return res;
    }

    bool clipboardChanged = false;
    constexpr int maxAttempts = 20;
    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        if (GetClipboardSequenceNumber() != sequenceBeforeCopy)
        {
            clipboardChanged = true;
            break;
        }
        QThread::msleep(10);
    }

    if (clipboardChanged)
    {
        res = m_clipboard->text(QClipboard::Clipboard);
    }
    else
    {
        qWarning() << "Clipboard did not change after simulating Ctrl + C";
    }

    // QClipboard takes ownership. Restoring all advertised MIME formats also
    // preserves Unicode text and file/URL clipboard contents.
    m_clipboard->setMimeData(clipboardBackup.release(), QClipboard::Clipboard);
#endif

    return res;
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
        qDebug() << "selectedTextString:" << res;
        CFRelease(selectedTextString);
    }
    CFRelease(selectedTextValue);
    CFRelease(systemWideElement);
#endif

    return res;
}
