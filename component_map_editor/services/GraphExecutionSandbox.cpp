#include "GraphExecutionSandbox.h"

#include <algorithm>

#include "extensions/contracts/ExtensionContractRegistry.h"

namespace {

bool idComparator(const QString &a, const QString &b)
{
    return a < b;
}

} // namespace

GraphExecutionSandbox::GraphExecutionSandbox(QObject *parent)
    : QObject(parent)
{}

GraphModel *GraphExecutionSandbox::graph() const
{
    return m_graph;
}

void GraphExecutionSandbox::setGraph(GraphModel *graph)
{
    if (m_graph == graph)
        return;
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

QVariantList GraphExecutionSandbox::timeline() const
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

void GraphExecutionSandbox::setExecutionSemanticsProviders(const QList<const IExecutionSemanticsProvider *> &providers)
{
    m_providerByComponentType.clear();
    for (const IExecutionSemanticsProvider *provider : providers) {
        if (!provider)
            continue;
        const QStringList supportedTypes = provider->supportedComponentTypes();
        for (const QString &componentType : supportedTypes) {
            if (componentType.isEmpty() || m_providerByComponentType.contains(componentType))
                continue;
            m_providerByComponentType.insert(componentType, provider);
        }
    }
}

void GraphExecutionSandbox::rebuildSemanticsFromRegistry(const ExtensionContractRegistry &registry)
{
    setExecutionSemanticsProviders(registry.executionSemanticsProviders());
}

bool GraphExecutionSandbox::start(const QVariantMap &inputSnapshot)
{
    reset();
    if (!captureGraphSnapshot())
        return false;

    m_inputSnapshot = inputSnapshot;
    m_executionState = inputSnapshot;
    emit executionStateChanged();

    appendTimelineEvent(QStringLiteral("simulationStarted"),
                        QVariantMap{
                            { QStringLiteral("componentCount"), m_nodesById.size() },
                            { QStringLiteral("inputKeys"), inputSnapshot.keys() }
                        });

    setStatus(RunStatus::Paused);
    finalizeIfNoReadyNodes();
    return m_status != RunStatus::Error;
}

bool GraphExecutionSandbox::step()
{
    if (m_status == RunStatus::Idle || m_status == RunStatus::Completed || m_status == RunStatus::Error)
        return false;

    setStatus(RunStatus::Running);
    const bool ok = executeOneStep(true);
    if (!ok)
        return false;

    if (m_status == RunStatus::Running)
        setStatus(RunStatus::Paused);

    return true;
}

int GraphExecutionSandbox::run(int maxSteps)
{
    if (m_status == RunStatus::Idle || m_status == RunStatus::Completed || m_status == RunStatus::Error)
        return 0;

    setStatus(RunStatus::Running);
    m_deferTimelineSignal = true;
    int executed = 0;

    while (m_status == RunStatus::Running) {
        if (maxSteps >= 0 && executed >= maxSteps)
            break;

        if (m_readyQueue.isEmpty()) {
            finalizeIfNoReadyNodes();
            break;
        }

        const QString nextId = m_readyQueue.first();
        if (m_breakpoints.contains(nextId)) {
            appendTimelineEvent(QStringLiteral("breakpointHit"),
                                QVariantMap{
                                    { QStringLiteral("componentId"), nextId },
                                    { QStringLiteral("tick"), m_tick }
                                });
            setStatus(RunStatus::Paused);
            break;
        }

        if (!executeOneStep(false))
            break;
        ++executed;
    }

    if (m_status == RunStatus::Running)
        setStatus(RunStatus::Paused);

    m_deferTimelineSignal = false;
    flushTimelineChanged();

    return executed;
}

void GraphExecutionSandbox::pause()
{
    if (m_status == RunStatus::Running)
        setStatus(RunStatus::Paused);
}

void GraphExecutionSandbox::reset()
{
    clearSimulationData();
    setStatus(RunStatus::Idle);
}

void GraphExecutionSandbox::setBreakpoint(const QString &componentId, bool enabled)
{
    if (componentId.trimmed().isEmpty())
        return;
    if (enabled)
        m_breakpoints.insert(componentId);
    else
        m_breakpoints.remove(componentId);
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

QVariantMap GraphExecutionSandbox::snapshotSummary() const
{
    return QVariantMap{
        { QStringLiteral("componentCount"), m_nodesById.size() },
        { QStringLiteral("executedCount"), m_executed.size() },
        { QStringLiteral("pendingCount"), m_nodesById.size() - m_executed.size() },
        { QStringLiteral("readyQueue"), m_readyQueue },
        { QStringLiteral("breakpoints"), breakpoints() }
    };
}

QString GraphExecutionSandbox::statusToString(RunStatus status)
{
    switch (status) {
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

void GraphExecutionSandbox::setStatus(RunStatus status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void GraphExecutionSandbox::appendTimelineEvent(const QString &event, const QVariantMap &payload)
{
    QVariantMap entry = payload;
    entry.insert(QStringLiteral("event"), event);
    entry.insert(QStringLiteral("tick"), m_tick);
    m_timeline.append(entry);
    m_timelineDirty = true;
    if (!m_deferTimelineSignal)
        flushTimelineChanged();
}

void GraphExecutionSandbox::flushTimelineChanged()
{
    if (!m_timelineDirty)
        return;

    m_timelineDirty = false;
    emit timelineChanged();
}

void GraphExecutionSandbox::markError(const QString &message)
{
    m_lastError = message;
    emit lastErrorChanged();
    appendTimelineEvent(QStringLiteral("error"),
                        QVariantMap{
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
    m_timeline.clear();
    m_lastError.clear();
    emit executionStateChanged();
    m_timelineDirty = true;
    flushTimelineChanged();
    emit lastErrorChanged();

    m_nodesById.clear();
    m_outgoingBySource.clear();
    m_pendingInDegree.clear();
    m_executed.clear();
    m_readyQueue.clear();
    m_readyQueueSet.clear();
}

bool GraphExecutionSandbox::captureGraphSnapshot()
{
    if (!m_graph) {
        markError(QStringLiteral("Graph is not set."));
        return false;
    }

    const QList<ComponentModel *> components = m_graph->componentList();
    for (ComponentModel *component : components) {
        if (!component)
            continue;
        const QString componentId = component->id().trimmed();
        if (componentId.isEmpty())
            continue;

        NodeSnapshot node;
        node.id = componentId;
        node.type = component->type();
        node.title = component->title();
        node.attributes = QVariantMap{
            { QStringLiteral("id"), node.id },
            { QStringLiteral("type"), node.type },
            { QStringLiteral("title"), node.title },
            { QStringLiteral("x"), component->x() },
            { QStringLiteral("y"), component->y() },
            { QStringLiteral("width"), component->width() },
            { QStringLiteral("height"), component->height() },
            { QStringLiteral("color"), component->color() },
            { QStringLiteral("shape"), component->shape() }
        };

        // Capture dynamic QML properties so extension execution semantics can
        // consume schema-defined fields (for example inputNumber/addValue).
        const QList<QByteArray> dynamicProps = component->dynamicPropertyNames();
        for (const QByteArray &propName : dynamicProps) {
            const QString key = QString::fromUtf8(propName);
            if (key.isEmpty())
                continue;
            node.attributes.insert(key, component->property(propName.constData()));
        }

        m_nodesById.insert(node.id, node);
    }

    for (auto it = m_nodesById.constBegin(); it != m_nodesById.constEnd(); ++it)
        m_pendingInDegree.insert(it.key(), 0);

    const QList<ConnectionModel *> connections = m_graph->connectionList();
    for (ConnectionModel *connection : connections) {
        if (!connection)
            continue;

        EdgeSnapshot edge;
        edge.id = connection->id();
        edge.sourceId = connection->sourceId();
        edge.targetId = connection->targetId();
        edge.label = connection->label();

        if (!m_nodesById.contains(edge.sourceId) || !m_nodesById.contains(edge.targetId))
            continue;

        QList<EdgeSnapshot> &out = m_outgoingBySource[edge.sourceId];
        out.append(edge);
        m_pendingInDegree[edge.targetId] = m_pendingInDegree.value(edge.targetId, 0) + 1;
    }

    for (auto it = m_outgoingBySource.begin(); it != m_outgoingBySource.end(); ++it) {
        QList<EdgeSnapshot> &edges = it.value();
        std::sort(edges.begin(), edges.end(), [](const EdgeSnapshot &a, const EdgeSnapshot &b) {
            if (a.targetId != b.targetId)
                return a.targetId < b.targetId;
            return a.id < b.id;
        });
    }

    QStringList nodeIds = m_nodesById.keys();
    std::sort(nodeIds.begin(), nodeIds.end(), idComparator);
    for (const QString &nodeId : nodeIds) {
        if (m_pendingInDegree.value(nodeId, 0) == 0)
            enqueueReadyNode(nodeId);
    }

    return true;
}

bool GraphExecutionSandbox::executeOneStep(bool bypassBreakpoint)
{
    if (m_readyQueue.isEmpty()) {
        finalizeIfNoReadyNodes();
        return m_status != RunStatus::Error;
    }

    const QString componentId = m_readyQueue.first();
    if (!bypassBreakpoint && m_breakpoints.contains(componentId)) {
        appendTimelineEvent(QStringLiteral("breakpointHit"),
                            QVariantMap{
                                { QStringLiteral("componentId"), componentId },
                                { QStringLiteral("tick"), m_tick }
                            });
        setStatus(RunStatus::Paused);
        return true;
    }

    m_readyQueue.removeFirst();
    m_readyQueueSet.remove(componentId);
    const NodeSnapshot node = m_nodesById.value(componentId);
    QVariantMap trace;
    QVariantMap outputState = m_executionState;

    const IExecutionSemanticsProvider *provider = m_providerByComponentType.value(node.type, nullptr);
    if (provider) {
        QString error;
        if (!provider->executeComponent(node.type,
                                        node.id,
                                        toComponentSnapshotMap(node),
                                        m_executionState,
                                        &outputState,
                                        &trace,
                                        &error)) {
            markError(error.isEmpty() ? QStringLiteral("Execution semantics provider returned failure.")
                                      : error);
            return false;
        }
    } else {
        trace.insert(QStringLiteral("provider"), QStringLiteral("default"));
        trace.insert(QStringLiteral("note"), QStringLiteral("No execution semantics provider registered for component type."));
        outputState.insert(QStringLiteral("lastExecutedComponentId"), node.id);
    }

    m_executionState = outputState;
    emit executionStateChanged();

    QVariantMap state = m_componentStates.value(node.id).toMap();
    state.insert(QStringLiteral("status"), QStringLiteral("executed"));
    state.insert(QStringLiteral("tick"), m_tick);
    state.insert(QStringLiteral("type"), node.type);
    if (!trace.isEmpty())
        state.insert(QStringLiteral("trace"), trace);
    m_componentStates.insert(node.id, state);

    m_executed.insert(node.id);

    appendTimelineEvent(QStringLiteral("stepExecuted"),
                        QVariantMap{
                            { QStringLiteral("componentId"), node.id },
                            { QStringLiteral("componentType"), node.type },
                            { QStringLiteral("trace"), trace }
                        });

    const QList<EdgeSnapshot> outgoing = m_outgoingBySource.value(node.id);
    for (const EdgeSnapshot &edge : outgoing) {
        const QString targetId = edge.targetId;
        const int updatedInDegree = m_pendingInDegree.value(targetId, 0) - 1;
        m_pendingInDegree[targetId] = updatedInDegree;
        if (updatedInDegree == 0)
            enqueueReadyNode(targetId);
    }

    ++m_tick;
    emit currentTickChanged();
    finalizeIfNoReadyNodes();
    return true;
}

void GraphExecutionSandbox::finalizeIfNoReadyNodes()
{
    if (!m_readyQueue.isEmpty())
        return;

    if (m_executed.size() == m_nodesById.size()) {
        appendTimelineEvent(QStringLiteral("simulationCompleted"),
                            QVariantMap{
                                { QStringLiteral("executedCount"), m_executed.size() }
                            });
        setStatus(RunStatus::Completed);
        return;
    }

    if (m_status == RunStatus::Running || m_status == RunStatus::Paused) {
        appendTimelineEvent(QStringLiteral("simulationBlocked"),
                            QVariantMap{
                                { QStringLiteral("executedCount"), m_executed.size() },
                                { QStringLiteral("remainingCount"), m_nodesById.size() - m_executed.size() }
                            });
        setStatus(RunStatus::Completed);
    }
}

void GraphExecutionSandbox::enqueueReadyNode(const QString &componentId)
{
    if (componentId.isEmpty() || m_executed.contains(componentId) || m_readyQueueSet.contains(componentId))
        return;

    auto it = std::lower_bound(m_readyQueue.begin(), m_readyQueue.end(), componentId, idComparator);
    m_readyQueue.insert(it, componentId);
    m_readyQueueSet.insert(componentId);
}

QVariantMap GraphExecutionSandbox::toComponentSnapshotMap(const NodeSnapshot &node) const
{
    return node.attributes;
}
