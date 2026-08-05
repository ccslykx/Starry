#include "Platform/WinSelectionProvider.h"

#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <Windows.h>
#include <WinUser.h>
#include <UIAutomation.h>

#include <QEventLoop>
#include <QFutureWatcher>
#include <QScopedValueRollback>
#include <QStringList>
#include <QtConcurrent>

#include <limits>
#include <utility>
#include <vector>

namespace
{
constexpr DWORD kWindowMessageTimeoutMs = 100;
constexpr qsizetype kMaxFallbackTextCharacters = 4 * 1024 * 1024;
constexpr int kMaxAutomationParentDepth = 8;
bool selectionRefreshInProgress = false;

template<typename T>
class ComPtr
{
public:
    ComPtr() = default;
    explicit ComPtr(T *value) : m_value(value) {}

    ComPtr(const ComPtr &) = delete;
    ComPtr &operator=(const ComPtr &) = delete;

    ComPtr(ComPtr &&other) noexcept : m_value(other.detach()) {}

    ComPtr &operator=(ComPtr &&other) noexcept
    {
        if (this != &other)
        {
            reset(other.detach());
        }
        return *this;
    }

    ~ComPtr()
    {
        reset();
    }

    T *get() const { return m_value; }
    T *operator->() const { return m_value; }

    T **put()
    {
        reset();
        return &m_value;
    }

    void reset(T *value = nullptr)
    {
        if (m_value)
        {
            m_value->Release();
        }
        m_value = value;
    }

    T *detach()
    {
        T *value = m_value;
        m_value = nullptr;
        return value;
    }

    explicit operator bool() const { return m_value != nullptr; }

private:
    T *m_value = nullptr;
};

class ComApartment
{
public:
    ComApartment()
        : m_result(CoInitializeEx(nullptr, COINIT_MULTITHREADED))
    {
    }

    ~ComApartment()
    {
        if (SUCCEEDED(m_result))
        {
            CoUninitialize();
        }
    }

    bool isReady() const { return SUCCEEDED(m_result); }

private:
    HRESULT m_result;
};

HWND focusedWindow()
{
    const HWND foregroundWindow = GetForegroundWindow();
    if (!foregroundWindow)
    {
        return nullptr;
    }

    const DWORD foregroundThread = GetWindowThreadProcessId(foregroundWindow, nullptr);
    GUITHREADINFO threadInfo = {};
    threadInfo.cbSize = sizeof(threadInfo);
    if (foregroundThread && GetGUIThreadInfo(foregroundThread, &threadInfo) && threadInfo.hwndFocus)
    {
        return threadInfo.hwndFocus;
    }
    return foregroundWindow;
}

QString textFromAutomationPattern(IUIAutomationTextPattern *pattern)
{
    ComPtr<IUIAutomationTextRangeArray> ranges;
    if (FAILED(pattern->GetSelection(ranges.put())) || !ranges)
    {
        return {};
    }

    int rangeCount = 0;
    if (FAILED(ranges->get_Length(&rangeCount)) || rangeCount <= 0)
    {
        return {};
    }

    QStringList selectedRanges;
    selectedRanges.reserve(rangeCount);
    for (int index = 0; index < rangeCount; ++index)
    {
        ComPtr<IUIAutomationTextRange> range;
        if (FAILED(ranges->GetElement(index, range.put())) || !range)
        {
            continue;
        }

        BSTR selectedText = nullptr;
        const HRESULT textResult = range->GetText(-1, &selectedText);
        if (SUCCEEDED(textResult) && selectedText)
        {
            const QString text = QString::fromWCharArray(
                selectedText,
                static_cast<qsizetype>(SysStringLen(selectedText)));
            if (!text.isEmpty())
            {
                selectedRanges.append(text);
            }
        }
        if (selectedText)
        {
            SysFreeString(selectedText);
        }
    }
    return selectedRanges.join(QLatin1Char('\n'));
}

QString textFromAutomationElement(
    IUIAutomation *automation,
    IUIAutomationElement *initialElement)
{
    if (!initialElement)
    {
        return {};
    }

    ComPtr<IUIAutomationTreeWalker> walker;
    automation->get_ControlViewWalker(walker.put());

    initialElement->AddRef();
    ComPtr<IUIAutomationElement> element(initialElement);
    for (int depth = 0; element && depth <= kMaxAutomationParentDepth; ++depth)
    {
        BOOL isPassword = FALSE;
        if (SUCCEEDED(element->get_CurrentIsPassword(&isPassword)) && isPassword)
        {
            return {};
        }

        void *patternObject = nullptr;
        if (SUCCEEDED(element->GetCurrentPatternAs(
                UIA_TextPatternId,
                IID_IUIAutomationTextPattern,
                &patternObject))
            && patternObject)
        {
            ComPtr<IUIAutomationTextPattern> pattern(
                static_cast<IUIAutomationTextPattern *>(patternObject));
            const QString selection = textFromAutomationPattern(pattern.get());
            if (!selection.isEmpty())
            {
                return selection;
            }
        }

        if (!walker)
        {
            break;
        }
        ComPtr<IUIAutomationElement> parent;
        if (FAILED(walker->GetParentElement(element.get(), parent.put())))
        {
            break;
        }
        element = std::move(parent);
    }
    return {};
}

QString selectedTextWithUiAutomation()
{
    ComApartment apartment;
    if (!apartment.isReady())
    {
        return {};
    }

    ComPtr<IUIAutomation> automation;
    if (FAILED(CoCreateInstance(
            CLSID_CUIAutomation,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IUIAutomation,
            reinterpret_cast<void **>(automation.put())))
        || !automation)
    {
        return {};
    }

    ComPtr<IUIAutomationElement> focusedElement;
    if (SUCCEEDED(automation->GetFocusedElement(focusedElement.put())) && focusedElement)
    {
        const QString selection = textFromAutomationElement(
            automation.get(), focusedElement.get());
        if (!selection.isEmpty())
        {
            return selection;
        }
    }

    POINT cursorPosition = {};
    ComPtr<IUIAutomationElement> pointedElement;
    if (GetCursorPos(&cursorPosition)
        && SUCCEEDED(automation->ElementFromPoint(cursorPosition, pointedElement.put()))
        && pointedElement)
    {
        return textFromAutomationElement(automation.get(), pointedElement.get());
    }
    return {};
}

bool isClassicTextControl(HWND window)
{
    wchar_t className[256] = {};
    const int classNameLength = GetClassNameW(window, className, ARRAYSIZE(className));
    if (classNameLength <= 0)
    {
        return false;
    }

    const QString normalizedClass = QString::fromWCharArray(className, classNameLength).toUpper();
    return normalizedClass == QStringLiteral("EDIT")
        || normalizedClass.contains(QStringLiteral("RICHEDIT"))
        || normalizedClass.startsWith(QStringLiteral("WINDOWSFORMS10.EDIT"));
}

bool sendTextControlMessage(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD_PTR *result)
{
    SetLastError(ERROR_SUCCESS);
    return SendMessageTimeoutW(
        window,
        message,
        wParam,
        lParam,
        SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
        kWindowMessageTimeoutMs,
        result) != 0;
}

QString selectedTextWithWindowMessages(HWND window)
{
    if (!window || !IsWindow(window) || !isClassicTextControl(window))
    {
        return {};
    }

    const LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    if ((style & ES_PASSWORD) != 0)
    {
        return {};
    }

    DWORD selectionStart = std::numeric_limits<DWORD>::max();
    DWORD selectionEnd = std::numeric_limits<DWORD>::max();
    DWORD_PTR ignoredResult = 0;
    if (!sendTextControlMessage(
            window,
            EM_GETSEL,
            reinterpret_cast<WPARAM>(&selectionStart),
            reinterpret_cast<LPARAM>(&selectionEnd),
            &ignoredResult)
        || selectionStart == std::numeric_limits<DWORD>::max()
        || selectionEnd == std::numeric_limits<DWORD>::max()
        || selectionStart == selectionEnd)
    {
        return {};
    }

    if (selectionStart > selectionEnd)
    {
        std::swap(selectionStart, selectionEnd);
    }

    DWORD_PTR textLengthResult = 0;
    if (!sendTextControlMessage(
            window,
            WM_GETTEXTLENGTH,
            0,
            0,
            &textLengthResult))
    {
        return {};
    }

    const qsizetype textLength = static_cast<qsizetype>(textLengthResult);
    if (textLength <= 0
        || textLength > kMaxFallbackTextCharacters
        || selectionEnd > static_cast<DWORD>(textLength))
    {
        return {};
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(textLength) + 1, L'\0');
    DWORD_PTR copiedLengthResult = 0;
    if (!sendTextControlMessage(
            window,
            WM_GETTEXT,
            static_cast<WPARAM>(buffer.size()),
            reinterpret_cast<LPARAM>(buffer.data()),
            &copiedLengthResult))
    {
        return {};
    }

    const qsizetype copiedLength = static_cast<qsizetype>(copiedLengthResult);
    if (selectionEnd > static_cast<DWORD>(copiedLength))
    {
        return {};
    }

    return QString::fromWCharArray(
        buffer.data() + selectionStart,
        static_cast<qsizetype>(selectionEnd - selectionStart));
}

QString selectedTextWhileKeepingUiResponsive(HWND textControl)
{
    QFutureWatcher<QString> watcher;
    QEventLoop eventLoop;
    QObject::connect(
        &watcher,
        &QFutureWatcher<QString>::finished,
        &eventLoop,
        &QEventLoop::quit);

    watcher.setFuture(QtConcurrent::run([textControl]() {
        QString selection = selectedTextWithWindowMessages(textControl);
        if (selection.isEmpty())
        {
            selection = selectedTextWithUiAutomation();
        }
        return selection;
    }));

    // UI Automation can synchronously call back into the target process.
    // Keep dispatching GUI and low-level mouse-hook messages while preserving
    // the synchronous result contract expected by SMouseListener.
    if (!watcher.isFinished())
    {
        eventLoop.exec();
    }
    return watcher.result();
}
}

bool WinSelectionProvider::tryQuerySelectedText(QString &selection)
{
    // The nested event loop may deliver another selection timer event. Let the
    // original request finish instead of starting another automation query.
    if (selectionRefreshInProgress)
    {
        return false;
    }

    QScopedValueRollback<bool> refreshGuard(selectionRefreshInProgress, true);
    selection = selectedTextWhileKeepingUiResponsive(focusedWindow());
    return true;
}
