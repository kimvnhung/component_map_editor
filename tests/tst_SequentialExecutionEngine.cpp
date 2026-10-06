#include <QtTest>

#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/ExecutionMigrationFlags.h"
#include "services/execution_internal/IExecutionEngine.h"
#include "services/execution_internal/SequentialExecutionEngine.h"

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

// A -> B -> D and A -> C -> D: D must wait for BOTH B and C (in-degree join gate).
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

// Scriptable provider: "start"/"process" always succeed, "faulty" always fails,
// "invalid" succeeds but never produces its declared output key.
class ScriptedExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("test.engine.provider");
    }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("start"), QStringLiteral("process"), QStringLiteral("faulty"), QStringLiteral("invalid") };
    }

    QStringList providedOutputKeys(const QString &componentType) const override
    {
        if (componentType == QStringLiteral("invalid"))
        {
            return { QStringLiteral("requiredKey") };
        }

        return {};
    }

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const cme::execution::IncomingTokens &incomingTokens,
                          cme::execution::ExecutionPayload *outputPayload,
                          QVariantMap *trace,
                          QString *error) const override
    {
        Q_UNUSED(componentSnapshot);
        Q_UNUSED(incomingTokens);
        Q_UNUSED(trace);

        if (componentType == QStringLiteral("faulty"))
        {
            if (error)
            {
                *error = QStringLiteral("boom");
            }

            return false;
        }

        if (componentType == QStringLiteral("invalid"))
        {
            if (outputPayload)
            {
                outputPayload->insert(QStringLiteral("otherKey"), 1);
            }

            return true;
        }

        if (outputPayload)
        {
            outputPayload->insert(QStringLiteral("visited"), componentId);
        }

        return true;
    }
};

QHash<QString, const IExecutionSemanticsProvider *> providerMapFor(const IExecutionSemanticsProvider &provider)
{
    QHash<QString, const IExecutionSemanticsProvider *> providers;

    for (const QString &type : provider.supportedComponentTypes())
    {
        providers.insert(type, &provider);
    }

    return providers;
}

// Minimal IExecutionEngine implementation with zero dependency on GraphModel/QML - proves the
// interface introduced in Plan 1 can be mocked/substituted by callers that only see IExecutionEngine.
class NullExecutionEngine : public IExecutionEngine
{
public:
    bool prepare(GraphModel *, const QHash<QString, const IExecutionSemanticsProvider *> &,
                const QVariantMap &, QString *) override
    {
        m_remaining = 1;
        m_executed = 0;
        return true;
    }

    bool hasReadyWork() const override { return m_remaining > 0; }
    QString peekNextReadyComponentId() const override { return hasReadyWork() ? QStringLiteral("mock") : QString(); }
    QStringList readyComponentIds() const override { return hasReadyWork() ? QStringList{ QStringLiteral("mock") } : QStringList(); }
    int executedCount() const override { return m_executed; }
    int totalComponentCount() const override { return 1; }
    const cme::GraphSnapshot &graphSnapshot() const override { return m_emptySnapshot; }
    Telemetry telemetry() const override { return {}; }

    StepOutcome executeNext(const QVariantMap &, ExecutionContext *outCtx, ExecuteResult *outResult, QString *) override
    {
        if (m_remaining <= 0)
        {
            return StepOutcome::NoWork;
        }

        m_remaining = 0;
        ++m_executed;

        if (outCtx)
        {
            outCtx->componentId = QStringLiteral("mock");
        }

        if (outResult)
        {
            outResult->status = ExecuteResult::Status::Ok;
        }

        return StepOutcome::Committed;
    }

    void reset() override
    {
        m_remaining = 0;
        m_executed = 0;
    }

private:
    int m_remaining = 0;
    int m_executed = 0;
    cme::GraphSnapshot m_emptySnapshot;
};

} // namespace

class tst_SequentialExecutionEngine : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void prepareBuildsReadyQueueFromZeroInDegreeComponents();
    void executeNextCommitsAndTopologicallyUnlocksNextReadyComponent();
    void executeNextGatesMergeNodeUntilAllIncomingEdgesCommitted();
    void executeNextReturnsProviderErrorWithoutMarkingComponentExecuted();
    void executeNextReturnsValidationErrorButStillPopulatesOutput();
    void resetClearsAllSchedulingAndTelemetryState();
    void telemetryTracksTokenReadWriteCountersAcrossSteps();
    void legacyGlobalStateFlowsToContextWhenTokenTransportDisabled();
    void engineInterfaceIsMockableWithoutGraphModelOrQml();
};

void tst_SequentialExecutionEngine::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_SequentialExecutionEngine::prepareBuildsReadyQueueFromZeroInDegreeComponents()
{
    GraphModel graph;
    buildLinearGraph(graph);

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString error;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap(), &error));
    QVERIFY(error.isEmpty());

    QVERIFY(engine.hasReadyWork());
    QCOMPARE(engine.peekNextReadyComponentId(), QStringLiteral("A"));
    QCOMPARE(engine.readyComponentIds(), QStringList{ QStringLiteral("A") });
    QCOMPARE(engine.totalComponentCount(), 4);
    QCOMPARE(engine.executedCount(), 0);
}

void tst_SequentialExecutionEngine::executeNextCommitsAndTopologicallyUnlocksNextReadyComponent()
{
    GraphModel graph;
    buildLinearGraph(graph);

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString error;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap(), &error));

    QStringList committedOrder;

    while (engine.hasReadyWork())
    {
        ExecutionContext ctx;
        ExecuteResult result;
        const IExecutionEngine::StepOutcome outcome = engine.executeNext(QVariantMap(), &ctx, &result, &error);
        QCOMPARE(outcome, IExecutionEngine::StepOutcome::Committed);
        committedOrder.append(ctx.componentId);
    }

    QCOMPARE(committedOrder, (QStringList{ QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("D") }));
    QCOMPARE(engine.executedCount(), 4);
    QCOMPARE(engine.executedCount(), engine.totalComponentCount());
}

void tst_SequentialExecutionEngine::executeNextGatesMergeNodeUntilAllIncomingEdgesCommitted()
{
    GraphModel graph;
    buildDiamondGraph(graph);

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString error;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap(), &error));

    // Execute A: unlocks both B and C, D must stay gated (in-degree 2).
    ExecutionContext ctx;
    ExecuteResult result;
    QCOMPARE(engine.executeNext(QVariantMap(), &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);
    QCOMPARE(ctx.componentId, QStringLiteral("A"));
    QCOMPARE(engine.readyComponentIds(), (QStringList{ QStringLiteral("B"), QStringLiteral("C") }));

    // Execute B: only one of D's two incoming edges is satisfied, D must remain gated.
    QCOMPARE(engine.executeNext(QVariantMap(), &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);
    QCOMPARE(ctx.componentId, QStringLiteral("B"));
    QCOMPARE(engine.readyComponentIds(), QStringList{ QStringLiteral("C") });
    QVERIFY(!engine.readyComponentIds().contains(QStringLiteral("D")));

    // Execute C: D's last incoming edge is satisfied, D becomes ready.
    QCOMPARE(engine.executeNext(QVariantMap(), &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);
    QCOMPARE(ctx.componentId, QStringLiteral("C"));
    QCOMPARE(engine.readyComponentIds(), QStringList{ QStringLiteral("D") });

    QCOMPARE(engine.executeNext(QVariantMap(), &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);
    QCOMPARE(ctx.componentId, QStringLiteral("D"));
    QVERIFY(!engine.hasReadyWork());
    QCOMPARE(engine.executedCount(), 4);
}

void tst_SequentialExecutionEngine::executeNextReturnsProviderErrorWithoutMarkingComponentExecuted()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("faulty")));

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString prepareError;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap(), &prepareError));

    ExecutionContext ctx;
    ExecuteResult result;
    QString error;
    const IExecutionEngine::StepOutcome outcome = engine.executeNext(QVariantMap(), &ctx, &result, &error);

    QCOMPARE(outcome, IExecutionEngine::StepOutcome::ProviderError);
    QCOMPARE(error, QStringLiteral("boom"));
    QCOMPARE(engine.executedCount(), 0);
    QVERIFY(!engine.hasReadyWork());
}

void tst_SequentialExecutionEngine::executeNextReturnsValidationErrorButStillPopulatesOutput()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("invalid")));

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString prepareError;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap(), &prepareError));

    ExecutionContext ctx;
    ExecuteResult result;
    QString error;
    const IExecutionEngine::StepOutcome outcome = engine.executeNext(QVariantMap(), &ctx, &result, &error);

    QCOMPARE(outcome, IExecutionEngine::StepOutcome::ValidationError);
    QVERIFY(!error.isEmpty());
    // Facade contract (GraphExecutionSandbox::executeOneStep): even on ValidationError, outResult must
    // still carry the produced output so the caller can apply it to executionState before erroring out.
    QCOMPARE(result.output.value(QStringLiteral("otherKey")).toInt(), 1);
    QCOMPARE(engine.executedCount(), 0);
}

void tst_SequentialExecutionEngine::resetClearsAllSchedulingAndTelemetryState()
{
    GraphModel graph;
    buildLinearGraph(graph);

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString error;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap(), &error));

    ExecutionContext ctx;
    ExecuteResult result;
    QCOMPARE(engine.executeNext(QVariantMap(), &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);

    engine.reset();

    QVERIFY(!engine.hasReadyWork());
    QCOMPARE(engine.executedCount(), 0);
    QCOMPARE(engine.totalComponentCount(), 0);

    const IExecutionEngine::Telemetry telemetry = engine.telemetry();
    QCOMPARE(telemetry.tokenReadCount, 0);
    QCOMPARE(telemetry.tokenWriteCount, 0);
    QCOMPARE(telemetry.payloadBytesRead, qint64(0));
    QCOMPARE(telemetry.payloadBytesWritten, qint64(0));
    QCOMPARE(telemetry.maxPayloadBytes, qint64(0));
}

void tst_SequentialExecutionEngine::telemetryTracksTokenReadWriteCountersAcrossSteps()
{
    GraphModel graph;
    buildLinearGraph(graph);

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString error;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap{ { QStringLiteral("seed"), 1 } }, &error));

    while (engine.hasReadyWork())
    {
        ExecutionContext ctx;
        ExecuteResult result;
        QCOMPARE(engine.executeNext(QVariantMap(), &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);
    }

    const IExecutionEngine::Telemetry telemetry = engine.telemetry();
    QVERIFY(telemetry.tokenReadCount > 0);
    QVERIFY(telemetry.tokenWriteCount > 0);
    QVERIFY(telemetry.payloadBytesWritten > 0);
}

void tst_SequentialExecutionEngine::legacyGlobalStateFlowsToContextWhenTokenTransportDisabled()
{
    cme::execution::MigrationFlags::setTokenTransportEnabled(false);

    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));

    ScriptedExecutionProvider provider;
    SequentialExecutionEngine engine;
    QString error;
    QVERIFY(engine.prepare(&graph, providerMapFor(provider), QVariantMap(), &error));

    const QVariantMap legacyState{ { QStringLiteral("counter"), 42 } };
    ExecutionContext ctx;
    ExecuteResult result;
    QCOMPARE(engine.executeNext(legacyState, &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);

    QCOMPARE(ctx.stepState, legacyState);
    QCOMPARE(ctx.incomingTokens.value(QStringLiteral("__legacy_global_state__")), legacyState);
}

void tst_SequentialExecutionEngine::engineInterfaceIsMockableWithoutGraphModelOrQml()
{
    NullExecutionEngine mockEngine;
    IExecutionEngine *engine = &mockEngine; // exercised purely through the interface

    QString error;
    QVERIFY(engine->prepare(nullptr, {}, QVariantMap(), &error));
    QVERIFY(engine->hasReadyWork());
    QCOMPARE(engine->peekNextReadyComponentId(), QStringLiteral("mock"));

    ExecutionContext ctx;
    ExecuteResult result;
    QCOMPARE(engine->executeNext(QVariantMap(), &ctx, &result, &error), IExecutionEngine::StepOutcome::Committed);
    QCOMPARE(ctx.componentId, QStringLiteral("mock"));
    QCOMPARE(engine->executedCount(), 1);
    QVERIFY(!engine->hasReadyWork());

    engine->reset();
    QCOMPARE(engine->executedCount(), 0);
}

QTEST_MAIN(tst_SequentialExecutionEngine)
#include "tst_SequentialExecutionEngine.moc"
