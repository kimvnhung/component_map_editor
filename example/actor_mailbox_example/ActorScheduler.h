#ifndef ACTORSCHEDULER_H
#define ACTORSCHEDULER_H

#include <QThreadPool>

#include <functional>

struct ActorTaskResult
{
    enum class Status
    {
        Success,
        Failure,
        Cancelled
    } status;

    std::string message;
};

using TaskFinishCallback = std::function<void(const ActorTaskResult &)>;
struct ActorTask
{
    std::function<void()> taskFunction;
    TaskFinishCallback finishCallback;
};

class ActorScheduler
{
public:
    ActorScheduler();
    void setMaxThreads(int maxThreads);
    int maxThreads() const;
    void scheduleTask(ActorTask task);
private:
    QThreadPool *m_threadPool{nullptr};
    int m_maxTheads{1};
};

#endif // ACTORSCHEDULER_H
