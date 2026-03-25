#include "TraversalEngine.h"

#include <QQueue>
#include <QStack>

#include <algorithm>
#include <limits>

namespace {

QStringList toStringList(const QVariantList &values)
{
    QStringList out;
    out.reserve(values.size());
    for (const QVariant &value : values) {
        const QString text = value.toString().trimmed();
        if (!text.isEmpty())
            out.append(text);
    }
    return out;
}

QSet<QString> toStringSet(const QVariantList &values)
{
    const QStringList list = toStringList(values);
    return QSet<QString>(list.begin(), list.end());
}

QHash<QString, double> toDoubleMap(const QVariantMap &map)
{
    QHash<QString, double> out;
    out.reserve(map.size());
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        out.insert(it.key(), it.value().toDouble());
    return out;
}

bool topoComparator(const QString &a, const QString &b)
{
    return a < b;
}

} // namespace

TraversalEngine::TraversalEngine(QObject *parent)
    : QObject(parent)
{}

GraphModel *TraversalEngine::graph() const
{
    return m_graph;
}

void TraversalEngine::setGraph(GraphModel *graph)
{
    if (m_graph == graph)
        return;

    disconnectGraphSignals();
    m_graph = graph;

    m_componentTypeById.clear();
    m_edgesById.clear();
    m_outgoingEdgeIds.clear();
    m_inDegree.clear();
    m_trackedComponentIds.clear();
    m_trackedConnectionIds.clear();
    m_dirtyComponentIds.clear();
    m_dirtyConnectionIds.clear();
    m_fullDirty = true;

    connectGraphSignals();

    emit graphChanged();
}

int TraversalEngine::cacheRevision() const
{
    return m_cacheRevision;
}

QStringList TraversalEngine::bfs(const QVariantList &entryIds, const QVariantMap &policyMap)
{
    ensureCacheReady();
    const RuntimePolicy policy = parsePolicy(entryIds, policyMap);

    QStringList order;
    QSet<QString> visited;
    QQueue<QPair<QString, int>> queue;

    const QStringList entries = entryPointsForPolicy(policy);
    for (const QString &entry : entries) {
        if (isPruned(entry, policy))
            continue;
        if (visited.contains(entry))
            continue;
        visited.insert(entry);
        queue.enqueue({entry, 0});
    }

    while (!queue.isEmpty()) {
        const auto item = queue.dequeue();
        const QString current = item.first;
        const int depth = item.second;

        order.append(current);

        if (policy.maxDepth >= 0 && depth >= policy.maxDepth)
            continue;

        const QList<EdgeInfo> edges = outgoingEdges(current, policy);
        for (const EdgeInfo &edge : edges) {
            const QString next = edge.targetId;
            if (isPruned(next, policy))
                continue;
            if (visited.contains(next))
                continue;
            visited.insert(next);
            queue.enqueue({next, depth + 1});
        }
    }

    return order;
}

QStringList TraversalEngine::dfs(const QVariantList &entryIds, const QVariantMap &policyMap)
{
    ensureCacheReady();
    const RuntimePolicy policy = parsePolicy(entryIds, policyMap);

    QStringList order;
    QSet<QString> visited;
    QStack<QPair<QString, int>> stack;

    const QStringList entries = entryPointsForPolicy(policy);
    for (int i = entries.size() - 1; i >= 0; --i) {
        const QString &entry = entries.at(i);
        if (isPruned(entry, policy))
            continue;
        if (visited.contains(entry))
            continue;
        stack.push({entry, 0});
    }

    while (!stack.isEmpty()) {
        const auto item = stack.pop();
        const QString current = item.first;
        const int depth = item.second;

        if (visited.contains(current))
            continue;
        visited.insert(current);
        order.append(current);

        if (policy.maxDepth >= 0 && depth >= policy.maxDepth)
            continue;

        const QList<EdgeInfo> edges = outgoingEdges(current, policy);
        for (int i = edges.size() - 1; i >= 0; --i) {
            const QString next = edges.at(i).targetId;
            if (isPruned(next, policy))
                continue;
            if (!visited.contains(next))
                stack.push({next, depth + 1});
        }
    }

    return order;
}

QVariantMap TraversalEngine::topologicalTraversal(const QVariantMap &policyMap)
{
    ensureCacheReady();
    const RuntimePolicy policy = parsePolicy({}, policyMap);

    QHash<QString, int> indegree;
    QStringList nodes;
    nodes.reserve(m_componentTypeById.size());

    for (auto it = m_componentTypeById.constBegin(); it != m_componentTypeById.constEnd(); ++it) {
        const QString id = it.key();
        if (isPruned(id, policy))
            continue;
        nodes.append(id);
        indegree.insert(id, 0);
    }

    for (auto it = m_edgesById.constBegin(); it != m_edgesById.constEnd(); ++it) {
        const EdgeInfo &edge = it.value();
        if (!indegree.contains(edge.sourceId) || !indegree.contains(edge.targetId))
            continue;
        if (!edgeAllowed(edge, policy))
            continue;
        indegree[edge.targetId] += 1;
    }

    QStringList zero;
    zero.reserve(nodes.size());
    for (const QString &id : std::as_const(nodes)) {
        if (indegree.value(id) == 0)
            zero.append(id);
    }
    std::sort(zero.begin(), zero.end(), topoComparator);

    QStringList order;
    while (!zero.isEmpty()) {
        const QString current = zero.takeFirst();
        order.append(current);

        const QList<EdgeInfo> edges = outgoingEdges(current, policy);
        for (const EdgeInfo &edge : edges) {
            if (!indegree.contains(edge.targetId))
                continue;
            indegree[edge.targetId] -= 1;
            if (indegree.value(edge.targetId) == 0) {
                zero.append(edge.targetId);
                std::sort(zero.begin(), zero.end(), topoComparator);
            }
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("order"), order);
    result.insert(QStringLiteral("acyclic"), order.size() == nodes.size());
    result.insert(QStringLiteral("visitedCount"), order.size());
    result.insert(QStringLiteral("nodeCount"), nodes.size());
    return result;
}

QVariantList TraversalEngine::stronglyConnectedComponents(const QVariantMap &policyMap)
{
    ensureCacheReady();
    const RuntimePolicy policy = parsePolicy({}, policyMap);

    QHash<QString, int> indexByNode;
    QHash<QString, int> lowlinkByNode;
    QSet<QString> onStack;
    QStringList stack;
    QVariantList components;
    int index = 0;

    std::function<void(const QString &)> strongConnect = [&](const QString &node) {
        indexByNode.insert(node, index);
        lowlinkByNode.insert(node, index);
        ++index;
        stack.append(node);
        onStack.insert(node);

        const QList<EdgeInfo> edges = outgoingEdges(node, policy);
        for (const EdgeInfo &edge : edges) {
            const QString next = edge.targetId;
            if (isPruned(next, policy))
                continue;
            if (!indexByNode.contains(next)) {
                strongConnect(next);
                lowlinkByNode[node] = qMin(lowlinkByNode[node], lowlinkByNode[next]);
            } else if (onStack.contains(next)) {
                lowlinkByNode[node] = qMin(lowlinkByNode[node], indexByNode[next]);
            }
        }

        if (lowlinkByNode.value(node) == indexByNode.value(node)) {
            QVariantList component;
            while (!stack.isEmpty()) {
                const QString top = stack.takeLast();
                onStack.remove(top);
                component.append(top);
                if (top == node)
                    break;
            }
            components.append(QVariant(component));
        }
    };

    QStringList nodes = m_componentTypeById.keys();
    std::sort(nodes.begin(), nodes.end(), topoComparator);
    for (const QString &node : std::as_const(nodes)) {
        if (isPruned(node, policy))
            continue;
        if (!indexByNode.contains(node))
            strongConnect(node);
    }

    return components;
}

bool TraversalEngine::pathExists(const QString &sourceId,
                                 const QString &targetId,
                                 const QVariantMap &policyMap)
{
    return !shortestPath(sourceId, targetId, policyMap).isEmpty();
}

QStringList TraversalEngine::shortestPath(const QString &sourceId,
                                          const QString &targetId,
                                          const QVariantMap &policyMap)
{
    ensureCacheReady();
    const RuntimePolicy policy = parsePolicy({}, policyMap);

    if (sourceId.isEmpty() || targetId.isEmpty())
        return {};
    if (isPruned(sourceId, policy) || isPruned(targetId, policy))
        return {};
    if (!m_componentTypeById.contains(sourceId) || !m_componentTypeById.contains(targetId))
        return {};

    QHash<QString, double> dist;
    QHash<QString, QString> prev;
    QSet<QString> unvisited;

    const QStringList allNodes = m_componentTypeById.keys();
    for (const QString &node : allNodes) {
        if (isPruned(node, policy))
            continue;
        dist.insert(node, std::numeric_limits<double>::infinity());
        unvisited.insert(node);
    }

    if (!dist.contains(sourceId) || !dist.contains(targetId))
        return {};

    dist[sourceId] = 0.0;

    while (!unvisited.isEmpty()) {
        QString current;
        double best = std::numeric_limits<double>::infinity();
        for (const QString &node : std::as_const(unvisited)) {
            const double value = dist.value(node);
            if (value < best) {
                best = value;
                current = node;
            }
        }

        if (current.isEmpty() || std::isinf(best))
            break;

        unvisited.remove(current);
        if (current == targetId)
            break;

        const QList<EdgeInfo> edges = outgoingEdges(current, policy);
        for (const EdgeInfo &edge : edges) {
            const QString next = edge.targetId;
            if (!unvisited.contains(next))
                continue;

            const double nextDist = dist.value(current) + edgeTraversalCost(edge, policy);
            if (nextDist < dist.value(next)) {
                dist[next] = nextDist;
                prev[next] = current;
            }
        }
    }

    if (!prev.contains(targetId) && sourceId != targetId)
        return {};

    QStringList path;
    QString current = targetId;
    path.prepend(current);
    while (current != sourceId) {
        if (!prev.contains(current))
            return {};
        current = prev.value(current);
        path.prepend(current);
    }

    return path;
}

void TraversalEngine::refreshCache()
{
    ensureCacheReady();
}

int TraversalEngine::fullRebuildCount() const
{
    return m_fullRebuildCount;
}

int TraversalEngine::incrementalRecomputeCount() const
{
    return m_incrementalRecomputeCount;
}

int TraversalEngine::cachedNodeCount() const
{
    return m_componentTypeById.size();
}

int TraversalEngine::cachedEdgeCount() const
{
    return m_edgesById.size();
}

int TraversalEngine::pendingDirtyNodeCount() const
{
    return m_dirtyComponentIds.size();
}

int TraversalEngine::pendingDirtyConnectionCount() const
{
    return m_dirtyConnectionIds.size();
}

void TraversalEngine::markFullDirtyForTest()
{
    m_fullDirty = true;
}

void TraversalEngine::connectGraphSignals()
{
    if (!m_graph)
        return;

    connect(m_graph, &GraphModel::componentAdded, this, [this](ComponentModel *component) {
        onComponentAdded(component);
    });
    connect(m_graph, &GraphModel::componentRemoved, this, [this](const QString &id) {
        onComponentRemoved(id);
    });
    connect(m_graph, &GraphModel::connectionAdded, this, [this](ConnectionModel *connection) {
        onConnectionAdded(connection);
    });
    connect(m_graph, &GraphModel::connectionRemoved, this, [this](const QString &id) {
        onConnectionRemoved(id);
    });

    const QList<ComponentModel *> components = m_graph->componentList();
    for (ComponentModel *component : components)
        trackComponent(component);

    const QList<ConnectionModel *> connections = m_graph->connectionList();
    for (ConnectionModel *connection : connections)
        trackConnection(connection);
}

void TraversalEngine::disconnectGraphSignals()
{
    if (!m_graph)
        return;
    disconnect(m_graph, nullptr, this, nullptr);

    for (auto it = m_trackedComponentIds.constBegin(); it != m_trackedComponentIds.constEnd(); ++it)
        disconnect(it.key(), nullptr, this, nullptr);
    for (auto it = m_trackedConnectionIds.constBegin(); it != m_trackedConnectionIds.constEnd(); ++it)
        disconnect(it.key(), nullptr, this, nullptr);
}

void TraversalEngine::trackComponent(ComponentModel *component)
{
    if (!component)
        return;

    m_trackedComponentIds.insert(component, component->id());

    connect(component, &ComponentModel::idChanged, this, [this, component]() {
        onTrackedComponentIdChanged(component);
    });
    connect(component, &ComponentModel::typeChanged, this, [this, component]() {
        onTrackedComponentTypeChanged(component);
    });
}

void TraversalEngine::untrackComponentById(const QString &id)
{
    ComponentModel *found = nullptr;
    for (auto it = m_trackedComponentIds.begin(); it != m_trackedComponentIds.end(); ++it) {
        if (it.value() == id) {
            found = it.key();
            break;
        }
    }

    if (!found)
        return;

    disconnect(found, nullptr, this, nullptr);
    m_trackedComponentIds.remove(found);
}

void TraversalEngine::trackConnection(ConnectionModel *connection)
{
    if (!connection)
        return;

    m_trackedConnectionIds.insert(connection, connection->id());

    connect(connection, &ConnectionModel::idChanged, this, [this, connection]() {
        onTrackedConnectionIdChanged(connection);
    });
    connect(connection, &ConnectionModel::sourceIdChanged, this, [this, connection]() {
        onTrackedConnectionChanged(connection);
    });
    connect(connection, &ConnectionModel::targetIdChanged, this, [this, connection]() {
        onTrackedConnectionChanged(connection);
    });
    connect(connection, &ConnectionModel::labelChanged, this, [this, connection]() {
        onTrackedConnectionChanged(connection);
    });
}

void TraversalEngine::untrackConnectionById(const QString &id)
{
    ConnectionModel *found = nullptr;
    for (auto it = m_trackedConnectionIds.begin(); it != m_trackedConnectionIds.end(); ++it) {
        if (it.value() == id) {
            found = it.key();
            break;
        }
    }

    if (!found)
        return;

    disconnect(found, nullptr, this, nullptr);
    m_trackedConnectionIds.remove(found);
}

void TraversalEngine::onComponentAdded(ComponentModel *component)
{
    trackComponent(component);
    if (component)
        m_dirtyComponentIds.insert(component->id());
}

void TraversalEngine::onComponentRemoved(const QString &id)
{
    untrackComponentById(id);
    m_dirtyComponentIds.insert(id);
}

void TraversalEngine::onConnectionAdded(ConnectionModel *connection)
{
    trackConnection(connection);
    if (connection)
        m_dirtyConnectionIds.insert(connection->id());
}

void TraversalEngine::onConnectionRemoved(const QString &id)
{
    untrackConnectionById(id);
    m_dirtyConnectionIds.insert(id);
}

void TraversalEngine::onTrackedComponentIdChanged(ComponentModel *component)
{
    Q_UNUSED(component)
    // Re-keying component IDs requires rewriting adjacency and edge endpoints.
    m_fullDirty = true;
}

void TraversalEngine::onTrackedComponentTypeChanged(ComponentModel *component)
{
    if (!component)
        return;
    m_dirtyComponentIds.insert(component->id());
}

void TraversalEngine::onTrackedConnectionIdChanged(ConnectionModel *connection)
{
    Q_UNUSED(connection)
    // Re-keying connection IDs requires rewriting edge-index maps.
    m_fullDirty = true;
}

void TraversalEngine::onTrackedConnectionChanged(ConnectionModel *connection)
{
    if (!connection)
        return;
    const QString id = m_trackedConnectionIds.value(connection, connection->id());
    if (!id.isEmpty())
        m_dirtyConnectionIds.insert(id);
}

TraversalEngine::RuntimePolicy TraversalEngine::parsePolicy(const QVariantList &entryIds,
                                                            const QVariantMap &policyMap) const
{
    RuntimePolicy policy;

    if (!entryIds.isEmpty())
        policy.entryIds = toStringSet(entryIds);

    if (!policyMap.isEmpty()) {
        const QString entryMode = policyMap.value(QStringLiteral("entryMode")).toString();
        if (entryMode == QStringLiteral("zeroInDegree"))
            policy.useZeroIndegreeEntries = true;

        if (policy.entryIds.isEmpty()) {
            const QVariantList explicitEntries = policyMap.value(QStringLiteral("entryComponentIds")).toList();
            if (!explicitEntries.isEmpty())
                policy.entryIds = toStringSet(explicitEntries);
        }

        policy.pruneComponentIds = toStringSet(policyMap.value(QStringLiteral("pruneComponentIds")).toList());
        policy.pruneComponentTypes = toStringSet(policyMap.value(QStringLiteral("pruneComponentTypes")).toList());
        policy.allowedEdgeLabels = toStringSet(policyMap.value(QStringLiteral("allowedEdgeLabels")).toList());
        policy.blockedEdgeLabels = toStringSet(policyMap.value(QStringLiteral("blockedEdgeLabels")).toList());
        policy.componentTypeScores = toDoubleMap(policyMap.value(QStringLiteral("componentTypeScores")).toMap());
        policy.edgeLabelScores = toDoubleMap(policyMap.value(QStringLiteral("edgeLabelScores")).toMap());
        policy.maxDepth = policyMap.value(QStringLiteral("maxDepth"), -1).toInt();
    }

    return policy;
}

QStringList TraversalEngine::entryPointsForPolicy(const RuntimePolicy &policy) const
{
    if (!policy.entryIds.isEmpty()) {
        QStringList entries = policy.entryIds.values();
        std::sort(entries.begin(), entries.end(), topoComparator);
        return entries;
    }

    QStringList entries;
    if (policy.useZeroIndegreeEntries) {
        for (auto it = m_componentTypeById.constBegin(); it != m_componentTypeById.constEnd(); ++it) {
            const QString id = it.key();
            if (m_inDegree.value(id, 0) == 0)
                entries.append(id);
        }
    } else {
        entries = m_componentTypeById.keys();
    }

    std::sort(entries.begin(), entries.end(), topoComparator);
    return entries;
}

bool TraversalEngine::isPruned(const QString &componentId, const RuntimePolicy &policy) const
{
    if (policy.pruneComponentIds.contains(componentId))
        return true;

    const QString type = m_componentTypeById.value(componentId);
    if (!type.isEmpty() && policy.pruneComponentTypes.contains(type))
        return true;

    return false;
}

bool TraversalEngine::edgeAllowed(const EdgeInfo &edge, const RuntimePolicy &policy) const
{
    if (!policy.allowedEdgeLabels.isEmpty() && !policy.allowedEdgeLabels.contains(edge.label))
        return false;
    if (policy.blockedEdgeLabels.contains(edge.label))
        return false;
    return true;
}

double TraversalEngine::edgeTraversalScore(const EdgeInfo &edge, const RuntimePolicy &policy) const
{
    double score = 0.0;
    const QString targetType = m_componentTypeById.value(edge.targetId);
    score += policy.componentTypeScores.value(targetType, 0.0);
    score += policy.edgeLabelScores.value(edge.label, 0.0);
    return score;
}

double TraversalEngine::edgeTraversalCost(const EdgeInfo &edge, const RuntimePolicy &policy) const
{
    const double raw = 1.0 - edgeTraversalScore(edge, policy);
    return qMax(0.001, raw);
}

void TraversalEngine::ensureCacheReady()
{
    if (!m_graph)
        return;

    if (!m_fullDirty) {
        if (m_componentTypeById.size() != m_graph->componentCount() ||
            m_edgesById.size() != m_graph->connectionCount()) {
            m_fullDirty = true;
        }
    }

    if (m_fullDirty) {
        rebuildCacheFull();
        return;
    }

    if (!m_dirtyComponentIds.isEmpty() || !m_dirtyConnectionIds.isEmpty())
        applyIncrementalDirtySet();
}

void TraversalEngine::rebuildCacheFull()
{
    if (!m_graph)
        return;

    m_componentTypeById.clear();
    m_edgesById.clear();
    m_outgoingEdgeIds.clear();
    m_inDegree.clear();

    const QList<ComponentModel *> components = m_graph->componentList();
    for (ComponentModel *component : components) {
        if (!component)
            continue;
        const QString id = component->id();
        if (id.isEmpty())
            continue;
        m_componentTypeById.insert(id, component->type());
        m_inDegree.insert(id, 0);
    }

    const QList<ConnectionModel *> connections = m_graph->connectionList();
    for (ConnectionModel *connection : connections) {
        if (!connection)
            continue;

        EdgeInfo edge;
        edge.id = connection->id();
        edge.sourceId = connection->sourceId();
        edge.targetId = connection->targetId();
        edge.label = connection->label();

        if (edge.id.isEmpty())
            continue;

        m_edgesById.insert(edge.id, edge);
        addEdgeToAdjacency(edge);
    }

    m_dirtyComponentIds.clear();
    m_dirtyConnectionIds.clear();
    m_fullDirty = false;
    ++m_cacheRevision;
    ++m_fullRebuildCount;
    emit cacheRevisionChanged();
}

void TraversalEngine::applyIncrementalDirtySet()
{
    if (!m_graph)
        return;

    for (const QString &componentId : std::as_const(m_dirtyComponentIds)) {
        if (componentId.isEmpty())
            continue;

        ComponentModel *component = m_graph->componentById(componentId);
        if (!component) {
            m_componentTypeById.remove(componentId);
            m_inDegree.remove(componentId);
            m_outgoingEdgeIds.remove(componentId);

            for (auto it = m_outgoingEdgeIds.begin(); it != m_outgoingEdgeIds.end(); ++it) {
                QList<QString> &edgeIds = it.value();
                edgeIds.erase(std::remove_if(edgeIds.begin(),
                                             edgeIds.end(),
                                             [&](const QString &edgeId) {
                                                 const EdgeInfo edge = m_edgesById.value(edgeId);
                                                 return edge.targetId == componentId;
                                             }),
                              edgeIds.end());
            }

            QList<QString> removeEdges;
            for (auto edgeIt = m_edgesById.constBegin(); edgeIt != m_edgesById.constEnd(); ++edgeIt) {
                if (edgeIt->sourceId == componentId || edgeIt->targetId == componentId)
                    removeEdges.append(edgeIt.key());
            }
            for (const QString &edgeId : std::as_const(removeEdges)) {
                const EdgeInfo edge = m_edgesById.take(edgeId);
                removeEdgeFromAdjacency(edge);
            }
        } else {
            m_componentTypeById.insert(componentId, component->type());
            m_inDegree.insert(componentId, m_inDegree.value(componentId, 0));
            m_outgoingEdgeIds.insert(componentId, m_outgoingEdgeIds.value(componentId));
        }
    }

    for (const QString &connectionId : std::as_const(m_dirtyConnectionIds)) {
        if (connectionId.isEmpty())
            continue;

        if (m_edgesById.contains(connectionId)) {
            const EdgeInfo oldEdge = m_edgesById.take(connectionId);
            removeEdgeFromAdjacency(oldEdge);
        }

        ConnectionModel *connection = m_graph->connectionById(connectionId);
        if (!connection)
            continue;

        EdgeInfo edge;
        edge.id = connectionId;
        edge.sourceId = connection->sourceId();
        edge.targetId = connection->targetId();
        edge.label = connection->label();

        m_edgesById.insert(connectionId, edge);
        addEdgeToAdjacency(edge);
    }

    m_dirtyComponentIds.clear();
    m_dirtyConnectionIds.clear();

    ++m_cacheRevision;
    ++m_incrementalRecomputeCount;
    emit cacheRevisionChanged();
}

void TraversalEngine::removeEdgeFromAdjacency(const EdgeInfo &edge)
{
    if (edge.sourceId.isEmpty() || edge.targetId.isEmpty())
        return;

    QList<QString> &out = m_outgoingEdgeIds[edge.sourceId];
    out.erase(std::remove(out.begin(), out.end(), edge.id), out.end());

    if (m_inDegree.contains(edge.targetId))
        m_inDegree[edge.targetId] = qMax(0, m_inDegree.value(edge.targetId) - 1);
}

void TraversalEngine::addEdgeToAdjacency(const EdgeInfo &edge)
{
    if (edge.sourceId.isEmpty() || edge.targetId.isEmpty())
        return;
    if (!m_componentTypeById.contains(edge.sourceId) || !m_componentTypeById.contains(edge.targetId))
        return;

    QList<QString> &out = m_outgoingEdgeIds[edge.sourceId];
    if (!out.contains(edge.id))
        out.append(edge.id);

    m_inDegree[edge.targetId] = m_inDegree.value(edge.targetId, 0) + 1;
}

QList<TraversalEngine::EdgeInfo> TraversalEngine::outgoingEdges(const QString &componentId,
                                                                const RuntimePolicy &policy) const
{
    QList<EdgeInfo> edges;
    const QList<QString> edgeIds = m_outgoingEdgeIds.value(componentId);
    edges.reserve(edgeIds.size());

    for (const QString &edgeId : edgeIds) {
        const EdgeInfo edge = m_edgesById.value(edgeId);
        if (edge.id.isEmpty())
            continue;
        if (!edgeAllowed(edge, policy))
            continue;
        edges.append(edge);
    }

    std::sort(edges.begin(), edges.end(), [&](const EdgeInfo &a, const EdgeInfo &b) {
        const double sa = edgeTraversalScore(a, policy);
        const double sb = edgeTraversalScore(b, policy);
        if (!qFuzzyCompare(sa + 1.0, sb + 1.0))
            return sa > sb;
        if (a.targetId != b.targetId)
            return a.targetId < b.targetId;
        return a.id < b.id;
    });

    return edges;
}
