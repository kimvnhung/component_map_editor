#include "ActorScheduler.h"

#include <base_log.h>


ActorScheduler::ActorScheduler(ActorProcessingMessageFinishedEvent callback, size_t workerCount)
    : workers_(workerCount)
    , tokenRouter_(std::make_unique<TokenRouter>())
    , externalCallback_(callback)
{

}

ActorScheduler::~ActorScheduler()
{
    shutdown();
}

void ActorScheduler::start(std::unique_ptr<ConnectionRoutingTable> routingTable)
{
    LOGDF("[ActorScheduler] Starting {} worker threads.", workers_.size());
    routingTable_ = std::move(routingTable);

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

void ActorScheduler::setExecutionMode(ExecutionMode mode)
{
    executionMode_ = mode;
    workAvailable_.notify_all(); // Wake up workers to re-evaluate execution mode
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

void ActorScheduler::routeMessageToActor(const QString& targetId, Message&& msg)
{
    auto targetActor = registry_.getActor(targetId);

    if (targetActor)
    {
        targetActor->enqueueMessage(std::move(msg));
    }
    else
    {
        LOGWF("[ActorScheduler] No actor found for target ID {}. Message dropped.", targetId.toStdString());
    }
}

void ActorScheduler::routeMessageFrom(const QString& fromComponentId)
{
    QString sourceId = fromComponentId;

    if (routingTable_)
    {
        QStringList targetIds = routingTable_->lookup(sourceId);

        if (!targetIds.isEmpty())
        {
            for (const QString& targetId : targetIds)
            {
                auto tokens = tokenRouter_ ? tokenRouter_->consumeTokensFor(targetId) : QVariantMap();
                routeMessageToActor(targetId, Message{0, sourceId, tokens});
            }
        }
        else
        {
            LOGWF("[ActorScheduler] No routing targets found for source ID {}. Message dropped.", sourceId.toStdString());
        }
    }
    else
    {
        LOGWF("[ActorScheduler] Routing table is not initialized. Message from {} dropped.", sourceId.toStdString());
    }
}

void ActorScheduler::handleActorProcessFinished(const ExecutionContext& ctx, ExecuteResult& result)
{
    if (tokenRouter_)
    {
        tokenRouter_->onTokenProduced(ctx.componentId, result.outputState,
                                      routingTable_ ? *routingTable_ : ConnectionRoutingTable());
    }

    if (externalCallback_)
    {
        // Call the callback to notify that the actor has finished processing
        // Merge the output state with the consumed tokens from the TokenRouter
        // result.outputState = tokenRouter_->consumeTokensFor(ctx.componentId);
        externalCallback_(ctx, result);
    }
}

ActorScheduler::Metrics ActorScheduler::getMetrics() const
{
    Metrics metrics;
    // Implement metrics collection logic here if needed
    return metrics;
}

TokenRouter *ActorScheduler::getTokenRouter() const
{
    return tokenRouter_.get();
}

ExecutionSnapshot ActorScheduler::globalSnapshot() const
{
    return globalSnapshot_;
}

void ActorScheduler::workerLoop(size_t workerIdx)
{
    while (!shutdown_)
    {
        IActor* actor = nullptr;

        {
            std::unique_lock<std::mutex> lock(runQueueMu_);
            LOGDF("[ActorScheduler][Worker {}] runQueue_.size={}", workerIdx, runQueue_.size());
            workAvailable_.wait(lock, [this, workerIdx]
            {
                if (shutdown_)
                {
                    return true;
                }

                if (executionMode_ == ExecutionMode::SEQUENTIAL)
                {
                    // In SEQUENTIAL mode, only the first worker (workerIdx == 0) should process actors
                    return !runQueue_.empty() && workerIdx == 0;
                }
                else // PARALLEL
                {
                    return !runQueue_.empty();
                }
            });

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