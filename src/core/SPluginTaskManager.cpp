#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

#include "SPluginTaskManager.h"

namespace
{
#ifdef Q_OS_WIN
bool isWindowsCommandBuiltin(const QString &program)
{
    static const QStringList builtins{
        QStringLiteral("assoc"),
        QStringLiteral("break"),
        QStringLiteral("call"),
        QStringLiteral("cd"),
        QStringLiteral("chdir"),
        QStringLiteral("cls"),
        QStringLiteral("color"),
        QStringLiteral("copy"),
        QStringLiteral("date"),
        QStringLiteral("del"),
        QStringLiteral("dir"),
        QStringLiteral("echo"),
        QStringLiteral("endlocal"),
        QStringLiteral("erase"),
        QStringLiteral("exit"),
        QStringLiteral("for"),
        QStringLiteral("ftype"),
        QStringLiteral("goto"),
        QStringLiteral("if"),
        QStringLiteral("md"),
        QStringLiteral("mkdir"),
        QStringLiteral("mklink"),
        QStringLiteral("move"),
        QStringLiteral("path"),
        QStringLiteral("pause"),
        QStringLiteral("popd"),
        QStringLiteral("prompt"),
        QStringLiteral("pushd"),
        QStringLiteral("rd"),
        QStringLiteral("rem"),
        QStringLiteral("ren"),
        QStringLiteral("rename"),
        QStringLiteral("rmdir"),
        QStringLiteral("set"),
        QStringLiteral("setlocal"),
        QStringLiteral("shift"),
        QStringLiteral("start"),
        QStringLiteral("time"),
        QStringLiteral("title"),
        QStringLiteral("type"),
        QStringLiteral("ver"),
        QStringLiteral("verify"),
        QStringLiteral("vol"),
    };
    return builtins.contains(program, Qt::CaseInsensitive);
}

bool requiresWindowsCommandInterpreter(const QString &program)
{
    return isWindowsCommandBuiltin(program)
        || program.endsWith(QStringLiteral(".bat"), Qt::CaseInsensitive)
        || program.endsWith(QStringLiteral(".cmd"), Qt::CaseInsensitive)
        || QStandardPaths::findExecutable(program).isEmpty();
}

QString escapeWindowsCommandToken(const QString &token)
{
    if (token.isEmpty())
    {
        return QStringLiteral("\"\"");
    }

    static const QString specialCharacters = QStringLiteral(" \t&|<>()^%!\"");
    QString escaped;
    escaped.reserve(token.size() * 2);
    for (const QChar character : token)
    {
        if (character == QLatin1Char('\r') || character == QLatin1Char('\n'))
        {
            // A literal line break would begin another command in cmd.exe.
            escaped.append(QLatin1Char('^'));
            escaped.append(QLatin1Char(' '));
            continue;
        }
        if (specialCharacters.contains(character))
        {
            escaped.append(QLatin1Char('^'));
        }
        escaped.append(character);
    }
    return escaped;
}

QString windowsCommandLine(
    const QString &program,
    const QStringList &arguments)
{
    QStringList tokens;
    tokens.reserve(arguments.size() + 1);
    tokens.append(escapeWindowsCommandToken(program));
    for (const QString &argument : arguments)
    {
        tokens.append(escapeWindowsCommandToken(argument));
    }
    return tokens.join(QLatin1Char(' '));
}

QString windowsCommandInterpreter()
{
    const QString configuredInterpreter =
        QProcessEnvironment::systemEnvironment().value(QStringLiteral("ComSpec"));
    if (!configuredInterpreter.isEmpty())
    {
        return configuredInterpreter;
    }
    return QStandardPaths::findExecutable(QStringLiteral("cmd.exe"));
}
#endif
}

SPluginTaskManager *SPluginTaskManager::m_instance = nullptr;

SPluginTask::SPluginTask(
    const QString &pluginName,
    const QString &program,
    const QStringList &arguments,
    const QString &commandLine,
    QObject *parent)
    : QObject(parent)
    , m_pluginName(pluginName)
    , m_program(program)
    , m_arguments(arguments)
    , m_commandLine(commandLine)
    , m_process(new QProcess(this))
{
    QObject::connect(m_process, &QProcess::started, this, [this] {
        m_startedAt = QDateTime::currentDateTime();
        setState(State::Running);
        emit started(this);
    });
    QObject::connect(m_process, &QProcess::errorOccurred, this, [this] (QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || m_state != State::Starting)
        {
            return;
        }
        setState(State::Failed);
        emit failed(this, m_process->errorString());
    });
    QObject::connect(
        m_process,
        qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this,
        [this] (int exitCode, QProcess::ExitStatus exitStatus) {
            if (m_forceStopRequested)
            {
                setState(State::Terminated);
            }
            else if (exitStatus == QProcess::NormalExit && exitCode == 0)
            {
                setState(State::Finished);
            }
            else
            {
                setState(State::Failed);
            }
            emit finished(this, exitCode, exitStatus);
        });
}

QString SPluginTask::pluginName() const
{
    return m_pluginName;
}

QString SPluginTask::commandLine() const
{
    return m_commandLine;
}

QDateTime SPluginTask::startedAt() const
{
    return m_startedAt;
}

qint64 SPluginTask::processId() const
{
    return m_process->processId();
}

SPluginTask::State SPluginTask::state() const
{
    return m_state;
}

void SPluginTask::forceStop()
{
    if (m_state != State::Starting && m_state != State::Running)
    {
        return;
    }
    m_forceStopRequested = true;
    setState(State::Stopping);
    if (m_process->state() == QProcess::NotRunning)
    {
        setState(State::Terminated);
        emit finished(this, -1, QProcess::CrashExit);
        return;
    }
    m_process->kill();
}

void SPluginTask::start()
{
    if (m_state != State::Starting)
    {
        return;
    }
#ifdef Q_OS_WIN
    if (requiresWindowsCommandInterpreter(m_program))
    {
        m_process->setProgram(windowsCommandInterpreter());
        m_process->setNativeArguments(
            QStringLiteral("/d /v:off /s /c ")
            + windowsCommandLine(m_program, m_arguments));
    }
    else
#endif
    {
        m_process->setProgram(m_program);
        m_process->setArguments(m_arguments);
    }
    m_process->setStandardOutputFile(QProcess::nullDevice());
    m_process->setStandardErrorFile(QProcess::nullDevice());
    m_process->start();
}

void SPluginTask::setState(State state)
{
    if (m_state == state)
    {
        return;
    }
    m_state = state;
    emit stateChanged(this);
}

SPluginTaskManager *SPluginTaskManager::instance()
{
    if (!m_instance)
    {
        m_instance = new SPluginTaskManager(QCoreApplication::instance());
        QObject::connect(m_instance, &QObject::destroyed, [] {
            m_instance = nullptr;
        });
    }
    return m_instance;
}

SPluginTask *SPluginTaskManager::startTask(
    const QString &pluginName,
    const QString &program,
    const QStringList &arguments,
    const QString &commandLine)
{
    if (program.trimmed().isEmpty())
    {
        return nullptr;
    }

    SPluginTask *task = new SPluginTask(
        pluginName,
        program,
        arguments,
        commandLine,
        this);
    m_tasks.push_back(task);

    QObject::connect(task, &SPluginTask::failed, this, [this] (SPluginTask *failedTask) {
        removeTask(failedTask);
    });
    QObject::connect(task, &SPluginTask::finished, this, [this] (SPluginTask *finishedTask) {
        removeTask(finishedTask);
    });

    emit taskAdded(task);
    QTimer::singleShot(0, task, &SPluginTask::start);
    return task;
}

QVector<SPluginTask *> SPluginTaskManager::activeTasks() const
{
    return m_tasks;
}

SPluginTaskManager::SPluginTaskManager(QObject *parent)
    : QObject(parent)
{
}

void SPluginTaskManager::removeTask(SPluginTask *task)
{
    if (!m_tasks.removeOne(task))
    {
        return;
    }
    emit taskRemoved(task);
    task->deleteLater();
}
