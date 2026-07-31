#pragma once

#include <QString>
#include <QStringList>

class SPluginCommand
{
public:
    static constexpr auto PLAINTEXT_PLACEHOLDER = "$PLAINTEXT";
    static constexpr auto URL_ENCODED_PLACEHOLDER = "$URLENCODED";

    static bool parse(
        const QString &commandLine,
        const QString &selectedText,
        SPluginCommand *result,
        QString *errorMessage = nullptr);

    bool isCopyToClipboardCommand() const;

    QString program;
    QStringList arguments;
};
