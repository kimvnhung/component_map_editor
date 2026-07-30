#include "GraphExecutionSandbox.h"

#include <algorithm>
#include <base_log.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "adapters/ExecutionAdapter.h"
#include "adapters/GraphAdapter.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/PublicApiContractAdapter.h"
#include "services/ExecutionMigrationFlags.h"
#include "utils/GraphHelper.h"

namespace
{

    bool idComparator(const QString &a, const QString &b)
    {
        return a < b;
    }

    QVariantMap mergeIncomingTokens(const cme::execution::IncomingTokens &incomingTokens)
    {
        QVariantMap merged;
        QStringList tokenKeys = incomingTokens.keys();
        std::sort(tokenKeys.begin(), tokenKeys.end());

        for (const QString &tokenKey : tokenKeys)
        {
            merged.insert(incomingTokens.value(tokenKey));
        }

        return merged;
    }

    QVariant redactVariant(const QVariant &value,
                           const QSet<QString> &sensitiveKeys,
                           int *redactedCount)
    {
        if (value.metaType().id() == QMetaType::QVariantMap)
        {
            const QVariantMap map = value.toMap();
            QVariantMap redacted;
            QStringList keys = map.keys();
            std::sort(keys.begin(), keys.end());

            for (const QString &key : keys)
            {
                if (sensitiveKeys.contains(key))
                {
                    redacted.insert(key, QStringLiteral("<redacted>"));

                    if (redactedCount)
                    {
                        ++(*redactedCount);
                    }
                }
                else
                {
                    redacted.insert(key, redactVariant(map.value(key), sensitiveKeys, redactedCount));
                }
            }

            return redacted;
        }

        if (value.metaType().id() == QMetaType::QVariantList)
        {
            const QVariantList list = value.toList();
            QVariantList redacted;
            redacted.reserve(list.size());

            for (const QVariant &item : list)
            {
                redacted.append(redactVariant(item, sensitiveKeys, redactedCount));
            }

            return redacted;
        }

        return value;
    }

    qint64 estimatePayloadBytes(const QVariantMap &payload)
    {
        return QJsonDocument::fromVariant(payload).toJson(QJsonDocument::Compact).size();
    }

} // namespace

GraphExecutionSandbox::GraphExecutionSandbox(QObject *parent)
    : QObject(parent)
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
    return QVariantMap
    {
        { QStringLiteral("tokenReadCount"), m_tokenReadCount },
        { QStringLiteral("tokenWriteCount"), m_tokenWriteCount },
        { QStringLiteral("payloadBytesRead"), m_payloadBytesRead },
        { QStringLiteral("payloadBytesWritten"), m_payloadBytesWritten },
        { QStringLiteral("maxPayloadBytes"), m_maxPayloadBytes },
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

bool GraphExecutionSandbox::start(const QVariantMap &inputSnapshot)
{
    reset();

    if (!captureGraphSnapshot())
    {
        return false;
    }

    m_inputSnapshot = inputSnapshot;
    m_executionState = inputSnapshot;
    emit executionStateChanged();

    appendTimelineEvent(TimelineEventKind::SimulationStarted,
                        QVariantMap
    {
        { QStringLiteral("componentCount"), cme::helper::getComponentCount(m_graphSnapshot) },
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
    const bool ok = executeOneStep(true);

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

        if (m_readyQueue.isEmpty())
        {
            finalizeIfNoReadyComponents();
            break;
        }

        const QString nextId = m_readyQueue.first();

        if (m_breakpoints.contains(nextId))
        {
            appendTimelineEvent(TimelineEventKind::BreakpointHit,
                                QVariantMap
            {
                { QStringLiteral("componentId"), nextId },
                { QStringLiteral("tick"), m_tick }
            });
            appendTimelineEvent(TimelineEventKind::SimulationPaused,
                                QVariantMap
            {
                { QStringLiteral("reason"), QStringLiteral("breakpoint") },
                { QStringLiteral("componentId"), nextId }
            });
            setStatus(RunStatus::Paused);
            break;
        }

        if (!executeOneStep(false))
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

void GraphExecutionSandbox::setBreakpoint(const QString &componentId, bool enabled)
{
    if (componentId.trimmed().isEmpty())
    {
        return;
    }

    if (enabled)
    {
        m_breakpoints.insert(componentId);
    }
    else
    {
        m_breakpoints.remove(componentId);
    }
}

void GraphExecutionSandbox::clearBreakpoints()
{
    m_breakpoints.clear();
}

QStringList GraphExecutionSandbox::breakpoints() const
{
    QStringList ids = m_breakpoints.values();
    std::sort(ids.begin(), ids.end(), idComparator);
    return ids;
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
        { QStringLiteral("componentCount"), cme::helper::getComponentCount(m_graphSnapshot) },
        { QStringLiteral("executedCount"), m_executed.size() },
        { QStringLiteral("pendingCount"), cme::helper::getComponentCount(m_graphSnapshot) - m_executed.size() },
        { QStringLiteral("readyQueue"), m_readyQueue },
        { QStringLiteral("breakpoints"), breakpoints() },
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
    QStringList componentIds = cme::helper::getComponentIds(m_graphSnapshot);
    std::sort(componentIds.begin(), componentIds.end());

    for (const QString &componentId : componentIds)
    {
        const QVariantMap state = m_componentStates.value(componentId).toMap();
        QVariantMap entry
        {
            { QStringLiteral("componentId"), componentId },
            { QStringLiteral("type"), QString::fromStdString(cme::helper::getComponentById(m_graphSnapshot, componentId).type_id()) },
            { QStringLiteral("consumedIncomingTokenIds"), state.value(QStringLiteral("consumedIncomingTokenIds")) },
            { QStringLiteral("producedOutgoingConnectionIds"), state.value(QStringLiteral("producedOutgoingConnectionIds")) },
            {
                QStringLiteral("lastOutputSummary"),
                redactVariant(state.value(QStringLiteral("outputState")), m_sensitiveDebugKeys, &redactedCount)
            }
        };
        components.append(entry);
    }

    QList<cme::ConnectionData> allEdges;
    auto outgoingMap = cme::helper::getOutgoingConnectionsBySourceId(m_graphSnapshot);

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
        const QVariantMap payload = cme::helper::getConnectionPayloadById(m_graphSnapshot,
                                    QString::fromStdString(edge.id()));
        const QVariantMap redactedPayload = redactVariant(payload, m_sensitiveDebugKeys, &redactedCount).toMap();
        connections.append(QVariantMap
        {
            { QStringLiteral("connectionId"), QString::fromStdString(edge.id()) },
            { QStringLiteral("sourceId"), QString::fromStdString(edge.source_id()) },
            { QStringLiteral("targetId"), QString::fromStdString(edge.target_id()) },
            { QStringLiteral("label"), QString::fromStdString(edge.label()) },
            { QStringLiteral("payloadBytes"), estimatePayloadBytes(payload) },
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

        case TimelineEventKind::BreakpointHit:
            return cme::TIMELINE_EVENT_TYPE_BREAKPOINT_HIT;

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

    m_inputSnapshot.clear();
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

    m_payloadBytesRead = 0;
    m_payloadBytesWritten = 0;
    m_maxPayloadBytes = 0;
    m_tokenReadCount = 0;
    m_tokenWriteCount = 0;
    m_redactedFieldCount = 0;

    m_graphSnapshot.Clear();
    m_pendingInDegree.clear();
    m_executed.clear();
    m_readyQueue.clear();
    m_readyQueueSet.clear();
}

bool GraphExecutionSandbox::captureGraphSnapshot()
{
    if (!m_graph)
    {
        markError(QStringLiteral("Graph is not set."));
        return false;
    }

    const QList<ComponentModel *> components = m_graph->componentList();

    for (ComponentModel *component : components)
    {
        if (!component)
        {
            continue;
        }

        const QString componentId = component->id().trimmed();

        if (componentId.isEmpty())
        {
            continue;
        }

        // ComponentSnapshot snap;
        // snap.id = componentId;
        // snap.type = component->type();
        // snap.title = component->title();
        // snap.attributes = QVariantMap
        // {
        //     { QStringLiteral("id"), snap.id },
        //     { QStringLiteral("type"), snap.type },
        //     { QStringLiteral("title"), snap.title },
        //     { QStringLiteral("x"), component->x() },
        //     { QStringLiteral("y"), component->y() },
        //     { QStringLiteral("width"), component->width() },
        //     { QStringLiteral("height"), component->height() },
        //     { QStringLiteral("color"), component->color() },
        //     { QStringLiteral("shape"), component->shape() }
        // };
        cme::ComponentData *snap = m_graphSnapshot.add_components();
        snap->set_id(componentId.toStdString());
        snap->set_type_id(component->type().toStdString());
        snap->set_title(component->title().toStdString());
        // Set properties
        auto *properties = snap->mutable_properties();
        (*properties)["x"] = QString::number(component->x()).toStdString();
        (*properties)["y"] = QString::number(component->y()).toStdString();
        (*properties)["width"] = QString::number(component->width()).toStdString();
        (*properties)["height"] = QString::number(component->height()).toStdString();
        (*properties)["color"] = component->color().toStdString();
        (*properties)["shape"] = component->shape().toStdString();

        // Capture dynamic QML properties so extension execution semantics can
        // consume schema-defined fields (for example inputNumber/addValue).
        const QList<QByteArray> dynamicProps = component->dynamicPropertyNames();

        for (const QByteArray &propName : dynamicProps)
        {
            const QString key = QString::fromUtf8(propName);

            if (key.isEmpty())
            {
                continue;
            }

            (*properties)[key.toStdString()] = component->property(propName.constData()).toString().toStdString();
        }
    }

    for (auto it = m_graphSnapshot.components().begin(); it != m_graphSnapshot.components().end(); ++it)
    {
        m_pendingInDegree.insert(it->id(), 0);
    }

    const QList<ConnectionModel *> connections = m_graph->connectionList();

    for (ConnectionModel *connection : connections)
    {
        if (!connection)
        {
            continue;
        }

        cme::ConnectionData conn;
        conn.set_id(connection->id().toStdString());
        conn.set_source_id(connection->sourceId().toStdString());
        conn.set_target_id(connection->targetId().toStdString());
        conn.set_label(connection->label().toStdString());

        // Check if source and target components exist in the graph snapshot
        if (!m_pendingInDegree.contains(conn.source_id()) ||
                !m_pendingInDegree.contains(conn.target_id()))
        {
            markError(QStringLiteral("Connection '%1' references non-existent source or target component.").arg(
                          QString::fromStdString(conn.id())));
            return false;
        }

        m_pendingInDegree[conn.target_id()] = m_pendingInDegree.value(conn.target_id(), 0) + 1;
        m_graphSnapshot.mutable_connections()->Add(std::move(conn));
    }

    auto outgoingMap = cme::helper::getOutgoingConnectionsBySourceId(m_graphSnapshot);

    for (auto it = outgoingMap.begin(); it != outgoingMap.end(); ++it)
    {
        QList<cme::ConnectionData> &edges = it.value();
        std::sort(edges.begin(), edges.end(), [](const cme::ConnectionData & a, const cme::ConnectionData & b)
        {
            if (a.target_id() != b.target_id())
            {
                return a.target_id() < b.target_id();
            }

            return a.id() < b.id();
        });
    }

    auto incomingMap = cme::helper::getIncomingConnectionsByTargetId(m_graphSnapshot);

    for (auto it = incomingMap.begin(); it != incomingMap.end(); ++it)
    {
        QList<cme::ConnectionData> &edges = it.value();
        std::sort(edges.begin(), edges.end(), [](const cme::ConnectionData & a, const cme::ConnectionData & b)
        {
            if (a.source_id() != b.source_id())
            {
                return a.source_id() < b.source_id();
            }

            return a.id() < b.id();
        });
    }

    QStringList componentIds = cme::helper::getComponentIds(m_graphSnapshot);
    std::sort(componentIds.begin(), componentIds.end(), idComparator);

    for (const QString &componentId : componentIds)
    {
        if (m_pendingInDegree.value(componentId.toStdString(), 0) == 0)
        {
            enqueueReadyComponent(componentId);
        }
    }

    return true;
}

QList<QVariantMap> GraphExecutionSandbox::collectIncomingTokens(const cme::GraphSnapshot & graph,
        const QString & componentId, const RunStatus & status)
{
    Q_UNUSED(graph);
    Q_UNUSED(status);
    const bool tokenRoutingEnabled = cme::execution::MigrationFlags::tokenTransportEnabled();
    const QList<cme::ConnectionData> incoming = cme::helper::getConnectionsByTargetId(m_graphSnapshot, componentId);
    const QList<cme::ConnectionData> outgoing = cme::helper::getConnectionsBySourceId(m_graphSnapshot, componentId);

    QVariantMap trace;
    QVariantMap outputState = m_executionState;
    cme::execution::IncomingTokens incomingTokens;
    QVariantMap inputStateForState;
    QVariantMap incomingTokenPayloads;
    QStringList incomingTokenIds;
    QStringList outgoingConnectionIds;
    outgoingConnectionIds.reserve(outgoing.size());

    for (const cme::ConnectionData &edge : outgoing)
    {
        outgoingConnectionIds.append(QString::fromStdString(edge.id()));
    }

    std::sort(outgoingConnectionIds.begin(), outgoingConnectionIds.end());

    if (tokenRoutingEnabled)
    {

        for (const cme::ConnectionData &edge : incoming)
        {
            auto tokenPayload = cme::helper::getConnectionPayloadById(m_graphSnapshot, QString::fromStdString(edge.id()));
            incomingTokens.insert(QString::fromStdString(edge.id()), tokenPayload);
        }

        if (incomingTokens.isEmpty() && !m_inputSnapshot.isEmpty())
        {
            incomingTokens.insert(QStringLiteral("__graph_input__"), m_inputSnapshot);
        }

        inputStateForState = mergeIncomingTokens(incomingTokens);
    }
    else
    {
        incomingTokens.insert(QStringLiteral("__legacy_global_state__"), m_executionState);
        inputStateForState = m_executionState;
    }

    incomingTokenIds = incomingTokens.keys();
    std::sort(incomingTokenIds.begin(), incomingTokenIds.end());

    for (const QString &tokenId : incomingTokenIds)
    {
        incomingTokenPayloads.insert(tokenId, incomingTokens.value(tokenId));
    }

    for (const QString &tokenId : incomingTokenIds)
    {
        ++m_tokenReadCount;
        const qint64 bytes = estimatePayloadBytes(incomingTokens.value(tokenId));
        m_payloadBytesRead += bytes;
        m_maxPayloadBytes = qMax(m_maxPayloadBytes, bytes);
    }

    return incomingTokens.values();
}

bool GraphExecutionSandbox::executeOneStep(bool bypassBreakpoint)
{
    if (m_readyQueue.isEmpty())
    {
        finalizeIfNoReadyComponents();
        return m_status != RunStatus::Error;
    }

    const QString componentId = m_readyQueue.first();

    if (!bypassBreakpoint && m_breakpoints.contains(componentId))
    {
        appendTimelineEvent(TimelineEventKind::BreakpointHit,
                            QVariantMap
        {
            { QStringLiteral("componentId"), componentId },
            { QStringLiteral("tick"), m_tick }
        });
        appendTimelineEvent(TimelineEventKind::SimulationPaused,
                            QVariantMap
        {
            { QStringLiteral("reason"), QStringLiteral("breakpoint") },
            { QStringLiteral("componentId"), componentId }
        });
        setStatus(RunStatus::Paused);
        return true;
    }

    m_readyQueue.removeFirst();
    m_readyQueueSet.remove(componentId);
    cme::ComponentData component = cme::helper::getComponentById(m_graphSnapshot, componentId);
    const bool tokenRoutingEnabled = cme::execution::MigrationFlags::tokenTransportEnabled();
    const QList<cme::ConnectionData> incoming = cme::helper::getConnectionsByTargetId(m_graphSnapshot, componentId);
    const QList<cme::ConnectionData> outgoing = cme::helper::getConnectionsBySourceId(m_graphSnapshot, componentId);

    QVariantMap trace;
    QVariantMap outputState = m_executionState;
    cme::execution::IncomingTokens incomingTokens;
    QVariantMap inputStateForState;
    QVariantMap incomingTokenPayloads;
    QStringList incomingTokenIds;
    QStringList outgoingConnectionIds;
    outgoingConnectionIds.reserve(outgoing.size());

    for (const cme::ConnectionData &edge : outgoing)
    {
        outgoingConnectionIds.append(QString::fromStdString(edge.id()));
    }

    std::sort(outgoingConnectionIds.begin(), outgoingConnectionIds.end());

    if (tokenRoutingEnabled)
    {

        for (const cme::ConnectionData &edge : incoming)
        {
            auto tokenPayload = cme::helper::getConnectionPayloadById(m_graphSnapshot, QString::fromStdString(edge.id()));
            incomingTokens.insert(QString::fromStdString(edge.id()), tokenPayload);
        }

        if (incomingTokens.isEmpty() && !m_inputSnapshot.isEmpty())
        {
            incomingTokens.insert(QStringLiteral("__graph_input__"), m_inputSnapshot);
        }

        inputStateForState = mergeIncomingTokens(incomingTokens);
    }
    else
    {
        incomingTokens.insert(QStringLiteral("__legacy_global_state__"), m_executionState);
        inputStateForState = m_executionState;
    }

    incomingTokenIds = incomingTokens.keys();
    std::sort(incomingTokenIds.begin(), incomingTokenIds.end());

    for (const QString &tokenId : incomingTokenIds)
    {
        incomingTokenPayloads.insert(tokenId, incomingTokens.value(tokenId));
    }

    for (const QString &tokenId : incomingTokenIds)
    {
        ++m_tokenReadCount;
        const qint64 bytes = estimatePayloadBytes(incomingTokens.value(tokenId));
        m_payloadBytesRead += bytes;
        m_maxPayloadBytes = qMax(m_maxPayloadBytes, bytes);
    }

    const IExecutionSemanticsProvider *provider = m_providerByComponentType.value(QString::fromStdString(
            component.type_id()), nullptr);

    if (provider)
    {
        QString error;

        if (!provider->executeComponent(QString::fromStdString(component.type_id()),
                                        QString::fromStdString(component.id()),
                                        cme::adapter::componentSnapshot(component),
                                        incomingTokens,
                                        &outputState,
                                        &trace,
                                        &error))
        {
            markError(error.isEmpty() ? QStringLiteral("Execution semantics provider returned failure.")
                      : error);
            return false;
        }

        // Validate that outputPayload contains at least one of the provider's
        // declared output keys. Also accept any keys explicitly configured
        // in the component's snapshot (e.g. outputKey="mySum"), since those
        // override the default declared names.
        const QStringList declared = provider->providedOutputKeys(QString::fromStdString(component.type_id()));

        if (!declared.isEmpty())
        {
            bool matched = false;

            for (const QString &key : declared)
            {
                if (outputState.contains(key))
                {
                    matched = true;
                    break;
                }
            }

            if (!matched)
            {
                // Fall back: check any snapshot-level output-key property
                static const QStringList kOutputKeyProps =
                {
                    QStringLiteral("outputKey"),
                    QStringLiteral("trueRouteKey"),
                    QStringLiteral("falseRouteKey"),
                    QStringLiteral("iterKey"),
                    QStringLiteral("continueKey"),
                    QStringLiteral("errorKey")
                };

                for (const QString &prop : kOutputKeyProps)
                {
                    const QString configured = QString::fromStdString(component.properties().find(prop.toStdString())->second).trimmed();

                    if (!configured.isEmpty() && outputState.contains(configured))
                    {
                        matched = true;
                        break;
                    }
                }
            }

            if (!matched)
            {
                markError(QStringLiteral("Provider '%1': output payload for type '%2' is missing all declared keys [%3].")
                          .arg(provider->providerId(), component.type_id(), declared.join(QStringLiteral(", "))));
                return false;
            }
        }
    }
    else
    {
        trace.insert(QStringLiteral("provider"), QStringLiteral("default"));
        trace.insert(QStringLiteral("note"), QStringLiteral("No execution semantics provider registered for component type."));

        if (tokenRoutingEnabled)
        {
            QVariantMap mergedIncoming;

            for (const cme::ConnectionData &edge : incoming)
            {
                // mergedIncoming.insert(m_connectionTokens.value(edge.id));
                mergedIncoming.insert(QString::fromStdString(edge.id()), cme::helper::getConnectionPayloadById(m_graphSnapshot,
                                      QString::fromStdString(edge.id())));
            }

            outputState = mergedIncoming;
        }

        outputState.insert(QStringLiteral("lastExecutedComponentId"), QString::fromStdString(component.id()));
    }

    m_executionState = outputState;
    emit executionStateChanged();

    QVariantMap state = m_componentStates.value(QString::fromStdString(component.id())).toMap();
    state.insert(QStringLiteral("status"), QStringLiteral("executed"));
    state.insert(QStringLiteral("tick"), m_tick);
    state.insert(QStringLiteral("type"), QString::fromStdString(component.type_id()));
    state.insert(QStringLiteral("inputState"), inputStateForState);
    state.insert(QStringLiteral("incomingTokenPayloads"), incomingTokenPayloads);
    state.insert(QStringLiteral("outputState"), outputState);
    state.insert(QStringLiteral("consumedIncomingTokenIds"), incomingTokenIds);
    state.insert(QStringLiteral("producedOutgoingConnectionIds"), outgoingConnectionIds);

    if (!trace.isEmpty())
    {
        state.insert(QStringLiteral("trace"), trace);
    }

    m_componentStates.insert(QString::fromStdString(component.id()), state);

    m_executed.insert(QString::fromStdString(component.id()));

    int stepRedactedCount = 0;
    const QVariant redactedOutputSummary = redactVariant(outputState, m_sensitiveDebugKeys, &stepRedactedCount);
    m_redactedFieldCount += stepRedactedCount;

    appendTimelineEvent(TimelineEventKind::StepExecuted,
                        QVariantMap
    {
        { QStringLiteral("componentId"), QString::fromStdString(component.id()) },
        { QStringLiteral("componentType"), QString::fromStdString(component.type_id()) },
        { QStringLiteral("incomingTokenCount"), incomingTokens.size() },
        { QStringLiteral("incomingTokenIds"), incomingTokenIds },
        { QStringLiteral("outgoingConnectionIds"), outgoingConnectionIds },
        { QStringLiteral("outputPayloadBytes"), estimatePayloadBytes(outputState) },
        { QStringLiteral("outputPayloadSummary"), redactedOutputSummary },
        { QStringLiteral("trace"), trace }
    });

    if (tokenRoutingEnabled)
    {
        for (const cme::ConnectionData &edge : outgoing)
        {
            cme::helper::setPayload(m_graphSnapshot, QString::fromStdString(edge.id()), outputState);
            ++m_tokenWriteCount;
            const qint64 bytes = estimatePayloadBytes(outputState);
            m_payloadBytesWritten += bytes;
            m_maxPayloadBytes = qMax(m_maxPayloadBytes, bytes);
        }
    }

    for (const cme::ConnectionData &edge : outgoing)
    {
        const std::string targetId = edge.target_id();
        const int updatedInDegree = m_pendingInDegree.value(targetId, 0) - 1;
        m_pendingInDegree[targetId] = updatedInDegree;

        if (updatedInDegree == 0)
        {
            enqueueReadyComponent(QString::fromStdString(targetId));
        }
    }

    ++m_tick;
    emit currentTickChanged();
    finalizeIfNoReadyComponents();
    return true;
}

void GraphExecutionSandbox::finalizeIfNoReadyComponents()
{
    if (!m_readyQueue.isEmpty())
    {
        return;
    }

    int componentCount = cme::helper::getComponentCount(m_graphSnapshot);

    if (m_executed.size() == componentCount)
    {
        appendTimelineEvent(TimelineEventKind::SimulationCompleted,
                            QVariantMap
        {
            { QStringLiteral("executedCount"), m_executed.size() }
        });
        setStatus(RunStatus::Completed);
        return;
    }

    if (m_status == RunStatus::Running || m_status == RunStatus::Paused)
    {
        appendTimelineEvent(TimelineEventKind::SimulationBlocked,
                            QVariantMap
        {
            { QStringLiteral("executedCount"), m_executed.size() },
            { QStringLiteral("remainingCount"), componentCount - m_executed.size() }
        });
        setStatus(RunStatus::Completed);
    }
}

void GraphExecutionSandbox::enqueueReadyComponent(const QString & componentId)
{
    if (componentId.isEmpty() || m_executed.contains(componentId) || m_readyQueueSet.contains(componentId))
    {
        return;
    }

    auto it = std::lower_bound(m_readyQueue.begin(), m_readyQueue.end(), componentId, idComparator);
    m_readyQueue.insert(it, componentId);
    m_readyQueueSet.insert(componentId);
}

