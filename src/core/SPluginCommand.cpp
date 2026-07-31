#include <QProcess>
#include <QUrl>

#include "SPluginCommand.h"

namespace
{
bool containsPlaceholder(const QString &value)
{
    return value.contains(QString::fromLatin1(SPluginCommand::PLAINTEXT_PLACEHOLDER))
        || value.contains(QString::fromLatin1(SPluginCommand::URL_ENCODED_PLACEHOLDER));
}

QString expandArgument(
    QString argument,
    const QString &selectedText,
    const QString &urlEncodedText)
{
    // URL expansion comes first so placeholder-like text inside the user's
    // selection is never interpreted as another variable.
    argument.replace(
        QString::fromLatin1(SPluginCommand::URL_ENCODED_PLACEHOLDER),
        urlEncodedText);
    argument.replace(
        QString::fromLatin1(SPluginCommand::PLAINTEXT_PLACEHOLDER),
        selectedText);
    return argument;
}
}

bool SPluginCommand::parse(
    const QString &commandLine,
    const QString &selectedText,
    SPluginCommand *result,
    QString *errorMessage)
{
    if (!result)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Internal command parsing error.");
        }
        return false;
    }

    QStringList parts = QProcess::splitCommand(commandLine.trimmed());
    if (parts.isEmpty() || parts.constFirst().isEmpty())
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Enter a command to run.");
        }
        return false;
    }

    const QString program = parts.takeFirst();
    if (containsPlaceholder(program))
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral(
                "Selection placeholders can only be used in command arguments.");
        }
        return false;
    }

    const QString urlEncodedText =
        QString::fromLatin1(QUrl::toPercentEncoding(selectedText));
    for (QString &argument : parts)
    {
        argument = expandArgument(argument, selectedText, urlEncodedText);
    }

    result->program = program;
    result->arguments = parts;
    if (errorMessage)
    {
        errorMessage->clear();
    }
    return true;
}

bool SPluginCommand::isCopyToClipboardCommand() const
{
    return program == QStringLiteral("starry")
        && !arguments.isEmpty()
        && arguments.constFirst() == QStringLiteral("copy2clipboard");
}
