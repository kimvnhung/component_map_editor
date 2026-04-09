#ifndef TRAVERSALENGINE_H
#define TRAVERSALENGINE_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QQmlEngine>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"

class TraversalEngine : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel *graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(int cacheRevision READ cacheRevision NOTIFY cacheRevisionChanged FINAL)

public:
    explicit TraversalEngine(QObject *parent = nullptr);

    GraphModel *graph() const;
    void setGraph(GraphModel *graph);

    int cacheRevision() const;

    Q_INVOKABLE QStringList bfs(const QVariantList &entryIds = {},
                                const QVariantMap &policy = {});
    Q_INVOKABLE QStringList dfs(const QVariantList &entryIds = {},
                                const QVariantMap &policy = {});
    Q_INVOKABLE QVariantMap topologicalTraversal(const QVariantMap &policy = {});
    Q_INVOKABLE QVariantList stronglyConnectedComponents(const QVariantMap &policy = {});
    Q_INVOKABLE bool pathExists(const QString &sourceId,
                                const QString &targetId,
                                const QVariantMap &policy = {});
    Q_INVOKABLE QStringList shortestPath(const QString &sourceId,
                                         const QString &targetId,
                                         const QVariantMap &policy = {});

    // Useful for tests and explicit refresh checkpoints.
    Q_INVOKABLE void refreshCache();

    int fullRebuildCount() const;
    int incrementalRecomputeCount() const;
    int cachedComponentCount() const;
    int cachedConnectionCount() const;
    int pendingDirtyComponentCount() const;
    int pendingDirtyConnectionCount() const;
    void markFullDirtyForTest();

signals:
    void graphChanged();
    void cacheRevisionChanged();

private:
    struct ConnectionInfo {
        QString id;
        QString sourceId;
        QString targetId;
        QString label;
    };

    struct RuntimePolicy {
        QSet<QString> entryIds;
        bool useZeroIndegreeEntries = false;
        QSet<QString> pruneComponentIds;
        QSet<QString> pruneComponentTypes;
        QSet<QString> allowedConnectionLabels;
        QSet<QString> blockedConnectionLabels;
        QHash<QString, double> componentTypeScores;
        QHash<QString, double> connectionLabelScores;
        int maxDepth = -1;
    };

    void connectGraphSignals();
    void disconnectGraphSignals();
    void trackComponent(ComponentModel *component);
    void untrackComponentById(const QString &id);
    void trackConnection(ConnectionModel *connection);
    void untrackConnectionById(const QString &id);

    void onComponentAdded(ComponentModel *component);
    void onComponentRemoved(const QString &id);
    void onConnectionAdded(ConnectionModel *connection);
    void onConnectionRemoved(const QString &id);

    void onTrackedComponentIdChanged(ComponentModel *component);
    void onTrackedComponentTypeChanged(ComponentModel *component);
    void onTrackedConnectionIdChanged(ConnectionModel *connection);
    void onTrackedConnectionChanged(ConnectionModel *connection);

    RuntimePolicy parsePolicy(const QVariantList &entryIds, const QVariantMap &policyMap) const;

    QStringList entryPointsForPolicy(const RuntimePolicy &policy) const;
    bool isPruned(const QString &componentId, const RuntimePolicy &policy) const;
    bool connectionAllowed(const ConnectionInfo &connection, const RuntimePolicy &policy) const;
    double connectionTraversalScore(const ConnectionInfo &connection, const RuntimePolicy &policy) const;
    double connectionTraversalCost(const ConnectionInfo &connection, const RuntimePolicy &policy) const;

    void ensureCacheReady();
    void rebuildCacheFull();
    void applyIncrementalDirtySet();

    void removeConnectionFromAdjacency(const ConnectionInfo &connection);
    void addConnectionToAdjacency(const ConnectionInfo &connection);

    QList<ConnectionInfo> outgoingConnections(const QString &componentId, const RuntimePolicy &policy) const;

    QPointer<GraphModel> m_graph;

    QHash<QString, QString> m_componentTypeById;
    QHash<QString, ConnectionInfo> m_connectionsById;
    QHash<QString, QList<QString>> m_outgoingConnectionIds;
    QHash<QString, int> m_inDegree;

    QHash<ComponentModel *, QString> m_trackedComponentIds;
    QHash<ConnectionModel *, QString> m_trackedConnectionIds;

    QSet<QString> m_dirtyComponentIds;
    QSet<QString> m_dirtyConnectionIds;

    bool m_fullDirty = true;
    int m_cacheRevision = 0;
    int m_fullRebuildCount = 0;
    int m_incrementalRecomputeCount = 0;
};

#endif // TRAVERSALENGINE_H
