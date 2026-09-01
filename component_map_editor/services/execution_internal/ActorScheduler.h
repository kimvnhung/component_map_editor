#ifndef ACTORSCHEDULER_H
#define ACTORSCHEDULER_H

#include <QString>
#include <QMap>

#include <mutex>
#include <deque>
#include <thread>
#include <condition_variable>

#include "ExecutionContext.h"
#include "ActorRegistry.h"
#include "MailBox.h"

#define DEFAULT_MAX_THREADS 4

class Connection;
class IExecutionSematicsProvider;
class ActorScheduler;

struct ConnectionRoutingTable
{
    QMap<QString, QStringList> routes;

    QStringList lookup(const QString &sourceId) const
    {
        return routes.value(sourceId, QStringList());
    }

    static std::unique_ptr<ConnectionRoutingTable> buildFromGraphSnapshot(const std::vector<IExecutionSematicsProvider *>
            &components,
            const std::vector<Connection *> &connections);
};

class TokenRouter
{
public:
    struct PendingToken
    {
        QString sourceComponentId;
        QString targetComponentId;
        QVariantMap payload;           // Full output (not per-port split)
        std::chrono::steady_clock::time_point createAt;
    };

    // Direct: Store full output from provider
    void onTokenProduced(const QString& sourceId,
                         const QVariantMap& outputState, const ConnectionRoutingTable& routingTable);

    // Collect: Merge all tokens for component (by connection order)
    QVariantMap consumeTokensFor(const QString& componentId);
private:
    std::deque<PendingToken> tokenQueue_;
};


using ActorProcessingMessageFinishedEvent =
    std::function<void(const ExecutionContext& ctx, const ExecuteResult& result)>;
class ActorScheduler
{
public:
    enum class ExecutionMode { SEQUENTIAL, PARALLEL };
    explicit ActorScheduler(ActorProcessingMessageFinishedEvent onActorProcessMessageFinished = nullptr,
                            size_t workerCount = DEFAULT_MAX_THREADS);
    ~ActorScheduler();

    // Start worker threads
    void start(std::unique_ptr<ConnectionRoutingTable> routingTable);

    void setExecutionMode(ExecutionMode mode);

    // Graceful shutdown: drain queues, stop workers, join threads
    void shutdown();

    // Register an actor with the scheduler
    void registerActor(const QString& id, std::shared_ptr<IActor> actor);

    // Enqueue actor for processing (called by ComponentActor when message arrives)
    void enqueueActorWork(IActor* actor);
    void routeMessageFrom(const QString& fromComponentId);
    void routeMessageToActor(const QString& targetId, Message&& msg);
    void handleActorProcessFinished(const ExecutionContext& ctx, ExecuteResult& result);

    // Get actor registry
    ActorRegistry &getRegistry() { return registry_; }

    // Tuning / config
    void setBatchSize(size_t sz) { batchSize_ = sz; }
    size_t getBatchSize() const { return batchSize_; }

    // Metrics (optional, for observability)
    struct Metrics
    {
        uint64_t messagesProcessed = 0;
        uint64_t actorsScheduled = 0;
        double avgMailboxLength = 0.0;
    };
    Metrics getMetrics() const;

    TokenRouter *getTokenRouter() const;
    // ExecutionSnapshot globalSnapshot() const;  // optional: capture global state for debugging
private:
    void workerLoop(size_t workerIdx);

    ActorRegistry registry_;
    std::deque<IActor *> runQueue_; // or lock-free queue for high-throughput
    mutable std::mutex runQueueMu_;
    std::condition_variable workAvailable_;

    std::vector<std::thread> workers_;
    std::atomic_bool shutdown_{false};
    size_t batchSize_ = 32;  // configurable
    ExecutionMode executionMode_ = ExecutionMode::PARALLEL;
    ActorProcessingMessageFinishedEvent externalCallback_;
    std::unique_ptr<TokenRouter> tokenRouter_;
    std::unique_ptr<ConnectionRoutingTable> routingTable_;
};

#endif // ACTORSCHEDULER_H
