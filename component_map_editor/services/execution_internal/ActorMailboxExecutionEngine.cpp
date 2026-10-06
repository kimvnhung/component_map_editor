#include "ActorMailboxExecutionEngine.h"

#include "actor_mailbox/GraphSnapshotActorAdapter.h"
#include "services/execution_internal/ProviderResultValidation.h"
#include "services/execution_internal/SequentialExecutionEngine.h"

using cme::actor::ActorScheduler;
using cme::actor::GraphSnapshotActorAdapter;

ActorMailboxExecutionEngine::ActorMailboxExecutionEngine() = default;

ActorMailboxExecutionEngine::~ActorMailboxExecutionEngine()
{
    if (m_scheduler)
    {
        m_scheduler->shutdown();
    }
}

bool ActorMailboxExecutionEngine::prepare(GraphModel *graph,
        const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
        const QVariantMap &inputSnapshot,
        QString *error)
{
    reset();

    // Reuse SequentialExecutionEngine purely for its already-tested GraphModel -> GraphSnapshot
    // capture; the actor engine intentionally does not duplicate that capture logic (ADR Plan 2).
    SequentialExecutionEngine snapshotCapture;

    if (!snapshotCapture.prepare(graph, providersByType, inputSnapshot, error))
    {
        return false;
    }

    m_graphSnapshot = snapshotCapture.graphSnapshot();
    m_totalComponentCount = snapshotCapture.totalComponentCount();

    m_scheduler = std::make_unique<ActorScheduler>([this](const ExecutionContext & ctx, const ExecuteResult & result)
    {
        onActorStep(ctx, result);
    });

    QStringList entryIds;

    if (!GraphSnapshotActorAdapter::build(m_graphSnapshot, providersByType, m_scheduler.get(), &entryIds, error))
    {
        m_scheduler.reset();
        return false;
    }

    m_scheduler->start();
    m_scheduler->seedEntryComponents(entryIds, inputSnapshot);
    return true;
}

bool ActorMailboxExecutionEngine::hasReadyWork() const
{
    if (!m_scheduler)
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_completedMutex);

        if (!m_completedSteps.empty())
        {
            return true;
        }
    }

    return m_scheduler->hasPendingWork();
}

QString ActorMailboxExecutionEngine::peekNextReadyComponentId() const
{
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);

        if (!m_completedSteps.empty())
        {
            return m_completedSteps.front().first.componentId;
        }
    }

    return m_scheduler ? m_scheduler->peekFrontActorId() : QString();
}

QStringList ActorMailboxExecutionEngine::readyComponentIds() const
{
    QStringList ids;

    {
        std::lock_guard<std::mutex> lock(m_completedMutex);

        for (const auto &step : m_completedSteps)
        {
            ids.append(step.first.componentId);
        }
    }

    if (m_scheduler)
    {
        ids.append(m_scheduler->readyActorIds());
    }

    return ids;
}

int ActorMailboxExecutionEngine::executedCount() const
{
    return m_executedCount;
}

int ActorMailboxExecutionEngine::totalComponentCount() const
{
    return m_totalComponentCount;
}

const cme::GraphSnapshot &ActorMailboxExecutionEngine::graphSnapshot() const
{
    return m_graphSnapshot;
}

IExecutionEngine::Telemetry ActorMailboxExecutionEngine::telemetry() const
{
    // TODO : Need implemement the values in Telemetry later, all are zero now.
    return Telemetry
    {
        m_payloadBytesRead,
        m_payloadBytesWritten,
        m_maxPayloadBytes,
        m_tokenReadCount,
        m_tokenWriteCount
    };
}

void ActorMailboxExecutionEngine::onActorStep(const ExecutionContext &ctx, const ExecuteResult &result)
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_completedSteps.emplace_back(ctx, result);
    m_completedAvailable.notify_one();
}

IExecutionEngine::StepOutcome ActorMailboxExecutionEngine::executeNext(const QVariantMap &legacyGlobalState,
        ExecutionContext *outCtx,
        ExecuteResult *outResult,
        QString *error)
{
    Q_UNUSED(legacyGlobalState); // actor engine only runs when token routing is enabled - see
    // GraphExecutionSandbox's fallback to SequentialExecutionEngine otherwise.

    if (!m_scheduler)
    {
        return StepOutcome::NoWork;
    }

    std::pair<ExecutionContext, ExecuteResult> step;

    {
        std::unique_lock<std::mutex> lock(m_completedMutex);

        while (m_completedSteps.empty())
        {
            if (!m_scheduler->hasPendingWork())
            {
                return StepOutcome::NoWork;
            }

            // hasPendingWork() needs the scheduler's own mutex, so poll in short slices rather
            // than waiting on a single predicate across two different mutexes.
            m_completedAvailable.wait_for(lock, std::chrono::milliseconds(200));
        }

        step = std::move(m_completedSteps.front());
        m_completedSteps.pop_front();
    }

    const ExecutionContext &ctx = step.first;
    ExecuteResult result = std::move(step.second);

    if (outCtx)
    {
        *outCtx = ctx;
    }

    if (result.status == ExecuteResult::Status::Error)
    {
        if (outResult)
        {
            *outResult = result;
        }

        if (error)
        {
            *error = result.errorMessage;
        }

        return StepOutcome::ProviderError;
    }

    if (result.status == ExecuteResult::Status::Rejected)
    {
        if (outResult)
        {
            *outResult = result;
        }

        return StepOutcome::Rejected;
    }

    QString validateMessage;
    const bool valid = cme::execution::validateExecutionResult(result, validateMessage);

    if (outResult)
    {
        *outResult = result;
    }

    if (!valid)
    {
        if (error)
        {
            *error = validateMessage;
        }

        return StepOutcome::ValidationError;
    }

    ++m_executedCount;
    return StepOutcome::Committed;
}

void ActorMailboxExecutionEngine::reset()
{
    if (m_scheduler)
    {
        m_scheduler->shutdown();
        m_scheduler.reset();
    }

    m_graphSnapshot.Clear();
    m_totalComponentCount = 0;
    m_executedCount = 0;

    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_completedSteps.clear();
}
