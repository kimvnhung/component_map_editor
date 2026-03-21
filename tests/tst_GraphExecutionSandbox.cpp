#include <QtTest>

#include "commands/UndoStack.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/GraphExecutionSandbox.h"

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
                                const QString &targetId)
{
    auto *connection = new ConnectionModel(&graph);
    connection->setId(id);
    connection->setSourceId(sourceId);
    connection->setTargetId(targetId);
    return connection;
}

class DeterministicExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("test.execution.provider");
    }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("start"), QStringLiteral("task"), QStringLiteral("end") };
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

        if (componentType == QStringLiteral("task")) {
            const int taskCount = state.value(QStringLiteral("taskCount"), 0).toInt() + 1;
            state.insert(QStringLiteral("taskCount"), taskCount);
        }

        state.insert(QStringLiteral("lastType"), componentType);

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

void buildLinearGraph(GraphModel &graph)
{
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("task")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("task")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("end")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("B"), QStringLiteral("C")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("C"), QStringLiteral("D")));
}

} // namespace

class tst_GraphExecutionSandbox : public QObject
{
    Q_OBJECT

private slots:
    void executionDoesNotMutateLiveGraphOrUndoStack();
    void repeatedRunsAreDeterministic();
    void stepRunPauseAndBreakpointControlsWork();
    void timelineAndStateTraceAvailable();
    void providersCanBeLoadedFromContractRegistry();
};

void tst_GraphExecutionSandbox::executionDoesNotMutateLiveGraphOrUndoStack()
{
    GraphModel graph;
    buildLinearGraph(graph);

    UndoStack undoStack;
    auto *extra = makeComponent(graph, QStringLiteral("X"), QStringLiteral("task"));
    undoStack.pushAddComponent(&graph, extra);

    const int componentCountBefore = graph.componentCount();
    const int connectionCountBefore = graph.connectionCount();
    const int undoCountBefore = undoStack.count();

    DeterministicExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 1 } }));
    const int executed = sandbox.run();
    QVERIFY(executed > 0);

    QCOMPARE(graph.componentCount(), componentCountBefore);
    QCOMPARE(graph.connectionCount(), connectionCountBefore);
    QCOMPARE(undoStack.count(), undoCountBefore);
}

void tst_GraphExecutionSandbox::repeatedRunsAreDeterministic()
{
    GraphModel graph;
    buildLinearGraph(graph);

    DeterministicExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 7 } }));
    sandbox.run();
    const QVariantMap firstState = sandbox.executionState();
    const QVariantList firstTimeline = sandbox.timeline();

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 7 } }));
    sandbox.run();
    const QVariantMap secondState = sandbox.executionState();
    const QVariantList secondTimeline = sandbox.timeline();

    QCOMPARE(secondState, firstState);
    QCOMPARE(secondTimeline, firstTimeline);
}

void tst_GraphExecutionSandbox::stepRunPauseAndBreakpointControlsWork()
{
    GraphModel graph;
    buildLinearGraph(graph);

    DeterministicExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });
    sandbox.setBreakpoint(QStringLiteral("C"));

    QVERIFY(sandbox.start());
    QCOMPARE(sandbox.status(), QStringLiteral("paused"));

    QVERIFY(sandbox.step());
    QCOMPARE(sandbox.currentTick(), 1);

    const int runExecuted = sandbox.run();
    QVERIFY(runExecuted > 0);
    QCOMPARE(sandbox.status(), QStringLiteral("paused"));

    const QVariantMap stateAtBreakpoint = sandbox.executionState();
    const QStringList orderAtBreakpoint = stateAtBreakpoint.value(QStringLiteral("order")).toStringList();
    QVERIFY(!orderAtBreakpoint.contains(QStringLiteral("C")));

    QVERIFY(sandbox.step());
    QVERIFY(sandbox.run() >= 0);
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
}

void tst_GraphExecutionSandbox::timelineAndStateTraceAvailable()
{
    GraphModel graph;
    buildLinearGraph(graph);

    DeterministicExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("traceMode"), QStringLiteral("full") } }));
    sandbox.run();

    const QVariantList timeline = sandbox.timeline();
    QVERIFY(timeline.size() >= 2);
    QCOMPARE(timeline.first().toMap().value(QStringLiteral("event")).toString(), QStringLiteral("simulationStarted"));
    QCOMPARE(timeline.last().toMap().value(QStringLiteral("event")).toString(), QStringLiteral("simulationCompleted"));

    const QVariantMap componentB = sandbox.componentState(QStringLiteral("B"));
    QCOMPARE(componentB.value(QStringLiteral("status")).toString(), QStringLiteral("executed"));
    QVERIFY(componentB.contains(QStringLiteral("trace")));

    const QVariantMap summary = sandbox.snapshotSummary();
    QCOMPARE(summary.value(QStringLiteral("componentCount")).toInt(), 4);
    QCOMPARE(summary.value(QStringLiteral("executedCount")).toInt(), 4);
}

void tst_GraphExecutionSandbox::providersCanBeLoadedFromContractRegistry()
{
    GraphModel graph;
    buildLinearGraph(graph);

    DeterministicExecutionProvider provider;
    ExtensionContractRegistry registry(ExtensionApiVersion{ 1, 0, 0 });
    QVERIFY(registry.registerExecutionSemanticsProvider(&provider));

    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.rebuildSemanticsFromRegistry(registry);

    QVERIFY(sandbox.start());
    sandbox.run();
    const QStringList order = sandbox.executionState().value(QStringLiteral("order")).toStringList();
    QCOMPARE(order.size(), 4);
}

QTEST_MAIN(tst_GraphExecutionSandbox)
#include "tst_GraphExecutionSandbox.moc"
