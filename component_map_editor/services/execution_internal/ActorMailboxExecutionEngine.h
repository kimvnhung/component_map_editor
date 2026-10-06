#ifndef ACTORMAILBOXEXECUTIONENGINE_H
#define ACTORMAILBOXEXECUTIONENGINE_H

#include "IExecutionEngine.h"
#include "actor_mailbox/ActorScheduler.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>

// IExecutionEngine adapter around the actor/mailbox core (ActorScheduler +
// GraphSnapshotActorAdapter). executeNext() pulls completed steps, in completion order, from a
// queue fed by the scheduler's worker threads - see ADR Plan 3 for why breakpoint-accurate
// peeking is intentionally NOT supported here (GraphExecutionSandbox falls back to
// SequentialExecutionEngine whenever breakpoints are set).
class ActorMailboxExecutionEngine : public IExecutionEngine
{
public:
    ActorMailboxExecutionEngine();
    ~ActorMailboxExecutionEngine() override;

    bool prepare(GraphModel *graph,
                 const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                 const QVariantMap &inputSnapshot,
                 QString *error) override;

    bool hasReadyWork() const override;
    QString peekNextReadyComponentId() const override;
    QStringList readyComponentIds() const override;
    int executedCount() const override;
    int totalComponentCount() const override;
    const cme::GraphSnapshot &graphSnapshot() const override;
    Telemetry telemetry() const override;

    StepOutcome executeNext(const QVariantMap &legacyGlobalState,
                            ExecutionContext *outCtx,
                            ExecuteResult *outResult,
                            QString *error) override;

    void reset() override;

private:
    void onActorStep(const ExecutionContext &ctx, const ExecuteResult &result);

    std::unique_ptr<cme::actor::ActorScheduler> m_scheduler;
    cme::GraphSnapshot m_graphSnapshot;
    int m_totalComponentCount = 0;
    int m_executedCount = 0;

    mutable std::mutex m_completedMutex;
    std::condition_variable m_completedAvailable;
    std::deque<std::pair<ExecutionContext, ExecuteResult>> m_completedSteps;

    qint64 m_payloadBytesRead = 0;
    qint64 m_payloadBytesWritten = 0;
    qint64 m_maxPayloadBytes = 0;
    int m_tokenReadCount = 0;
    int m_tokenWriteCount = 0;
};

#endif // ACTORMAILBOXEXECUTIONENGINE_H
