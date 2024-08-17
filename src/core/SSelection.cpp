#include "SSelection.h"
#include "utils.h"

#include <QGuiApplication>
#include <QProcessEnvironment>

#ifdef __linux__
#elif __APPLE__
#   include <ApplicationServices/ApplicationServices.h>
#elif _WIN32
#   include <Windows.h>
#   include <WinUser.h>
#   include <string>
#   include <vector>
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
    if (!tmp.isEmpty() && (m_selection != tmp))
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

QString SSelection::getSelection_win()
{
    SDEBUG
    QString res = QString();

#ifdef _WIN32
    /* Backup Clipboard */
    HWND hwndCurrent = GetForegroundWindow();

    // std::vector<std::string> buffers;
    // std::vector<size_t> bufLens;
    // std::vector<UINT> formats;

    // if (!OpenClipboard(hwndCurrent))
    // {
    //     qWarning() << "Error while reading clipboard";
    //     return res;
    // }

    // qDebug() << "Clipboard has formats count:" << CountClipboardFormats();
    // UINT fmt = EnumClipboardFormats(0);
    // while (fmt = EnumClipboardFormats(fmt))
    // {
    //     WCHAR name[64];
    //     GetClipboardFormatName(fmt, name, 64);
    //     HGLOBAL hglb = GetClipboardData(fmt);
    //     if (hglb == NULL)
    //     {
    //         DWORD error = GetLastError();
    //         qDebug() << "Error when calling GetClipboardData(), error:" << error << ", code:" << fmt << ", Clipboard format:" << *name;
    //         continue;
    //     }
    //     LPVOID tmp = GlobalLock(hglb);
    //     if (tmp == NULL)
    //     {
    //         DWORD error = GetLastError();
    //         qDebug() << "Error when calling GlobalLock(), error:" << error << ", code:" << fmt << ", Clipboard format:" << *name;
    //         GlobalUnlock(hglb);
    //         continue;
    //     }
    //     std::string buf = std::string(static_cast<char*>(tmp));
    //
    //     formats.push_back(fmt);
    //     bufLens.push_back(buf.size());
    //     buffers.push_back(buf);
    //     GlobalUnlock(hglb);
    // }
    // CloseClipboard();

    /* Copying Information to the Clipboard */
    // qDebug() << "Copying Information to the Clipboard";

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

    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (uSent != ARRAYSIZE(inputs))
    {
        qWarning() << "Simulate 'Ctrl + C' failed" << HRESULT_FROM_WIN32(GetLastError());
    }
    Sleep(10);

    /* Get Selections */
    if (OpenClipboard(hwndCurrent))
    {
        if (IsClipboardFormatAvailable(CF_TEXT))
        {
            HGLOBAL hglbSelection = GetClipboardData(CF_TEXT);
            LPVOID tmp = NULL;
            if (hglbSelection)
            {
                tmp = GlobalLock(hglbSelection);
            }
            if (tmp)
            {
                res = QString::fromStdString(std::string(static_cast<char*>(tmp)));
            }
            GlobalUnlock(hglbSelection);
        }
        CloseClipboard();
    }

    /* Restore Clipboard */

    /* Restoring clipboard may crash other application after copied files */

    // if (OpenClipboard(hwndCurrent))
    // {
    //     if (!EmptyClipboard())
    //     {
    //         qWarning() << "Empty Clipboard failed";
    //         CloseClipboard();
    //         return res;
    //     }
    //     if (formats.empty())
    //     {
    //         CloseClipboard();
    //         return res;
    //     }
    //     for (int i = 0; i < formats.size(); ++i)
    //     {
    //         if (buffers[i].empty()) continue;
    //         WCHAR name[256];
    //         GetClipboardFormatName(formats[i], name, 256);
    //         qDebug() << "Restoring Clipboard of Format:" << *name << ", length:" << bufLens[i];
    //         HANDLE restore = GlobalAlloc(GMEM_MOVEABLE, bufLens[i] + 1);
    //         char *buffer = (char*) GlobalLock(restore);
    //         strcpy_s(buffer, bufLens[i] + 1, buffers[i].c_str());
    //         SetClipboardData(formats[i], restore);
    //         GlobalUnlock(restore);
    //         if (strlen(buffer) == strlen(buffers[i].c_str()))
    //             { qDebug() << "Equal"; } else { qDebug() << "Not Equal";
    //             qDebug() << "buffer:" << buffer << "."; qDebug() << "c_str():" << buffers[i].c_str() << "."; }
    //     }
    //
    //     CloseClipboard();
    //     qDebug() << "CloseClipboard();";
    // }
#endif

    return res;
}

QString SSelection::getSelection_linux()
{
    SDEBUG
    return "TEST";
}

QString SSelection::getSelection_mac()
{
    SDEBUG
    QString res = QString();
#ifdef __APPLE__
    /* Get Selection through accessibility APIs */
    // Ref: https://stackoverflow.com/questions/76009610/get-selected-text-when-in-any-application-on-macos
    AXUIElementRef systemWideElement = AXUIElementCreateSystemWide();
    CFTypeRef *selectedTextValue = new CFTypeRef;
    AXError errorCode = AXUIElementCopyAttributeValue(systemWideElement, kAXFocusedUIElementAttribute, selectedTextValue);
    if (errorCode != kAXErrorSuccess)
    {
        qDebug() << "errorCode:" << errorCode;
        delete selectedTextValue;
        return std::move(res);
    }
    AXUIElementRef selectedTextElement = (AXUIElementRef)*selectedTextValue;
    CFStringRef *selectedTextString = new CFStringRef;
    AXError textErrorCode = AXUIElementCopyAttributeValue(selectedTextElement, kAXSelectedTextAttribute, (CFTypeRef *)selectedTextString);
    if (textErrorCode != kAXErrorSuccess)
    {
        qDebug() << "textErrorCode:" << textErrorCode;
        delete selectedTextString;
    }
        res = QString::fromCFString(*selectedTextString);
        qDebug() << "selectedTextString:" << res;
    delete selectedTextString;
    delete selectedTextValue;
#endif

    return std::move(res);
}