#pragma once

class MacAccessibilityPermission final
{
public:
    static bool isGranted();
    static bool request();

private:
    MacAccessibilityPermission() = delete;
};
