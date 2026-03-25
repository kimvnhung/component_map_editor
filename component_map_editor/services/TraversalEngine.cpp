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
    m_connectionsById.clear();
    m_outgoingConnectionIds.clear();
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

        const QList<ConnectionInfo> conns = outgoingConnections(current, policy);
        for (const ConnectionInfo &conn : conns) {
            const QString next = conn.targetId;
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

        const QList<ConnectionInfo> conns = outgoingConnections(current, policy);
        for (int i = conns.size() - 1; i >= 0; --i) {
            const QString next = conns.at(i).targetId;
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

    for (auto it = m_connectionsById.constBegin(); it != m_connectionsById.constEnd(); ++it) {
        const ConnectionInfo &conn = it.value();
        if (!indegree.contains(conn.sourceId) || !indegree.contains(conn.targetId))
            continue;
        if (!connectionAllowed(conn, policy))
            continue;
        indegree[conn.targetId] += 1;
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

        const QList<ConnectionInfo> conns = outgoingConnections(current, policy);
        for (const ConnectionInfo &conn : conns) {
            if (!indegree.contains(conn.targetId))
                continue;
            indegree[conn.targetId] -= 1;
            if (indegree.value(conn.targetId) == 0) {
                zero.append(conn.targetId);
                std::sort(zero.begin(), zero.end(), topoComparator);
            }
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("order"), order);
    result.insert(QStringLiteral("acyclic"), order.size() == nodes.size());
    result.insert(QStringLiteral("visitedCount"), order.size());
    result.insert(QStringLiteral("componentCount"), nodes.size());
    return result;
}

QVariantList TraversalEngine::stronglyConnectedComponents(const QVariantMap &policyMap)
{
    ensureCacheReady();
    const RuntimePolicy policy = parsePolicy({}, policyMap);

    QHash<QString, int> indexByComponent;
    QHash<QString, int> lowlinkByComponent;
    QSet<QString> onStack;
    QStringList stack;
    QVariantList components;
    int index = 0;

    std::function<void(const QString &)> strongConnect = [&](const QString &node) {
        indexByComponent.insert(node, index);
        lowlinkByComponent.insert(node, index);
        ++index;
        stack.append(node);
        onStack.insert(node);

        const QList<ConnectionInfo> conns = outgoingConnections(node, policy);
        for (const ConnectionInfo &conn : conns) {
            const QString next = conn.targetId;
            if (isPruned(next, policy))
                continue;
            if (!indexByComponent.contains(next)) {
                strongConnect(next);
                lowlinkByComponent[node] = qMin(lowlinkByComponent[node], lowlinkByComponent[next]);
            } else if (onStack.contains(next)) {
                lowlinkByComponent[node] = qMin(lowlinkByComponent[node], indexByComponent[next]);
            }
        }

        if (lowlinkByComponent.value(node) == indexByComponent.value(node)) {
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
        if (!indexByComponent.contains(node))
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

    const QStringList allComponents = m_componentTypeById.keys();
    for (const QString &node : allComponents) {
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

        const QList<ConnectionInfo> conns = outgoingConnections(current, policy);
        for (const ConnectionInfo &conn : conns) {
            const QString next = conn.targetId;
            if (!unvisited.contains(next))
                continue;

            const double nextDist = dist.value(current) + connectionTraversalCost(conn, policy);
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

int TraversalEngine::cachedComponentCount() const
{
    return m_componentTypeById.size();
}

int TraversalEngine::cachedConnectionCount() const
{
    return m_connectionsById.size();
}

int TraversalEngine::pendingDirtyComponentCount() const
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
        policy.allowedConnectionLabels = toStringSet(policyMap.value(QStringLiteral("allowedConnectionLabels")).toList());
        policy.blockedConnectionLabels = toStringSet(policyMap.value(QStringLiteral("blockedConnectionLabels")).toList());
        policy.componentTypeScores = toDoubleMap(policyMap.value(QStringLiteral("componentTypeScores")).toMap());
        policy.connectionLabelScores = toDoubleMap(policyMap.value(QStringLiteral("connectionLabelScores")).toMap());
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

bool TraversalEngine::connectionAllowed(const ConnectionInfo &connection, const RuntimePolicy &policy) const
{
    if (!policy.allowedConnectionLabels.isEmpty() && !policy.allowedConnectionLabels.contains(connection.label))
        return false;
    if (policy.blockedConnectionLabels.contains(connection.label))
        return false;
    return true;
}

double TraversalEngine::connectionTraversalScore(const ConnectionInfo &connection, const RuntimePolicy &policy) const
{
    double score = 0.0;
    const QString targetType = m_componentTypeById.value(connection.targetId);
    score += policy.componentTypeScores.value(targetType, 0.0);
    score += policy.connectionLabelScores.value(connection.label, 0.0);
    return score;
}

double TraversalEngine::connectionTraversalCost(const ConnectionInfo &connection, const RuntimePolicy &policy) const
{
    const double raw = 1.0 - connectionTraversalScore(connection, policy);
    return qMax(0.001, raw);
}

void TraversalEngine::ensureCacheReady()
{
    if (!m_graph)
        return;

    if (!m_fullDirty) {
        if (m_componentTypeById.size() != m_graph->componentCount() ||
            m_connectionsById.size() != m_graph->connectionCount()) {
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
    m_connectionsById.clear();
    m_outgoingConnectionIds.clear();
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

        ConnectionInfo conn;
        conn.id = connection->id();
        conn.sourceId = connection->sourceId();
        conn.targetId = connection->targetId();
        conn.label = connection->label();

        if (conn.id.isEmpty())
            continue;

        m_connectionsById.insert(conn.id, conn);
        addConnectionToAdjacency(conn);
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
            m_outgoingConnectionIds.remove(componentId);

            for (auto it = m_outgoingConnectionIds.begin(); it != m_outgoingConnectionIds.end(); ++it) {
                QList<QString> &connIds = it.value();
                connIds.erase(std::remove_if(connIds.begin(),
                                             connIds.end(),
                                             [&](const QString &connId) {
                                                 const ConnectionInfo conn = m_connectionsById.value(connId);
                                                 return conn.targetId == componentId;
                                             }),
                              connIds.end());
            }

            QList<QString> removeConns;
            for (auto connIt = m_connectionsById.constBegin(); connIt != m_connectionsById.constEnd(); ++connIt) {
                if (connIt->sourceId == componentId || connIt->targetId == componentId)
                    removeConns.append(connIt.key());
            }
            for (const QString &connId : std::as_const(removeConns)) {
                const ConnectionInfo conn = m_connectionsById.take(connId);
                removeConnectionFromAdjacency(conn);
            }
        } else {
            m_componentTypeById.insert(componentId, component->type());
            m_inDegree.insert(componentId, m_inDegree.value(componentId, 0));
                m_outgoingConnectionIds.insert(componentId, m_outgoingConnectionIds.value(componentId));
        }
    }

    for (const QString &connectionId : std::as_const(m_dirtyConnectionIds)) {
        if (connectionId.isEmpty())
            continue;

        if (m_connectionsById.contains(connectionId)) {
            const ConnectionInfo oldConn = m_connectionsById.take(connectionId);
            removeConnectionFromAdjacency(oldConn);
        }

        ConnectionModel *connection = m_graph->connectionById(connectionId);
        if (!connection)
            continue;

        ConnectionInfo conn;
        conn.id = connectionId;
        conn.sourceId = connection->sourceId();
        conn.targetId = connection->targetId();
        conn.label = connection->label();

        m_connectionsById.insert(connectionId, conn);
        addConnectionToAdjacency(conn);
    }

    m_dirtyComponentIds.clear();
    m_dirtyConnectionIds.clear();

    ++m_cacheRevision;
    ++m_incrementalRecomputeCount;
    emit cacheRevisionChanged();
}

void TraversalEngine::removeConnectionFromAdjacency(const ConnectionInfo &connection)
{
    if (connection.sourceId.isEmpty() || connection.targetId.isEmpty())
        return;

    QList<QString> &out = m_outgoingConnectionIds[connection.sourceId];
    out.erase(std::remove(out.begin(), out.end(), connection.id), out.end());

    if (m_inDegree.contains(connection.targetId))
        m_inDegree[connection.targetId] = qMax(0, m_inDegree.value(connection.targetId) - 1);
}

void TraversalEngine::addConnectionToAdjacency(const ConnectionInfo &connection)
{
    if (connection.sourceId.isEmpty() || connection.targetId.isEmpty())
        return;
    if (!m_componentTypeById.contains(connection.sourceId) || !m_componentTypeById.contains(connection.targetId))
        return;

    QList<QString> &out = m_outgoingConnectionIds[connection.sourceId];
    if (!out.contains(connection.id))
        out.append(connection.id);

    m_inDegree[connection.targetId] = m_inDegree.value(connection.targetId, 0) + 1;
}

QList<TraversalEngine::ConnectionInfo> TraversalEngine::outgoingConnections(const QString &componentId,
                                                                             const RuntimePolicy &policy) const
{
    QList<ConnectionInfo> conns;
    const QList<QString> connIds = m_outgoingConnectionIds.value(componentId);
    conns.reserve(connIds.size());

    for (const QString &connId : connIds) {
        const ConnectionInfo conn = m_connectionsById.value(connId);
        if (conn.id.isEmpty())
            continue;
        if (!connectionAllowed(conn, policy))
            continue;
        conns.append(conn);
    }

    std::sort(conns.begin(), conns.end(), [&](const ConnectionInfo &a, const ConnectionInfo &b) {
        const double sa = connectionTraversalScore(a, policy);
        const double sb = connectionTraversalScore(b, policy);
        if (!qFuzzyCompare(sa + 1.0, sb + 1.0))
            return sa > sb;
        if (a.targetId != b.targetId)
            return a.targetId < b.targetId;
        return a.id < b.id;
    });

    return conns;
}
