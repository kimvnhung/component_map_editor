#include "GraphExecutionSandbox.h"

#include <algorithm>
#include <base_log.h>

#include "adapters/ExecutionAdapter.h"
#include "adapters/GraphAdapter.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/PublicApiContractAdapter.h"
#include "services/ExecutionMigrationFlags.h"
#include "services/execution_internal/ActorMailboxExecutionEngine.h"
#include "services/execution_internal/SequentialExecutionEngine.h"
#include "utils/GraphHelper.h"

GraphExecutionSandbox::GraphExecutionSandbox(QObject *parent)
    : QObject(parent)
    , m_engine(std::make_unique<SequentialExecutionEngine>())
    , m_graph(nullptr)
    , m_timeline(nullptr)
{
    m_timeline = new TimelineModel(this);
}

GraphModel *GraphExecutionSandbox::graph() const
{
    return m_graph;
}

void GraphExecutionSandbox::setGraph(GraphModel *graph)
{
    if (m_graph == graph)
    {
        return;
    }

    m_graph = graph;
    reset();
    emit graphChanged();
}

QString GraphExecutionSandbox::status() const
{
    return statusToString(m_status);
}

int GraphExecutionSandbox::currentTick() const
{
    return m_tick;
}

TimelineModel *GraphExecutionSandbox::timeline() const
{
    return m_timeline;
}

QVariantMap GraphExecutionSandbox::executionState() const
{
    return m_executionState;
}

QString GraphExecutionSandbox::lastError() const
{
    return m_lastError;
}

QVariantMap GraphExecutionSandbox::providerOutputKeyHints() const
{
    QVariantMap hints;

    for (auto it = m_providerByComponentType.constBegin(); it != m_providerByComponentType.constEnd(); ++it)
    {
        const IExecutionSemanticsProvider *provider = it.value();

        if (!provider)
        {
            continue;
        }

        const QStringList keys = provider->providedOutputKeys(it.key());

        if (!keys.isEmpty())
        {
            hints.insert(it.key(), keys);
        }
    }

    return hints;
}

QVariantMap GraphExecutionSandbox::executionTelemetry() const
{
    const IExecutionEngine::Telemetry telemetry = m_engine ? m_engine->telemetry() : IExecutionEngine::Telemetry{};
    return QVariantMap
    {
        { QStringLiteral("tokenReadCount"), telemetry.tokenReadCount },
        { QStringLiteral("tokenWriteCount"), telemetry.tokenWriteCount },
        { QStringLiteral("payloadBytesRead"), telemetry.payloadBytesRead },
        { QStringLiteral("payloadBytesWritten"), telemetry.payloadBytesWritten },
        { QStringLiteral("maxPayloadBytes"), telemetry.maxPayloadBytes },
        { QStringLiteral("redactedFieldCount"), m_redactedFieldCount }
    };
}

QStringList GraphExecutionSandbox::sensitiveDebugKeys() const
{
    QStringList keys = m_sensitiveDebugKeys.values();
    std::sort(keys.begin(), keys.end());
    return keys;
}

void GraphExecutionSandbox::setSensitiveDebugKeys(const QStringList &keys)
{
    m_sensitiveDebugKeys = QSet<QString>(keys.begin(), keys.end());
}

void GraphExecutionSandbox::setExecutionSemanticsProviders(const QList<const IExecutionSemanticsProvider *> &providers)
{
    m_providerByComponentType.clear();

    for (const IExecutionSemanticsProvider *provider : providers)
    {
        if (!provider)
        {
            continue;
        }

        const QStringList supportedTypes = provider->supportedComponentTypes();

        for (const QString &componentType : supportedTypes)
        {
            if (componentType.isEmpty() || m_providerByComponentType.contains(componentType))
            {
                continue;
            }

            m_providerByComponentType.insert(componentType, provider);
        }
    }

    emit providerOutputKeyHintsChanged();
}

void GraphExecutionSandbox::rebuildSemanticsFromRegistry(const ExtensionContractRegistry &registry)
{
    setExecutionSemanticsProviders(registry.executionSemanticsProviders());
}

void GraphExecutionSandbox::forceSequentialEngineForNestedExecution()
{
    m_forceSequentialEngine = true;
}

void GraphExecutionSandbox::selectEngine()
{
    const bool wantActor = !m_forceSequentialEngine
                           && cme::execution::MigrationFlags::actorEngineEnabled()
                           && cme::execution::MigrationFlags::tokenTransportEnabled();

    if (m_engine && wantActor == m_usingActorEngine)
    {
        return;
    }

    if (wantActor)
    {
        m_engine = std::make_unique<ActorMailboxExecutionEngine>();
    }
    else
    {
        m_engine = std::make_unique<SequentialExecutionEngine>();
    }

    m_usingActorEngine = wantActor;
}

bool GraphExecutionSandbox::start(const QVariantMap &inputSnapshot)
{
    selectEngine();
    reset();

    QString error;

    if (!m_engine->prepare(m_graph, m_providerByComponentType, inputSnapshot, &error))
    {
        markError(error);
        return false;
    }

    m_executionState = inputSnapshot;
    emit executionStateChanged();

    appendTimelineEvent(TimelineEventKind::SimulationStarted,
                        QVariantMap
    {
        { QStringLiteral("componentCount"), m_engine->totalComponentCount() },
        { QStringLiteral("inputKeys"), inputSnapshot.keys() },
        {
            QStringLiteral("tokenTransportEnabled"),
            cme::execution::MigrationFlags::tokenTransportEnabled()
        }
    });

    setStatus(RunStatus::Paused);
    finalizeIfNoReadyComponents();
    return m_status != RunStatus::Error;
}

bool GraphExecutionSandbox::startTyped(const google::protobuf::Struct &inputSnapshot)
{
    const QVariantMap legacySnapshot =
        cme::runtime::PublicApiContractAdapter::protoStructToVariantMap(inputSnapshot);
    return start(legacySnapshot);
}

bool GraphExecutionSandbox::step()
{
    if (m_status == RunStatus::Idle || m_status == RunStatus::Completed || m_status == RunStatus::Error)
    {
        return false;
    }

    setStatus(RunStatus::Running);
    const bool ok = executeOneStep();

    if (!ok)
    {
        return false;
    }

    if (m_status == RunStatus::Running)
    {
        setStatus(RunStatus::Paused);
    }

    return true;
}

int GraphExecutionSandbox::run(int maxSteps)
{
    if (m_status == RunStatus::Idle || m_status == RunStatus::Completed || m_status == RunStatus::Error)
    {
        return 0;
    }

    setStatus(RunStatus::Running);
    m_deferTimelineSignal = true;
    int executed = 0;

    while (m_status == RunStatus::Running)
    {
        if (maxSteps >= 0 && executed >= maxSteps)
        {
            break;
        }

        if (!m_engine->hasReadyWork())
        {
            finalizeIfNoReadyComponents();
            break;
        }

        if (!executeOneStep())
        {
            break;
        }

        ++executed;
    }

    if (m_status == RunStatus::Running)
    {
        setStatus(RunStatus::Paused);
    }

    m_deferTimelineSignal = false;
    flushTimelineChanged();

    return executed;
}

void GraphExecutionSandbox::pause()
{
    if (m_status == RunStatus::Running)
    {
        appendTimelineEvent(TimelineEventKind::SimulationPaused,
                            QVariantMap
        {
            { QStringLiteral("reason"), QStringLiteral("manual") }
        });
        setStatus(RunStatus::Paused);
    }
}

void GraphExecutionSandbox::reset()
{
    clearSimulationData();
    setStatus(RunStatus::Idle);
}

QVariantMap GraphExecutionSandbox::componentState(const QString &componentId) const
{
    return m_componentStates.value(componentId).toMap();
}

bool GraphExecutionSandbox::componentStateTyped(const QString &componentId,
        google::protobuf::Struct *outState,
        QString *error) const
{
    if (!outState)
    {
        if (error)
        {
            *error = QStringLiteral("componentState output pointer is null");
        }

        return false;
    }

    const QVariantMap state = componentState(componentId);
    outState->Clear();
    cme::runtime::PublicApiContractAdapter::variantMapToProtoStruct(state, outState);
    return true;
}

QVariantMap GraphExecutionSandbox::snapshotSummary() const
{
    return QVariantMap
    {
        { QStringLiteral("componentCount"), m_engine->totalComponentCount() },
        { QStringLiteral("executedCount"), m_engine->executedCount() },
        { QStringLiteral("pendingCount"), m_engine->totalComponentCount() - m_engine->executedCount() },
        { QStringLiteral("readyQueue"), m_engine->readyComponentIds() },
        {
            QStringLiteral("tokenTransportEnabled"),
            cme::execution::MigrationFlags::tokenTransportEnabled()
        },
        { QStringLiteral("telemetry"), executionTelemetry() }
    };
}

QVariantMap GraphExecutionSandbox::debugSnapshot() const
{
    int redactedCount = 0;

    QVariantList components;
    QStringList componentIds = cme::helper::getComponentIds(m_engine->graphSnapshot());
    std::sort(componentIds.begin(), componentIds.end());

    for (const QString &componentId : componentIds)
    {
        const QVariantMap state = m_componentStates.value(componentId).toMap();
        QVariantMap entry
        {
            { QStringLiteral("componentId"), componentId },
            { QStringLiteral("type"), QString::fromStdString(cme::helper::getComponentById(m_engine->graphSnapshot(), componentId).type_id()) },
            { QStringLiteral("consumedIncomingTokenIds"), state.value(QStringLiteral("consumedIncomingTokenIds")) },
            { QStringLiteral("producedOutgoingConnectionIds"), state.value(QStringLiteral("producedOutgoingConnectionIds")) },
            {
                QStringLiteral("lastOutputSummary"),
                cme::helper::redactVariant(state.value(QStringLiteral("outputState")), m_sensitiveDebugKeys, &redactedCount)
            }
        };
        components.append(entry);
    }

    QList<cme::ConnectionData> allEdges;
    auto outgoingMap = cme::helper::getOutgoingConnectionsBySourceId(m_engine->graphSnapshot());

    for (auto it = outgoingMap.constBegin(); it != outgoingMap.constEnd(); ++it)
    {
        for (const cme::ConnectionData &edge : it.value())
        {
            allEdges.append(edge);
        }
    }

    std::sort(allEdges.begin(), allEdges.end(), [](const cme::ConnectionData & a, const cme::ConnectionData & b)
    {
        return a.id() < b.id();
    });

    QVariantList connections;

    for (const cme::ConnectionData &edge : allEdges)
    {
        const QVariantMap payload = cme::helper::getConnectionPayloadById(m_engine->graphSnapshot(),
                                    QString::fromStdString(edge.id()));
        const QVariantMap redactedPayload = cme::helper::redactVariant(payload, m_sensitiveDebugKeys, &redactedCount).toMap();
        connections.append(QVariantMap
        {
            { QStringLiteral("connectionId"), QString::fromStdString(edge.id()) },
            { QStringLiteral("sourceId"), QString::fromStdString(edge.source_id()) },
            { QStringLiteral("targetId"), QString::fromStdString(edge.target_id()) },
            { QStringLiteral("label"), QString::fromStdString(edge.label()) },
            { QStringLiteral("payloadBytes"), cme::helper::estimatePayloadBytes(payload) },
            { QStringLiteral("payloadSummary"), redactedPayload }
        });
    }

    QVariantMap snapshot
    {
        { QStringLiteral("status"), status() },
        { QStringLiteral("currentTick"), currentTick() },
        { QStringLiteral("tokenTransportEnabled"), cme::execution::MigrationFlags::tokenTransportEnabled() },
        { QStringLiteral("sensitiveDebugKeys"), sensitiveDebugKeys() },
        { QStringLiteral("components"), components },
        { QStringLiteral("connections"), connections },
        { QStringLiteral("telemetry"), executionTelemetry() }
    };
    snapshot.insert(QStringLiteral("redactedFieldCountPreview"), redactedCount);
    return snapshot;
}

cme::ExecutionSnapshot GraphExecutionSandbox::executionSnapshotTyped() const
{
    cme::ExecutionSnapshot snapshot;

    for (const cme::TimelineEvent &event : m_typedTimeline)
    {
        *snapshot.add_events() = event;
    }

    for (auto it = m_componentStates.constBegin(); it != m_componentStates.constEnd(); ++it)
    {
        cme::ComponentExecutionState *state = snapshot.add_component_states();
        state->set_component_id(it.key().toStdString());

        const QVariantMap asMap = it.value().toMap();
        const QVariantMap inputMap = asMap.value(QStringLiteral("inputState")).toMap();
        const QVariantMap outputMap = asMap.value(QStringLiteral("outputState")).toMap();
        const QString trace = asMap.value(QStringLiteral("trace")).toString();

        for (auto m = inputMap.constBegin(); m != inputMap.constEnd(); ++m)
        {
            (*state->mutable_input_state())[m.key().toStdString()] = m.value().toString().toStdString();
        }

        for (auto m = outputMap.constBegin(); m != outputMap.constEnd(); ++m)
        {
            (*state->mutable_output_state())[m.key().toStdString()] = m.value().toString().toStdString();
        }

        if (!trace.isEmpty())
        {
            state->set_trace(trace.toStdString());
        }
    }

    return snapshot;
}

QString GraphExecutionSandbox::statusToString(RunStatus status)
{
    switch (status)
    {
        case RunStatus::Idle:
            return QStringLiteral("idle");

        case RunStatus::Running:
            return QStringLiteral("running");

        case RunStatus::Paused:
            return QStringLiteral("paused");

        case RunStatus::Completed:
            return QStringLiteral("completed");

        case RunStatus::Error:
            return QStringLiteral("error");
    }

    return QStringLiteral("error");
}

cme::TimelineEventType GraphExecutionSandbox::timelineKindToProtoType(TimelineEventKind kind)
{
    switch (kind)
    {
        case TimelineEventKind::SimulationStarted:
            return cme::TIMELINE_EVENT_TYPE_SIMULATION_STARTED;

        case TimelineEventKind::StepExecuted:
            return cme::TIMELINE_EVENT_TYPE_STEP_EXECUTED;

        case TimelineEventKind::SimulationPaused:
            return cme::TIMELINE_EVENT_TYPE_SIMULATION_PAUSED;

        case TimelineEventKind::SimulationCompleted:
            return cme::TIMELINE_EVENT_TYPE_SIMULATION_COMPLETED;

        case TimelineEventKind::SimulationBlocked:
            return cme::TIMELINE_EVENT_TYPE_SIMULATION_BLOCKED;

        case TimelineEventKind::Error:
            return cme::TIMELINE_EVENT_TYPE_ERROR;
    }

    return cme::TIMELINE_EVENT_TYPE_UNSPECIFIED;
}

void GraphExecutionSandbox::setStatus(RunStatus status)
{
    if (m_status == status)
    {
        return;
    }

    m_status = status;
    emit statusChanged();
}

void GraphExecutionSandbox::appendTimelineEvent(TimelineEventKind kind, const QVariantMap & payload)
{
    cme::TimelineEvent typedEvent;
    typedEvent.set_type(timelineKindToProtoType(kind));

    const QString componentId = payload.value(QStringLiteral("componentId")).toString();

    if (!componentId.isEmpty())
    {
        typedEvent.set_component_id(componentId.toStdString());
    }

    const QString message = payload.value(QStringLiteral("message")).toString();

    if (!message.isEmpty())
    {
        typedEvent.set_message(message.toStdString());
    }

    m_typedTimeline.append(typedEvent);

    QVariantMap entry = cme::adapter::timelineEventToVariantMap(typedEvent);
    entry.insert(payload);
    entry.insert(QStringLiteral("event"), entry.value(QStringLiteral("type")).toString());
    entry.insert(QStringLiteral("tick"), m_tick);
    m_timeline->append(
    {
        entry.value(QStringLiteral("type")).toString(),
        m_tick,
        payload
    });
    m_timelineDirty = true;

    if (!m_deferTimelineSignal)
    {
        flushTimelineChanged();
    }
}

void GraphExecutionSandbox::flushTimelineChanged()
{
    if (!m_timelineDirty)
    {
        return;
    }

    m_timelineDirty = false;
    emit timelineChanged();
}

void GraphExecutionSandbox::markError(const QString & message)
{
    m_lastError = message;
    emit lastErrorChanged();
    appendTimelineEvent(TimelineEventKind::Error,
                        QVariantMap
    {
        { QStringLiteral("message"), message }
    });
    setStatus(RunStatus::Error);
}

void GraphExecutionSandbox::clearSimulationData()
{
    m_tick = 0;
    emit currentTickChanged();

    m_executionState.clear();
    m_componentStates.clear();

    if (m_timeline)
    {
        m_timeline->clear();
    }

    m_typedTimeline.clear();
    m_lastError.clear();
    emit executionStateChanged();
    m_timelineDirty = true;
    flushTimelineChanged();
    emit lastErrorChanged();

    m_redactedFieldCount = 0;

    if (m_engine)
    {
        m_engine->reset();
    }
}

void GraphExecutionSandbox::commitExecutionState(const ExecutionContext &ctx, const ExecuteResult &result)
{
    QVariantMap state = componentState(ctx.componentId);
    state.insert(QStringLiteral("status"), QStringLiteral("executed"));
    state.insert(QStringLiteral("tick"), m_tick);
    state.insert(QStringLiteral("type"), ctx.componentType);
    state.insert(QStringLiteral("inputState"), ctx.stepState);
    auto incomingTokenPayloads = QVariantMap();

    for (const QString &tokenId : ctx.incomingTokens.keys())
    {
        incomingTokenPayloads.insert(tokenId, ctx.incomingTokens.value(tokenId));
    }

    state.insert(QStringLiteral("incomingTokenPayloads"), incomingTokenPayloads);
    state.insert(QStringLiteral("outputState"), result.output);
    auto incomingTokenIds = ctx.incomingTokens.keys();
    state.insert(QStringLiteral("consumedIncomingTokenIds"), incomingTokenIds);
    auto outgoingConnections = cme::helper::getConnectionsBySourceId(m_engine->graphSnapshot(), ctx.componentId);
    auto outgoingConnectionIds = QStringList();

    for (const cme::ConnectionData &edge : outgoingConnections)
    {
        outgoingConnectionIds.append(QString::fromStdString(edge.id()));
    }

    state.insert(QStringLiteral("producedOutgoingConnectionIds"), outgoingConnectionIds);

    if (!result.trace.isEmpty())
    {
        state.insert(QStringLiteral("trace"), result.trace);
    }

    m_componentStates.insert(ctx.componentId, state);

    // In-degree/ready-queue/executed-set đã được m_engine cập nhật bên trong executeNext().
    ++m_tick;
    emit currentTickChanged();
    finalizeIfNoReadyComponents();
}

void GraphExecutionSandbox::recordTimelineEvent(const ExecutionContext &ctx, const ExecuteResult &result)
{
    int stepRedactedCount = 0;
    const QVariant redactedOutputSummary = cme::helper::redactVariant(result.output, m_sensitiveDebugKeys, &stepRedactedCount);
    m_redactedFieldCount += stepRedactedCount;

    auto incomingTokenIds = ctx.incomingTokens.keys();
    auto outgoingConnections = cme::helper::getConnectionsBySourceId(m_engine->graphSnapshot(), ctx.componentId);
    auto outgoingConnectionIds = QStringList();

    for (const cme::ConnectionData &edge : outgoingConnections)
    {
        outgoingConnectionIds.append(QString::fromStdString(edge.id()));
    }

    appendTimelineEvent(TimelineEventKind::StepExecuted,
                        QVariantMap
    {
        { QStringLiteral("componentId"), ctx.componentId},
        { QStringLiteral("componentType"), ctx.componentType },
        { QStringLiteral("incomingTokenCount"), ctx.incomingTokens.size()},
        { QStringLiteral("incomingTokenIds"), incomingTokenIds },
        { QStringLiteral("outgoingConnectionIds"), outgoingConnectionIds },
        { QStringLiteral("outputPayloadBytes"), cme::helper::estimatePayloadBytes(result.output) },
        { QStringLiteral("outputPayloadSummary"), redactedOutputSummary },
        { QStringLiteral("trace"), result.trace }
    });
}

bool GraphExecutionSandbox::executeOneStep()
{
    if (!m_engine->hasReadyWork())
    {
        finalizeIfNoReadyComponents();
        return m_status != RunStatus::Error;
    }

    ExecutionContext context;
    ExecuteResult result;
    QString error;
    const IExecutionEngine::StepOutcome outcome =
        m_engine->executeNext(m_executionState, &context, &result, &error);

    switch (outcome)
    {
        case IExecutionEngine::StepOutcome::NoWork:
            finalizeIfNoReadyComponents();
            return m_status != RunStatus::Error;

        case IExecutionEngine::StepOutcome::ProviderError:
            markError(error);
            return false;

        case IExecutionEngine::StepOutcome::Rejected:
            appendTimelineEvent(TimelineEventKind::SimulationBlocked,
                                QVariantMap
            {
                { QStringLiteral("componentId"), context.componentId },
                { QStringLiteral("tick"), m_tick },
                { QStringLiteral("reason"), QStringLiteral("rejected") }
            });
            setStatus(RunStatus::Paused);
            return true;

        case IExecutionEngine::StepOutcome::ValidationError:
            m_executionState = result.output;
            emit executionStateChanged();
            markError(error);
            return false;

        case IExecutionEngine::StepOutcome::Committed:
            break;
    }

    m_executionState = result.output;
    emit executionStateChanged();

    recordTimelineEvent(context, result);
    commitExecutionState(context, result);
    return true;
}

void GraphExecutionSandbox::finalizeIfNoReadyComponents()
{
    if (m_engine->hasReadyWork())
    {
        return;
    }

    const int componentCount = m_engine->totalComponentCount();
    const int executedCount = m_engine->executedCount();

    if (executedCount == componentCount)
    {
        appendTimelineEvent(TimelineEventKind::SimulationCompleted,
                            QVariantMap
        {
            { QStringLiteral("executedCount"), executedCount }
        });
        setStatus(RunStatus::Completed);
        return;
    }

    if (m_status == RunStatus::Running || m_status == RunStatus::Paused)
    {
        appendTimelineEvent(TimelineEventKind::SimulationBlocked,
                            QVariantMap
        {
            { QStringLiteral("executedCount"), executedCount },
            { QStringLiteral("remainingCount"), componentCount - executedCount }
        });
        setStatus(RunStatus::Completed);
    }
}

