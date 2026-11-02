#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_ACTOR_SCHEDULER_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_ACTOR_SCHEDULER_H

#include <QHash>
#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QThreadPool>
#include <QString>
#include <functional>

namespace cme::runtime
{

class RuntimeActorScheduler
{
public:
    using Task = std::function<void()>;

    explicit RuntimeActorScheduler(int maxConcurrency = 1, qsizetype mailboxCapacity = 64);
    ~RuntimeActorScheduler();

    RuntimeActorScheduler(const RuntimeActorScheduler &) = delete;
    RuntimeActorScheduler &operator=(const RuntimeActorScheduler &) = delete;

    void start();
    void stop();
    bool submit(const QString &actorId, Task task);
    bool waitForIdle(int timeoutMs = -1);

    bool isRunning() const;
    qsizetype mailboxDepth(const QString &actorId) const;
    qsizetype rejectedCount() const;
    int maxConcurrency() const;

private:
    struct ActorQueue
    {
        QQueue<Task> tasks;
        bool scheduled = false;
    };

    class TaskRunner;

    void scheduleNextLocked(const QString &actorId);
    void execute(const QString &actorId);

    mutable QMutex m_mutex;
    QThreadPool m_pool;
    QHash<QString, ActorQueue> m_queues;
    QSet<QString> m_runningActors;
    qsizetype m_pending = 0;
    qsizetype m_rejected = 0;
    qsizetype m_mailboxCapacity;
    bool m_running = false;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_ACTOR_SCHEDULER_H
