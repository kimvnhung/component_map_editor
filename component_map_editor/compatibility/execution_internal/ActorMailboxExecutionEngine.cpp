#include "ActorMailboxExecutionEngine.h"

#include "actor_mailbox/GraphSnapshotActorAdapter.h"
#include "compatibility/execution_internal/ProviderResultValidation.h"
#include "compatibility/execution_internal/SequentialExecutionEngine.h"

#include <algorithm>

#include "utils/GraphHelper.h"

namespace {

void recordTelemetry(const cme::GraphSnapshot &snapshot, const ExecutionContext &ctx,
                     const ExecuteResult &result, qint64 *bytesRead, qint64 *bytesWritten,
                     qint64 *maxBytes, int *tokenReads, int *tokenWrites)
{
    for (const QVariantMap &payload : ctx.incomingTokens)
    {
        const qint64 bytes = cme::helper::graph::estimatePayloadBytes(payload);
        *bytesRead += bytes;
        *maxBytes = qMax(*maxBytes, bytes);
        ++*tokenReads;
    }
    for (const cme::ConnectionData &edge : cme::helper::graph::getConnectionsBySourceId(snapshot, ctx.componentId))
    {
        const qint64 bytes = cme::helper::graph::estimatePayloadBytes(result.output);
        *bytesWritten += bytes;
        *maxBytes = qMax(*maxBytes, bytes);
        ++*tokenWrites;
    }
}

} // namespace

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
        const ExecutionRequest &request,
        QString *error)
{
    reset();

    // Reuse SequentialExecutionEngine purely for its already-tested GraphModel -> GraphSnapshot
    // capture; the actor engine intentionally does not duplicate that capture logic (ADR Plan 2).
    SequentialExecutionEngine snapshotCapture;

    if (!snapshotCapture.prepare(graph, providersByType, request, error))
    {
        return false;
    }

    m_graphSnapshot = snapshotCapture.graphSnapshot();
    m_totalComponentCount = snapshotCapture.totalComponentCount();
    m_resolvedStartComponentId = snapshotCapture.resolvedStartComponentId();
    m_reachableComponentIds = snapshotCapture.reachableComponentIds();
    m_ignoredComponentIds = snapshotCapture.ignoredComponentIds();
    m_activeConnectionCount = snapshotCapture.activeConnectionCount();

    m_scheduler = std::make_unique<ActorScheduler>([this](const ExecutionContext & ctx, const ExecuteResult & result)
    {
        onActorStep(ctx, result);
    });

    const QString requestedStartComponentId = request.requestedStartComponentId.trimmed();
    const QString resolvedStartComponentId = requestedStartComponentId.isEmpty()
            ? graph->startComponentId().trimmed()
            : requestedStartComponentId;

    if (!GraphSnapshotActorAdapter::build(m_graphSnapshot,
                                          providersByType,
                                          resolvedStartComponentId,
                                          m_scheduler.get(),
                                          error))
    {
        m_scheduler.reset();
        return false;
    }

    m_scheduler->start();
    QStringList entryComponentIds;
    for (const auto &component : m_graphSnapshot.components())
    {
        const QString componentId = QString::fromStdString(component.id());
        if (cme::helper::graph::getConnectionsByTargetId(m_graphSnapshot, componentId).isEmpty())
            entryComponentIds.append(componentId);
    }
    std::sort(entryComponentIds.begin(), entryComponentIds.end());
    m_scheduler->seedEntryComponents(entryComponentIds, request.inputSnapshot);
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

QString ActorMailboxExecutionEngine::resolvedStartComponentId() const
{
    return m_resolvedStartComponentId;
}

QStringList ActorMailboxExecutionEngine::reachableComponentIds() const
{
    return m_reachableComponentIds;
}

QStringList ActorMailboxExecutionEngine::ignoredComponentIds() const
{
    return m_ignoredComponentIds;
}

int ActorMailboxExecutionEngine::activeConnectionCount() const
{
    return m_activeConnectionCount;
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
    if (result.status == ExecuteResult::Status::Ok)
    {
        for (const cme::ConnectionData &edge : cme::helper::graph::getConnectionsBySourceId(m_graphSnapshot, ctx.componentId))
        {
            cme::helper::graph::setPayload(m_graphSnapshot, QString::fromStdString(edge.id()), result.output);
        }
    }

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

    recordTelemetry(m_graphSnapshot, ctx, result, &m_payloadBytesRead, &m_payloadBytesWritten,
                    &m_maxPayloadBytes, &m_tokenReadCount, &m_tokenWriteCount);
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
    m_resolvedStartComponentId.clear();
    m_reachableComponentIds.clear();
    m_ignoredComponentIds.clear();
    m_activeConnectionCount = 0;
    m_payloadBytesRead = 0;
    m_payloadBytesWritten = 0;
    m_maxPayloadBytes = 0;
    m_tokenReadCount = 0;
    m_tokenWriteCount = 0;

    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_completedSteps.clear();
}
