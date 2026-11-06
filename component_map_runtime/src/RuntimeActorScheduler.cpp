#include "component_map_runtime/RuntimeActorScheduler.h"

#include <QElapsedTimer>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QRunnable>

namespace cme::runtime
{

class RuntimeActorScheduler::TaskRunner final : public QRunnable
{
public:
    TaskRunner(RuntimeActorScheduler *scheduler, QString actorId)
        : m_scheduler(scheduler)
        , m_actorId(std::move(actorId))
    {
        setAutoDelete(true);
    }

    void run() override { m_scheduler->execute(m_actorId); }

private:
    RuntimeActorScheduler *m_scheduler;
    QString m_actorId;
};

RuntimeActorScheduler::RuntimeActorScheduler(int maxConcurrency, qsizetype mailboxCapacity)
    : m_mailboxCapacity(qMax<qsizetype>(1, mailboxCapacity))
{
    m_pool.setMaxThreadCount(qMax(1, maxConcurrency));
}

RuntimeActorScheduler::~RuntimeActorScheduler()
{
    stop();
}

void RuntimeActorScheduler::start()
{
    QMutexLocker locker(&m_mutex);
    m_running = true;
}

void RuntimeActorScheduler::stop()
{
    {
        QMutexLocker locker(&m_mutex);
        m_running = false;
        for (auto it = m_queues.begin(); it != m_queues.end(); ++it)
        {
            m_pending -= it->tasks.size();
            it->tasks.clear();
            it->scheduled = false;
        }
        m_pending = 0;
    }
    m_pool.waitForDone();
    QMutexLocker locker(&m_mutex);
    m_runningActors.clear();
}

bool RuntimeActorScheduler::submit(const QString &actorId, Task task)
{
    if (actorId.isEmpty() || !task)
        return false;

    QMutexLocker locker(&m_mutex);
    if (!m_running)
        return false;
    auto &queue = m_queues[actorId];
    if (queue.tasks.size() >= m_mailboxCapacity)
    {
        ++m_rejected;
        return false;
    }
    queue.tasks.enqueue(std::move(task));
    ++m_pending;
    scheduleNextLocked(actorId);
    return true;
}

void RuntimeActorScheduler::scheduleNextLocked(const QString &actorId)
{
    auto it = m_queues.find(actorId);
    if (it == m_queues.end() || it->scheduled || it->tasks.isEmpty()
        || m_runningActors.contains(actorId))
        return;
    it->scheduled = true;
    m_runningActors.insert(actorId);
    m_pool.start(new TaskRunner(this, actorId));
}

void RuntimeActorScheduler::execute(const QString &actorId)
{
    Task task;
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_queues.find(actorId);
        if (it == m_queues.end() || it->tasks.isEmpty())
        {
            if (it != m_queues.end()) it->scheduled = false;
            m_runningActors.remove(actorId);
            return;
        }
        task = it->tasks.dequeue();
        --m_pending;
    }

    task();

    {
        QMutexLocker locker(&m_mutex);
        auto it = m_queues.find(actorId);
        m_runningActors.remove(actorId);
        if (it != m_queues.end())
        {
            it->scheduled = false;
            scheduleNextLocked(actorId);
        }
    }
}

bool RuntimeActorScheduler::waitForIdle(int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    for (;;)
    {
        {
            QMutexLocker locker(&m_mutex);
            if (m_pending == 0 && m_runningActors.isEmpty())
                return true;
        }
        if (timeoutMs >= 0 && timer.elapsed() >= timeoutMs)
            return false;
        QThread::msleep(1);
    }
}

bool RuntimeActorScheduler::isRunning() const
{
    QMutexLocker locker(&m_mutex);
    return m_running;
}

qsizetype RuntimeActorScheduler::mailboxDepth(const QString &actorId) const
{
    QMutexLocker locker(&m_mutex);
    return m_queues.value(actorId).tasks.size();
}

qsizetype RuntimeActorScheduler::rejectedCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_rejected;
}

int RuntimeActorScheduler::maxConcurrency() const
{
    return m_pool.maxThreadCount();
}

} // namespace cme::runtime
