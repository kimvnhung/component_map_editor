#include <QtTest>

#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "extensions/runtime/CompositeExecutionProvider.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/ExecutionMigrationFlags.h"
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

class RecordingExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override { return QStringLiteral("test.actor.integration.provider"); }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("start"), QStringLiteral("process") };
    }

    QStringList providedOutputKeys(const QString &) const override { return {}; }

    bool executeComponent(const QString &,
                          const QString &componentId,
                          const QVariantMap &,
                          const cme::execution::IncomingTokens &incomingTokens,
                          cme::execution::ExecutionPayload *outputPayload,
                          QVariantMap *trace,
                          QString *error) const override
    {
        Q_UNUSED(trace);
        Q_UNUSED(error);

        QVariantMap merged;
        QStringList tokenKeys = incomingTokens.keys();
        std::sort(tokenKeys.begin(), tokenKeys.end());

        for (const QString &tokenKey : tokenKeys)
        {
            merged.insert(incomingTokens.value(tokenKey));
        }

        QString order = merged.value(QStringLiteral("order")).toString();

        if (!order.isEmpty())
        {
            order += QStringLiteral(",");
        }

        order += componentId;
        merged.insert(QStringLiteral("order"), order);

        if (outputPayload)
        {
            *outputPayload = merged;
        }

        return true;
    }
};

void buildLinearGraph(GraphModel &graph)
{
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("process")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("B"), QStringLiteral("C")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("C"), QStringLiteral("D")));
}

// A -> B -> D and A -> C -> D: D must wait for BOTH B and C.
void buildDiamondGraph(GraphModel &graph)
{
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("process")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("A"), QStringLiteral("C")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("B"), QStringLiteral("D")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e4"), QStringLiteral("C"), QStringLiteral("D")));
}

// Minimal leaf provider for the nested-composite deadlock check: deliberately simple, scalar
// (proto-round-trip-safe) payloads only - correctness of composite token mapping is already
// covered by tst_Phase4CompositeExecutionProvider; this test only cares about "does it hang".
class CompositeLeafProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override { return QStringLiteral("test.actor.integration.leaf"); }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("outer/source"), QStringLiteral("outer/sink"), QStringLiteral("inner/pass") };
    }

    QStringList providedOutputKeys(const QString &) const override { return {}; }

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &,
                          const cme::execution::IncomingTokens &incomingTokens,
                          cme::execution::ExecutionPayload *outputPayload,
                          QVariantMap *trace,
                          QString *error) const override
    {
        Q_UNUSED(trace);
        Q_UNUSED(error);
        QVariantMap out;

        if (componentType == QStringLiteral("outer/source"))
        {
            out.insert(QStringLiteral("value"), componentId);
        }
        else
        {
            const QStringList keys = incomingTokens.keys();
            out = keys.isEmpty() ? QVariantMap() : incomingTokens.value(keys.first());
        }

        if (outputPayload)
        {
            *outputPayload = out;
        }

        return true;
    }
};

cme::runtime::CompositeGraphDefinition makeSimpleCompositeDefinition()
{
    using namespace cme::runtime;

    CompositeGraphDefinition definition;
    definition.componentTypeId = QStringLiteral("composite/simple");
    definition.components =
    {
        { QStringLiteral("entry"), CompositeExecutionProvider::entryComponentType(), QStringLiteral("entry"),
          QVariantMap{ { QStringLiteral("portKey"), QStringLiteral("in") } } },
        { QStringLiteral("pass"), QStringLiteral("inner/pass"), QStringLiteral("pass"), {} },
        { QStringLiteral("exit"), CompositeExecutionProvider::exitComponentType(), QStringLiteral("exit"),
          QVariantMap{ { QStringLiteral("portKey"), QStringLiteral("out") } } }
    };
    definition.connections =
    {
        { QStringLiteral("inner-e1"), QStringLiteral("entry"), QStringLiteral("pass"), {} },
        { QStringLiteral("inner-e2"), QStringLiteral("pass"), QStringLiteral("exit"), {} }
    };
    definition.inputMappings = { { QStringLiteral("outer-e1"), QStringLiteral("entry"), QStringLiteral("in") } };
    definition.outputMappings = { { QStringLiteral("exit"), QStringLiteral("out"), QStringLiteral("result") } };
    return definition;
}

} // namespace

class tst_ActorEngineIntegration : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void linearGraphMatchesSequentialEngineViaFacade();
    void branchingGraphCompletesViaFacadeWithActorEngine();
    void breakpointsForceFallbackToSequentialEngine();
    void nestedCompositeExecutionDoesNotHangWithActorEngineEnabled();
    void rollbackByDisablingFlagRestoresSequentialEngine();
};

void tst_ActorEngineIntegration::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_ActorEngineIntegration::linearGraphMatchesSequentialEngineViaFacade()
{
    RecordingExecutionProvider provider;

    GraphModel sequentialGraph;
    buildLinearGraph(sequentialGraph);
    GraphExecutionSandbox sequentialSandbox;
    sequentialSandbox.setGraph(&sequentialGraph);
    sequentialSandbox.setExecutionSemanticsProviders({ &provider });
    QVERIFY(sequentialSandbox.start());
    sequentialSandbox.run();
    QCOMPARE(sequentialSandbox.status(), QStringLiteral("completed"));
    const QString sequentialOrder = sequentialSandbox.executionState().value(QStringLiteral("order")).toString();

    cme::execution::MigrationFlags::setActorEngineEnabled(true);

    GraphModel actorGraph;
    buildLinearGraph(actorGraph);
    GraphExecutionSandbox actorSandbox;
    actorSandbox.setGraph(&actorGraph);
    actorSandbox.setExecutionSemanticsProviders({ &provider });
    QVERIFY(actorSandbox.start());
    actorSandbox.run();
    QCOMPARE(actorSandbox.status(), QStringLiteral("completed"));
    const QString actorOrder = actorSandbox.executionState().value(QStringLiteral("order")).toString();

    QCOMPARE(actorOrder, sequentialOrder);
    QCOMPARE(actorOrder, QStringLiteral("A,B,C,D"));
}

void tst_ActorEngineIntegration::branchingGraphCompletesViaFacadeWithActorEngine()
{
    cme::execution::MigrationFlags::setActorEngineEnabled(true);

    GraphModel graph;
    buildDiamondGraph(graph);

    RecordingExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    const int executed = sandbox.run();

    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(executed, 4);

    const QVariantMap summary = sandbox.snapshotSummary();
    QCOMPARE(summary.value(QStringLiteral("executedCount")).toInt(), 4);
}

void tst_ActorEngineIntegration::breakpointsForceFallbackToSequentialEngine()
{
    cme::execution::MigrationFlags::setActorEngineEnabled(true);

    GraphModel graph;
    buildLinearGraph(graph);

    RecordingExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });
    sandbox.setBreakpoint(QStringLiteral("C"));

    // A non-empty breakpoint set must make the facade fall back to SequentialExecutionEngine
    // (the actor engine cannot deterministically gate on a breakpoint) - so this must pause
    // exactly before C, same as tst_GraphExecutionSandbox::stepRunPauseAndBreakpointControlsWork.
    QVERIFY(sandbox.start());
    QCOMPARE(sandbox.status(), QStringLiteral("paused"));

    const int executed = sandbox.run();
    QVERIFY(executed > 0);
    QCOMPARE(sandbox.status(), QStringLiteral("paused"));

    const QString orderAtBreakpoint = sandbox.executionState().value(QStringLiteral("order")).toString();
    QVERIFY(!orderAtBreakpoint.contains(QStringLiteral("C")));

    sandbox.clearBreakpoints();
    sandbox.run();
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
}

void tst_ActorEngineIntegration::nestedCompositeExecutionDoesNotHangWithActorEngineEnabled()
{
    cme::execution::MigrationFlags::setActorEngineEnabled(true);

    GraphModel outerGraph;
    outerGraph.addComponent(makeComponent(outerGraph, QStringLiteral("source"), QStringLiteral("outer/source")));
    outerGraph.addComponent(makeComponent(outerGraph, QStringLiteral("comp"), QStringLiteral("composite/simple")));
    outerGraph.addComponent(makeComponent(outerGraph, QStringLiteral("sink"), QStringLiteral("outer/sink")));
    outerGraph.addConnection(makeConnection(outerGraph, QStringLiteral("outer-e1"), QStringLiteral("source"), QStringLiteral("comp")));
    outerGraph.addConnection(makeConnection(outerGraph, QStringLiteral("outer-e2"), QStringLiteral("comp"), QStringLiteral("sink")));

    CompositeLeafProvider leafProvider;
    cme::runtime::CompositeExecutionProvider compositeProvider;
    compositeProvider.setDefinitions({ makeSimpleCompositeDefinition() });
    compositeProvider.setDelegateProviders({ &leafProvider });
    QVERIFY(compositeProvider.isValid());

    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&outerGraph);
    sandbox.setExecutionSemanticsProviders({ &leafProvider, &compositeProvider });

    QVERIFY(sandbox.start());
    const int executed = sandbox.run();

    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(executed, 3);
}

void tst_ActorEngineIntegration::rollbackByDisablingFlagRestoresSequentialEngine()
{
    cme::execution::MigrationFlags::setActorEngineEnabled(true);
    cme::execution::MigrationFlags::setActorEngineEnabled(false);

    GraphModel graph;
    buildLinearGraph(graph);

    RecordingExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    sandbox.run();
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(sandbox.executionState().value(QStringLiteral("order")).toString(), QStringLiteral("A,B,C,D"));
}

QTEST_MAIN(tst_ActorEngineIntegration)
#include "tst_ActorEngineIntegration.moc"
