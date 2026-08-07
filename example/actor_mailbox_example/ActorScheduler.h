#ifndef ACTORSCHEDULER_H
#define ACTORSCHEDULER_H

#include <thread>
#include <mutex>
#include <condition_variable>

#include "ActorRegistry.h"
#include "Actor.h"

#define DEFAULT_MAX_THREADS 4

class ActorScheduler
{
public:
    explicit ActorScheduler(size_t workerCount = DEFAULT_MAX_THREADS);
    ~ActorScheduler();

    // Start worker threads
    void start();

    // Graceful shutdown: drain queues, stop workers, join threads
    void shutdown();

    // Register an actor with the scheduler
    void registerActor(const std::string& id, std::shared_ptr<IActor> actor);

    // Enqueue actor for processing (called by ComponentActor when message arrives)
    void enqueueActorWork(IActor* actor);
    void routeMessage(const std::string& targetActorId, Message&& msg);

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

private:
    void workerLoop(size_t workerIdx);

    ActorRegistry registry_;
    std::deque<IActor *> runQueue_; // or lock-free queue for high-throughput
    mutable std::mutex runQueueMu_;
    std::condition_variable workAvailable_;

    std::vector<std::thread> workers_;
    std::atomic_bool shutdown_{false};
    size_t batchSize_ = 32;  // configurable

};

#endif // ACTORSCHEDULER_H
