#include <QtTest>

#include "SPluginCommand.h"

class SPluginCommandTests : public QObject
{
    Q_OBJECT

private slots:
    void expandsEmbeddedPlaintextWithoutRetokenizing();
    void percentEncodesTextForUrls();
    void expandsRepeatedPlaceholders();
    void doesNotRecursivelyExpandSelectedText();
    void rejectsSelectionPlaceholdersInProgramName();
    void rejectsEmptyCommands();
};

void SPluginCommandTests::expandsEmbeddedPlaintextWithoutRetokenizing()
{
    SPluginCommand command;
    QString error;
    const QString selection = QStringLiteral("hello world; $(touch ignored)");

    QVERIFY(SPluginCommand::parse(
        QStringLiteral("open https://www.google.com/search?q=$PLAINTEXT"),
        selection,
        &command,
        &error));
    QCOMPARE(error, QString());
    QCOMPARE(command.program, QStringLiteral("open"));
    QCOMPARE(command.arguments.size(), 1);
    QCOMPARE(
        command.arguments.constFirst(),
        QStringLiteral("https://www.google.com/search?q=") + selection);
}

void SPluginCommandTests::percentEncodesTextForUrls()
{
    SPluginCommand command;

    QVERIFY(SPluginCommand::parse(
        QStringLiteral("open https://example.com/search?q=$URLENCODED"),
        QStringLiteral("C++ & 中文#6"),
        &command));
    QCOMPARE(command.arguments.size(), 1);
    QCOMPARE(
        command.arguments.constFirst(),
        QStringLiteral(
            "https://example.com/search?q=C%2B%2B%20%26%20%E4%B8%AD%E6%96%87%236"));
}

void SPluginCommandTests::expandsRepeatedPlaceholders()
{
    SPluginCommand command;

    QVERIFY(SPluginCommand::parse(
        QStringLiteral("tool --raw=$PLAINTEXT/$PLAINTEXT --url=$URLENCODED"),
        QStringLiteral("a b"),
        &command));
    QCOMPARE(
        command.arguments,
        QStringList({
            QStringLiteral("--raw=a b/a b"),
            QStringLiteral("--url=a%20b"),
        }));
}

void SPluginCommandTests::doesNotRecursivelyExpandSelectedText()
{
    SPluginCommand command;

    QVERIFY(SPluginCommand::parse(
        QStringLiteral("tool $PLAINTEXT"),
        QStringLiteral("$URLENCODED"),
        &command));
    QCOMPARE(command.arguments, QStringList({QStringLiteral("$URLENCODED")}));
}

void SPluginCommandTests::rejectsSelectionPlaceholdersInProgramName()
{
    SPluginCommand command;
    QString error;

    QVERIFY(!SPluginCommand::parse(
        QStringLiteral("/tmp/$PLAINTEXT --version"),
        QStringLiteral("program"),
        &command,
        &error));
    QVERIFY(error.contains(QStringLiteral("arguments")));
}

void SPluginCommandTests::rejectsEmptyCommands()
{
    SPluginCommand command;
    QString error;

    QVERIFY(!SPluginCommand::parse(QStringLiteral("   "), QString(), &command, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(SPluginCommandTests)

#include "SPluginCommandTests.moc"
