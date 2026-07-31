#include "Core/Platform/MacAccessibilityPermission.h"

#include <ApplicationServices/ApplicationServices.h>

bool MacAccessibilityPermission::isGranted()
{
    return AXIsProcessTrusted();
}

bool MacAccessibilityPermission::request()
{
    const void *keys[] = {kAXTrustedCheckOptionPrompt};
    const void *values[] = {kCFBooleanTrue};
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault,
        keys,
        values,
        1,
        &kCFCopyStringDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
    if (!options)
    {
        return false;
    }

    const bool granted = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
    return granted;
}
