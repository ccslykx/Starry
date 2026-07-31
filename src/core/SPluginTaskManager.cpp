#include <QCoreApplication>
#include <QTimer>

#include "SPluginTaskManager.h"

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
    m_process->setProgram(m_program);
    m_process->setArguments(m_arguments);
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
