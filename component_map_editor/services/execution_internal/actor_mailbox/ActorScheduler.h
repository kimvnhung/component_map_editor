#ifndef CME_ACTORSCHEDULER_H
#define CME_ACTORSCHEDULER_H

#include <QHash>
#include <QString>
#include <QStringList>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <graph.pb.h>
#include <mutex>
#include <thread>
#include <vector>

#include "Actor.h"
#include "ActorRegistry.h"

#define CME_ACTOR_SCHEDULER_DEFAULT_WORKERS 4
#define CME_ACTOR_SCHEDULER_DEFAULT_BATCH_SIZE 32

namespace cme::actor
{

// Tracks, per target component, how many incoming edges are still pending and the
// per-connection payloads received so far. A target actor only gets a Message delivered
// to its mailbox once pendingInDegree reaches 0 - this is the join-gate fix identified in
// the actor/mailbox ADR (the earlier PoC fired downstream actors on the first predecessor).
struct JoinGateState
{
    int pendingInDegree = 0;
    QHash<QString, QVariantMap> receivedTokens; // connectionId -> payload
};

class ActorScheduler
{
public:
    enum class ExecutionMode { SEQUENTIAL, PARALLEL };

    explicit ActorScheduler(ActorFinishedCallback onStep,
                            size_t workerCount = CME_ACTOR_SCHEDULER_DEFAULT_WORKERS);
    ~ActorScheduler();

    void registerActor(const QString &id, std::shared_ptr<IActor> actor);
    void setRoutingTable(QHash<QString, QList<cme::ConnectionData>> outgoingBySource);
    void setInitialJoinGates(const QHash<QString, int> &pendingInDegreeByComponentId);

    void start();
    void shutdown();

    void setExecutionMode(ExecutionMode mode);
    void setBatchSize(size_t size) { m_batchSize = size; }

    // Delivers the graph input snapshot to every entry component (in-degree 0).
    void seedEntryComponents(const QStringList &entryComponentIds, const QVariantMap &graphInputSnapshot);

    bool hasPendingWork() const;

    // Blocks until the run-queue is empty and no message is mid-flight, or timeout elapses.
    // Returns false on timeout (e.g. a join-gate never completes) - callers/tests should
    // treat that as a hard failure, not silently ignore it.
    bool runUntilIdle(std::chrono::milliseconds timeout = std::chrono::seconds(5));

    ActorRegistry &registry() { return m_registry; }

    // Public so GraphSnapshotActorAdapter can bind each ComponentActor's onFinished callback
    // to this scheduler; not meant to be called directly by other callers.
    void handleActorFinished(const ExecutionContext &ctx, const ExecuteResult &result);

private:
    void workerLoop(size_t workerIdx);
    void deliverMessage(const QString &targetId, Message &&msg);

    ActorRegistry m_registry;
    QHash<QString, QList<cme::ConnectionData>> m_outgoingBySource;

    mutable std::mutex m_joinGateMutex;
    QHash<QString, JoinGateState> m_joinGates;

    mutable std::mutex m_runQueueMutex;
    std::condition_variable m_workAvailable;
    std::deque<std::shared_ptr<IActor>> m_runQueue;
    size_t m_inFlight = 0;

    std::vector<std::thread> m_workers;
    std::atomic_bool m_shutdown{false};
    std::atomic<ExecutionMode> m_mode{ExecutionMode::PARALLEL};
    size_t m_batchSize = CME_ACTOR_SCHEDULER_DEFAULT_BATCH_SIZE;

    ActorFinishedCallback m_onStep;
};

} // namespace cme::actor

#endif // CME_ACTORSCHEDULER_H
