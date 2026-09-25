#ifndef ACTORSCHEDULER_H
#define ACTORSCHEDULER_H

#include <QThreadPool>

#include <functional>
#include "Message.h"

struct ActorTaskResult
{
    enum class Status
    {
        Success,
        Failure,
        Cancelled
    } status;

    Message outputMessage;
    std::string error;
};

using TaskFinishCallback = std::function<void(const ActorTaskResult &)>;
struct ActorTask
{
    std::function<ActorTaskResult()> taskFunction;
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
