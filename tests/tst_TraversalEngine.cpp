#include <QtTest>

#include <algorithm>
#include <memory>
#include <unistd.h>

#include <QFile>

#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/TraversalEngine.h"

namespace {

ComponentModel *makeComponent(GraphModel &graph, const QString &id, const QString &type)
{
    auto *component = new ComponentModel(&graph);
    component->setId(id);
    component->setType(type);
    component->setTitle(id);
    return component;
}

ConnectionModel *makeConnection(GraphModel &graph,
                                const QString &id,
                                const QString &sourceId,
                                const QString &targetId,
                                const QString &label = QString())
{
    auto *connection = new ConnectionModel(&graph);
    connection->setId(id);
    connection->setSourceId(sourceId);
    connection->setTargetId(targetId);
    connection->setLabel(label);
    return connection;
}

qint64 currentRssBytes()
{
    QFile file(QStringLiteral("/proc/self/statm"));
    if (!file.open(QIODevice::ReadOnly))
        return -1;

    const QByteArray text = file.readAll().trimmed();
    const QList<QByteArray> fields = text.split(' ');
    if (fields.size() < 2)
        return -1;

    bool ok = false;
    const qint64 residentPages = fields.at(1).toLongLong(&ok);
    if (!ok)
        return -1;

    const long pageSize = ::sysconf(_SC_PAGESIZE);
    if (pageSize <= 0)
        return -1;

    return residentPages * pageSize;
}

bool respectsTopologicalOrder(const QStringList &order,
                              const QList<QPair<QString, QString>> &edges)
{
    QHash<QString, int> index;
    for (int i = 0; i < order.size(); ++i)
        index.insert(order.at(i), i);

    for (const auto &edge : edges) {
        if (!index.contains(edge.first) || !index.contains(edge.second))
            return false;
        if (index.value(edge.first) >= index.value(edge.second))
            return false;
    }

    return true;
}

} // namespace

class tst_TraversalEngine : public QObject
{
    Q_OBJECT

private slots:
    void traversalCorrectnessOnAcyclicGraph();
    void traversalCorrectnessOnCyclicGraph();
    void policyHooksAffectTraversalAndPathSelection();
    void incrementalRecomputeFasterThanFullOnLocalEdit();
    void memoryGrowthBoundedDuringRepeatedRuns();
};

void tst_TraversalEngine::traversalCorrectnessOnAcyclicGraph()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("stop")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("A"), QStringLiteral("C")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("B"), QStringLiteral("D")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e4"), QStringLiteral("C"), QStringLiteral("D")));

    TraversalEngine engine;
    engine.setGraph(&graph);

    const QStringList bfsOrder = engine.bfs({QStringLiteral("A")});
    QCOMPARE(bfsOrder.first(), QStringLiteral("A"));
    QCOMPARE(bfsOrder.size(), 4);

    const QStringList dfsOrder = engine.dfs({QStringLiteral("A")});
    QCOMPARE(dfsOrder.first(), QStringLiteral("A"));
    QCOMPARE(dfsOrder.size(), 4);

    const QVariantMap topo = engine.topologicalTraversal();
    QVERIFY(topo.value(QStringLiteral("acyclic")).toBool());
    const QStringList topoOrder = topo.value(QStringLiteral("order")).toStringList();
    QCOMPARE(topoOrder.size(), 4);

    const QList<QPair<QString, QString>> edges = {
        {QStringLiteral("A"), QStringLiteral("B")},
        {QStringLiteral("A"), QStringLiteral("C")},
        {QStringLiteral("B"), QStringLiteral("D")},
        {QStringLiteral("C"), QStringLiteral("D")}
    };
    QVERIFY(respectsTopologicalOrder(topoOrder, edges));

    QVERIFY(engine.pathExists(QStringLiteral("A"), QStringLiteral("D")));
    QVERIFY(!engine.pathExists(QStringLiteral("D"), QStringLiteral("A")));
}

void tst_TraversalEngine::traversalCorrectnessOnCyclicGraph()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("process")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("B"), QStringLiteral("C")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("C"), QStringLiteral("A")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e4"), QStringLiteral("C"), QStringLiteral("D")));

    TraversalEngine engine;
    engine.setGraph(&graph);

    const QVariantMap topo = engine.topologicalTraversal();
    QVERIFY(!topo.value(QStringLiteral("acyclic")).toBool());

    const QVariantList scc = engine.stronglyConnectedComponents();
    bool foundCycleScc = false;
    for (const QVariant &value : scc) {
        int size = value.toList().size();
        if (size == 0)
            size = value.toStringList().size();
        if (size > 1) {
            foundCycleScc = true;
            break;
        }
    }
    QVERIFY(foundCycleScc);
}

void tst_TraversalEngine::policyHooksAffectTraversalAndPathSelection()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("start"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("fast"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("slow"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("stop"), QStringLiteral("stop")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("start"), QStringLiteral("fast"), QStringLiteral("fast")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("start"), QStringLiteral("slow"), QStringLiteral("slow")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("fast"), QStringLiteral("stop"), QStringLiteral("fast")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e4"), QStringLiteral("slow"), QStringLiteral("stop"), QStringLiteral("slow")));

    TraversalEngine engine;
    engine.setGraph(&graph);

    const QStringList defaultPath = engine.shortestPath(QStringLiteral("start"), QStringLiteral("stop"));
    QVERIFY(defaultPath.size() >= 3);

    QVariantMap policy;
    policy.insert(QStringLiteral("entryComponentIds"), QVariantList{QStringLiteral("start")});
    policy.insert(QStringLiteral("blockedEdgeLabels"), QVariantList{QStringLiteral("fast")});

    const QStringList bfsOrder = engine.bfs({}, policy);
    QVERIFY(!bfsOrder.contains(QStringLiteral("fast")));
    QVERIFY(bfsOrder.contains(QStringLiteral("slow")));

    QVariantMap scorePolicy;
    scorePolicy.insert(QStringLiteral("edgeLabelScores"), QVariantMap{{QStringLiteral("slow"), 0.9}});
    const QStringList preferredPath = engine.shortestPath(QStringLiteral("start"), QStringLiteral("stop"), scorePolicy);
    QVERIFY(preferredPath.contains(QStringLiteral("slow")));
}

void tst_TraversalEngine::incrementalRecomputeFasterThanFullOnLocalEdit()
{
    auto createGraph = []() {
        auto graph = std::make_unique<GraphModel>();
        const int componentCount = 420;

        graph->beginBatchUpdate();
        for (int i = 0; i < componentCount; ++i)
            graph->addComponent(makeComponent(*graph, QStringLiteral("n%1").arg(i), QStringLiteral("process")));

        int edgeCounter = 0;
        for (int i = 0; i < componentCount - 1; ++i) {
            graph->addConnection(makeConnection(*graph,
                                                QStringLiteral("e%1").arg(edgeCounter++),
                                                QStringLiteral("n%1").arg(i),
                                                QStringLiteral("n%1").arg(i + 1)));
            if (i + 2 < componentCount) {
                graph->addConnection(makeConnection(*graph,
                                                    QStringLiteral("e%1").arg(edgeCounter++),
                                                    QStringLiteral("n%1").arg(i),
                                                    QStringLiteral("n%1").arg(i + 2)));
            }
        }
        graph->endBatchUpdate();
        return graph;
    };

    auto runScenario = [&](bool forceFullRebuild) {
        auto graph = createGraph();
        TraversalEngine engine;
        engine.setGraph(graph.get());
        engine.refreshCache();

        auto *editedConnection = graph->connectionById(QStringLiteral("e100"));
        if (!editedConnection)
            return qint64(-1);

        const int iterations = 160;
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < iterations; ++i) {
            // Mutate a local edge attribute so both modes pay the same mutation cost.
            editedConnection->setLabel((i % 2 == 0) ? QStringLiteral("hot") : QStringLiteral("cold"));
            if (forceFullRebuild)
                engine.markFullDirtyForTest();
            engine.refreshCache();
        }

        return timer.nsecsElapsed();
    };

    const qint64 incrementalTotalNs = runScenario(false);
    const qint64 fullTotalNs = runScenario(true);

    QVERIFY(incrementalTotalNs > 0);
    QVERIFY(fullTotalNs > 0);

    QVERIFY2(incrementalTotalNs * 12 < fullTotalNs * 10,
             qPrintable(QStringLiteral("Expected incremental recompute to be faster on edited subgraphs (inc total=%1ns full total=%2ns)")
                            .arg(incrementalTotalNs)
                            .arg(fullTotalNs)));
}

void tst_TraversalEngine::memoryGrowthBoundedDuringRepeatedRuns()
{
    GraphModel graph;
    const int componentCount = 600;

    graph.beginBatchUpdate();
    for (int i = 0; i < componentCount; ++i)
        graph.addComponent(makeComponent(graph, QStringLiteral("m%1").arg(i), QStringLiteral("process")));

    int edgeCounter = 0;
    for (int i = 0; i < componentCount - 1; ++i) {
        graph.addConnection(makeConnection(graph,
                                           QStringLiteral("me%1").arg(edgeCounter++),
                                           QStringLiteral("m%1").arg(i),
                                           QStringLiteral("m%1").arg(i + 1),
                                           (i % 2 == 0) ? QStringLiteral("a") : QStringLiteral("b")));
    }
    graph.endBatchUpdate();

    TraversalEngine engine;
    engine.setGraph(&graph);
    engine.refreshCache();

    const int nodeCountBefore = engine.cachedNodeCount();
    const int edgeCountBefore = engine.cachedEdgeCount();
    const qint64 rssBefore = currentRssBytes();

    for (int i = 0; i < 300; ++i) {
        (void)engine.bfs({QStringLiteral("m0")});
        (void)engine.dfs({QStringLiteral("m0")});
        (void)engine.topologicalTraversal();
        (void)engine.stronglyConnectedComponents();
        if ((i % 10) == 0)
            (void)engine.pathExists(QStringLiteral("m0"), QStringLiteral("m599"));
    }

    const qint64 rssAfter = currentRssBytes();
    QCOMPARE(engine.cachedNodeCount(), nodeCountBefore);
    QCOMPARE(engine.cachedEdgeCount(), edgeCountBefore);
    QCOMPARE(engine.pendingDirtyNodeCount(), 0);
    QCOMPARE(engine.pendingDirtyConnectionCount(), 0);

    if (rssBefore > 0 && rssAfter > 0) {
        const qint64 delta = rssAfter - rssBefore;
        QVERIFY2(delta < (32ll * 1024ll * 1024ll),
                 qPrintable(QStringLiteral("RSS growth exceeded bound: %1 bytes").arg(delta)));
    }
}

QTEST_MAIN(tst_TraversalEngine)
#include "tst_TraversalEngine.moc"
