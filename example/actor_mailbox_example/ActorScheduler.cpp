#include "ActorScheduler.h"

#include <base_log.h>

ActorScheduler::ActorScheduler(size_t workerCount)
    : workers_(workerCount)
{

}

ActorScheduler::~ActorScheduler()
{
    shutdown();
}

void ActorScheduler::start()
{
    LOGDF("[ActorScheduler] Starting {} worker threads.", workers_.size());

    for (size_t i = 0; i < workers_.size(); ++i)
    {
        workers_[i] = std::thread(&ActorScheduler::workerLoop, this, i);
    }
}

void ActorScheduler::shutdown()
{
    shutdown_ = true;
    workAvailable_.notify_all();

    for (std::thread &worker : workers_)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void ActorScheduler::registerActor(const QString& id, std::shared_ptr<IActor> actor)
{
    registry_.registerActor(id, actor);
}

void ActorScheduler::enqueueActorWork(IActor* actor)
{
    {
        std::lock_guard<std::mutex> lock(runQueueMu_);
        runQueue_.push_back(actor);
    }
    workAvailable_.notify_one();
}

void ActorScheduler::routeMessage(const QString& targetActorId, Message&& msg)
{
    auto actor = registry_.getActor(targetActorId);

    if (actor)
    {
        actor->enqueueMessage(std::move(msg));
    }
    else
    {
        LOGWF("[ActorScheduler] Actor with ID {} not found for routing message.", targetActorId.toStdString());
    }
}

ActorScheduler::Metrics ActorScheduler::getMetrics() const
{
    Metrics metrics;
    // Implement metrics collection logic here if needed
    return metrics;
}

void ActorScheduler::workerLoop(size_t workerIdx)
{
    while (!shutdown_)
    {
        IActor* actor = nullptr;

        {
            std::unique_lock<std::mutex> lock(runQueueMu_);
            LOGDF("[ActorScheduler][Worker {}] runQueue_.size={}", workerIdx, runQueue_.size());
            workAvailable_.wait(lock, [this] { return shutdown_ || !runQueue_.empty(); });

            if (shutdown_)
            {
                break;
            }

            actor = runQueue_.front();
            runQueue_.pop_front();
        }

        if (actor)
        {
            actor->markNotEnqueued();
            std::vector<Message> messages;

            if (actor->getMailbox().dequeueBatch(messages, batchSize_) > 0)
            {
                LOGDF("[ActorScheduler][Worker {}] Processing {} messages for actor {}.", workerIdx, messages.size(),
                      actor->getId().toStdString());

                for (Message &msg : messages)
                {
                    actor->onMessage(std::move(msg));
                }
            }

            if (actor->hasWork())
            {
                if (actor->enqueuedIfNot())
                {
                    enqueueActorWork(actor);
                }
            }
        }
        else
        {
            LOGWF("[ActorScheduler][Worker {}] No actor to process.", workerIdx);
        }
    }
}