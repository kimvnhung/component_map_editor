#include <QtTest>

#include "commands/UndoStack.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/ExecutionMigrationFlags.h"
#include "services/GraphExecutionSandbox.h"
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

void buildLinearExecutionGraph(GraphModel &graph)
{
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("stop")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("B"), QStringLiteral("C")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("C"), QStringLiteral("D")));
}

class DeterministicExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase0.baseline.provider");
    }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("start"), QStringLiteral("process"), QStringLiteral("stop") };
    }

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QVariantMap *trace,
                          QString *error) const override
    {
        Q_UNUSED(componentSnapshot)
        Q_UNUSED(error)

        QVariantMap state = inputState;
        QStringList order = state.value(QStringLiteral("order")).toStringList();
        order.append(componentId);
        state.insert(QStringLiteral("order"), order);

        if (componentType == QStringLiteral("process")) {
            const int processCount = state.value(QStringLiteral("processCount"), 0).toInt() + 1;
            state.insert(QStringLiteral("processCount"), processCount);
        }

        if (outputState)
            *outputState = state;

        if (trace) {
            trace->insert(QStringLiteral("provider"), providerId());
            trace->insert(QStringLiteral("componentId"), componentId);
            trace->insert(QStringLiteral("componentType"), componentType);
        }

        return true;
    }
};

} // namespace

class tst_Phase0DesignBBaselineFreeze : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void tokenTransportFlag_defaultOn();
    void baselineExecutionDeterminism_flagOff();
    void baselineTraversalCorrectness_flagOff();
    void baselineNoLiveGraphMutation_flagOff();
};

void tst_Phase0DesignBBaselineFreeze::init()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_Phase0DesignBBaselineFreeze::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_Phase0DesignBBaselineFreeze::tokenTransportFlag_defaultOn()
{
    QCOMPARE(cme::execution::MigrationFlags::tokenTransportEnabled(), true);
}

void tst_Phase0DesignBBaselineFreeze::baselineExecutionDeterminism_flagOff()
{
    GraphModel graph;
    buildLinearExecutionGraph(graph);

    DeterministicExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 17 } }));
    sandbox.run();
    const QVariantMap firstState = sandbox.executionState();
    const QVariantList firstTimeline = sandbox.timeline();
    const QVariantMap firstSummary = sandbox.snapshotSummary();

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 17 } }));
    sandbox.run();
    const QVariantMap secondState = sandbox.executionState();
    const QVariantList secondTimeline = sandbox.timeline();
    const QVariantMap secondSummary = sandbox.snapshotSummary();

    QCOMPARE(secondState, firstState);
    QCOMPARE(secondTimeline, firstTimeline);
    QCOMPARE(secondSummary, firstSummary);

    QCOMPARE(secondSummary.value(QStringLiteral("componentCount")).toInt(), 4);
    QCOMPARE(secondSummary.value(QStringLiteral("executedCount")).toInt(), 4);
    QCOMPARE(secondSummary.value(QStringLiteral("pendingCount")).toInt(), 0);
    QCOMPARE(secondSummary.value(QStringLiteral("tokenTransportEnabled")).toBool(), true);
}

void tst_Phase0DesignBBaselineFreeze::baselineTraversalCorrectness_flagOff()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("stop")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("flow")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("A"), QStringLiteral("C"), QStringLiteral("flow")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("B"), QStringLiteral("D"), QStringLiteral("flow")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e4"), QStringLiteral("C"), QStringLiteral("D"), QStringLiteral("flow")));

    TraversalEngine engine;
    engine.setGraph(&graph);

    const QStringList bfsOrder = engine.bfs({ QStringLiteral("A") });
    QCOMPARE(bfsOrder.first(), QStringLiteral("A"));
    QCOMPARE(bfsOrder.size(), 4);

    const QVariantMap topo = engine.topologicalTraversal();
    QCOMPARE(topo.value(QStringLiteral("acyclic")).toBool(), true);
    QCOMPARE(topo.value(QStringLiteral("visitedCount")).toInt(), 4);

    const QStringList shortest = engine.shortestPath(QStringLiteral("A"), QStringLiteral("D"));
    QVERIFY(!shortest.isEmpty());
    QCOMPARE(shortest.first(), QStringLiteral("A"));
    QCOMPARE(shortest.last(), QStringLiteral("D"));
}

void tst_Phase0DesignBBaselineFreeze::baselineNoLiveGraphMutation_flagOff()
{
    GraphModel graph;
    buildLinearExecutionGraph(graph);

    UndoStack undoStack;
    auto *extra = makeComponent(graph, QStringLiteral("X"), QStringLiteral("process"));
    undoStack.pushAddComponent(&graph, extra);

    const int componentCountBefore = graph.componentCount();
    const int connectionCountBefore = graph.connectionCount();
    const int undoCountBefore = undoStack.count();

    DeterministicExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 1 } }));
    QVERIFY(sandbox.run() > 0);

    QCOMPARE(graph.componentCount(), componentCountBefore);
    QCOMPARE(graph.connectionCount(), connectionCountBefore);
    QCOMPARE(undoStack.count(), undoCountBefore);
}

QTEST_MAIN(tst_Phase0DesignBBaselineFreeze)
#include "tst_Phase0DesignBBaselineFreeze.moc"
