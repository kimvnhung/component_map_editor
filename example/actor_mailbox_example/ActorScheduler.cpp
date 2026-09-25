#include "ActorScheduler.h"

#include <base_log.h>

#define DEFAULT_MAX_THREADS 4

ActorScheduler::ActorScheduler()
    : m_threadPool(new QThreadPool)
    , m_maxTheads(DEFAULT_MAX_THREADS)
{
    m_threadPool->setMaxThreadCount(m_maxTheads);
}

void ActorScheduler::setMaxThreads(int maxThreads)
{
    m_maxTheads = maxThreads;
    m_threadPool->setMaxThreadCount(maxThreads);
}

int ActorScheduler::maxThreads() const
{
    return m_maxTheads;
}

void ActorScheduler::scheduleTask(ActorTask task)
{
    m_threadPool->start([task]()
    {
        try
        {
            auto result = task.taskFunction();

            if (task.finishCallback)
            {
                task.finishCallback(result);
            }
        }
        catch (const std::exception &e)
        {
            LOGW("[ActorScheduler][ERROR] Exception in scheduled task: {}", e.what());
        }
        catch (...)
        {
            LOGW("[ActorScheduler][ERROR] Unknown exception in scheduled task.");
        }
    });
}
