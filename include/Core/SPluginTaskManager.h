#pragma once

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QVector>

class SPluginTaskManager;

class SPluginTask final : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        Starting,
        Running,
        Stopping,
        Finished,
        Failed,
        Terminated
    };
    Q_ENUM(State)

    QString pluginName() const;
    QString commandLine() const;
    QDateTime startedAt() const;
    qint64 processId() const;
    State state() const;

public slots:
    void forceStop();

signals:
    void stateChanged(SPluginTask *task);
    void started(SPluginTask *task);
    void failed(SPluginTask *task, const QString &reason);
    void finished(SPluginTask *task, int exitCode, QProcess::ExitStatus exitStatus);

private:
    friend class SPluginTaskManager;

    explicit SPluginTask(
        const QString &pluginName,
        const QString &program,
        const QStringList &arguments,
        const QString &commandLine,
        QObject *parent = nullptr);

    void start();
    void setState(State state);

    QString m_pluginName;
    QString m_program;
    QStringList m_arguments;
    QString m_commandLine;
    QDateTime m_startedAt;
    QProcess *m_process = nullptr;
    State m_state = State::Starting;
    bool m_forceStopRequested = false;
};

class SPluginTaskManager final : public QObject
{
    Q_OBJECT

public:
    static SPluginTaskManager *instance();

    SPluginTask *startTask(
        const QString &pluginName,
        const QString &program,
        const QStringList &arguments,
        const QString &commandLine);
    QVector<SPluginTask *> activeTasks() const;

signals:
    void taskAdded(SPluginTask *task);
    void taskRemoved(SPluginTask *task);

private:
    explicit SPluginTaskManager(QObject *parent = nullptr);
    void removeTask(SPluginTask *task);

    static SPluginTaskManager *m_instance;
    QVector<SPluginTask *> m_tasks;
};
