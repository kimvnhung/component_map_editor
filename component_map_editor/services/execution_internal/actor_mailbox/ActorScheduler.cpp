#include "ActorScheduler.h"

#include <base_log.h>

namespace cme::actor
{

ActorScheduler::ActorScheduler(ActorFinishedCallback onStep, size_t workerCount)
    : m_workers(workerCount)
    , m_onStep(std::move(onStep))
{
}

ActorScheduler::~ActorScheduler()
{
    shutdown();
}

void ActorScheduler::registerActor(const QString &id, std::shared_ptr<IActor> actor)
{
    m_registry.registerActor(id, std::move(actor));
}

void ActorScheduler::setRoutingTable(QHash<QString, QList<cme::ConnectionData>> outgoingBySource)
{
    m_outgoingBySource = std::move(outgoingBySource);
}

void ActorScheduler::setInitialJoinGates(const QHash<QString, int> &pendingInDegreeByComponentId)
{
    std::lock_guard<std::mutex> lock(m_joinGateMutex);
    m_joinGates.clear();

    for (auto it = pendingInDegreeByComponentId.constBegin(); it != pendingInDegreeByComponentId.constEnd(); ++it)
    {
        m_joinGates[it.key()].pendingInDegree = it.value();
    }
}

void ActorScheduler::start()
{
    for (size_t i = 0; i < m_workers.size(); ++i)
    {
        m_workers[i] = std::thread(&ActorScheduler::workerLoop, this, i);
    }
}

void ActorScheduler::shutdown()
{
    // Mutate the predicate under the same mutex used by workerLoop's wait() - otherwise a
    // worker that just read m_shutdown==false can miss this notify and sleep forever.
    {
        std::lock_guard<std::mutex> lock(m_runQueueMutex);
        m_shutdown = true;
    }
    m_workAvailable.notify_all();

    for (std::thread &worker : m_workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void ActorScheduler::setExecutionMode(ExecutionMode mode)
{
    {
        std::lock_guard<std::mutex> lock(m_runQueueMutex);
        m_mode = mode;
    }
    m_workAvailable.notify_all();
}

void ActorScheduler::seedEntryComponents(const QStringList &entryComponentIds, const QVariantMap &graphInputSnapshot)
{
    for (const QString &id : entryComponentIds)
    {
        QHash<QString, QVariantMap> tokens;

        if (!graphInputSnapshot.isEmpty())
        {
            tokens.insert(QStringLiteral("__graph_input__"), graphInputSnapshot);
        }

        deliverMessage(id, Message{ id, tokens });
    }
}

bool ActorScheduler::hasPendingWork() const
{
    std::lock_guard<std::mutex> lock(m_runQueueMutex);
    return m_inFlight != 0 || !m_runQueue.empty();
}

QString ActorScheduler::peekFrontActorId() const
{
    std::lock_guard<std::mutex> lock(m_runQueueMutex);
    return m_runQueue.empty() ? QString() : m_runQueue.front()->id();
}

QStringList ActorScheduler::readyActorIds() const
{
    std::lock_guard<std::mutex> lock(m_runQueueMutex);
    QStringList ids;
    ids.reserve(static_cast<int>(m_runQueue.size()));

    for (const std::shared_ptr<IActor> &actor : m_runQueue)
    {
        ids.append(actor->id());
    }

    return ids;
}

bool ActorScheduler::runUntilIdle(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_runQueueMutex);
    return m_workAvailable.wait_for(lock, timeout, [this]
    {
        return m_shutdown.load() || (m_inFlight == 0 && m_runQueue.empty());
    });
}

void ActorScheduler::deliverMessage(const QString &targetId, Message &&msg)
{
    auto actorPtr = m_registry.actor(targetId);

    if (!actorPtr)
    {
        LOGWF("[ActorScheduler] No actor found for target ID {}. Message dropped.", targetId.toStdString());
        return;
    }

    if (!actorPtr->enqueueMessage(std::move(msg)))
    {
        LOGWF("[ActorScheduler] Mailbox rejected message for actor {} (backpressure).", targetId.toStdString());
        return;
    }

    bool shouldEnqueue = false;
    {
        std::lock_guard<std::mutex> lock(m_runQueueMutex);
        ++m_inFlight;
        shouldEnqueue = actorPtr->enqueuedIfNot();

        if (shouldEnqueue)
        {
            m_runQueue.push_back(actorPtr);
        }
    }

    if (shouldEnqueue)
    {
        m_workAvailable.notify_one();
    }
}

void ActorScheduler::handleActorFinished(const ExecutionContext &ctx, const ExecuteResult &result)
{
    if (m_onStep)
    {
        m_onStep(ctx, result);
    }

    // Only propagate tokens downstream for a fully successful step - mirrors
    // SequentialExecutionEngine, which only calls routeOutgoingTokens()/commitSchedulingState()
    // after invokeProvider()+validateExecutionResult() both succeed.
    if (result.status != ExecuteResult::Status::Ok)
    {
        return;
    }

    const QList<cme::ConnectionData> outgoing = m_outgoingBySource.value(ctx.componentId);

    for (const cme::ConnectionData &edge : outgoing)
    {
        const QString targetId = QString::fromStdString(edge.target_id());
        const QString connectionId = QString::fromStdString(edge.id());

        bool ready = false;
        QHash<QString, QVariantMap> tokensForDelivery;

        {
            std::lock_guard<std::mutex> lock(m_joinGateMutex);
            JoinGateState &gate = m_joinGates[targetId];
            gate.receivedTokens.insert(connectionId, result.output);
            --gate.pendingInDegree;

            if (gate.pendingInDegree <= 0)
            {
                ready = true;
                tokensForDelivery = gate.receivedTokens;
                gate.receivedTokens.clear();
            }
        }

        if (ready)
        {
            deliverMessage(targetId, Message{ targetId, tokensForDelivery });
        }
    }
}

void ActorScheduler::workerLoop(size_t workerIdx)
{
    while (!m_shutdown)
    {
        std::shared_ptr<IActor> actor;

        {
            std::unique_lock<std::mutex> lock(m_runQueueMutex);
            m_workAvailable.wait(lock, [this, workerIdx]
            {
                if (m_shutdown)
                {
                    return true;
                }

                if (m_mode.load() == ExecutionMode::SEQUENTIAL)
                {
                    return !m_runQueue.empty() && workerIdx == 0;
                }

                return !m_runQueue.empty();
            });

            if (m_shutdown)
            {
                break;
            }

            actor = m_runQueue.front();
            m_runQueue.pop_front();
        }

        actor->markNotEnqueued();

        std::vector<Message> messages;
        const size_t dequeued = actor->mailbox().dequeueBatch(messages, m_batchSize);

        for (Message &msg : messages)
        {
            actor->onMessage(std::move(msg));
        }

        if (actor->hasWork() && actor->enqueuedIfNot())
        {
            std::lock_guard<std::mutex> lock(m_runQueueMutex);
            m_runQueue.push_back(actor);
            m_workAvailable.notify_one();
        }

        {
            std::lock_guard<std::mutex> lock(m_runQueueMutex);
            m_inFlight -= dequeued;

            if (m_inFlight == 0 && m_runQueue.empty())
            {
                m_workAvailable.notify_all();
            }
        }
    }
}

} // namespace cme::actor
