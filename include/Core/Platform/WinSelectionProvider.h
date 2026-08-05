#pragma once

#include <QString>

class WinSelectionProvider final
{
public:
    // Returns false when another query is already in progress. A completed
    // query returns true even when no text is currently selected.
    static bool tryQuerySelectedText(QString &selection);

    WinSelectionProvider() = delete;
};
