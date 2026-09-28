#include <QtTest>

#include <mutex>
#include <vector>

#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/execution_internal/SequentialExecutionEngine.h"
#include "services/execution_internal/actor_mailbox/ActorScheduler.h"
#include "services/execution_internal/actor_mailbox/GraphSnapshotActorAdapter.h"
#include "utils/GraphHelper.h"

using cme::actor::ActorScheduler;
using cme::actor::GraphSnapshotActorAdapter;

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
void buildDiamondGraph(GraphModel &graph, const QString &bType = QStringLiteral("process"))
{
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), bType));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("D"), QStringLiteral("process")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("A"), QStringLiteral("C")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e3"), QStringLiteral("B"), QStringLiteral("D")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e4"), QStringLiteral("C"), QStringLiteral("D")));
}

class ScriptedActorProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override { return QStringLiteral("test.actor.provider"); }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("start"), QStringLiteral("process"), QStringLiteral("faulty") };
    }

    QStringList providedOutputKeys(const QString &) const override { return {}; }

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const cme::execution::IncomingTokens &incomingTokens,
                          cme::execution::ExecutionPayload *outputPayload,
                          QVariantMap *trace,
                          QString *error) const override
    {
        Q_UNUSED(componentSnapshot);
        Q_UNUSED(trace);

        if (componentType == QStringLiteral("faulty"))
        {
            if (error)
            {
                *error = QStringLiteral("boom");
            }

            return false;
        }

        QVariantMap merged = cme::helper::mergeIncomingTokens(incomingTokens);
        QString order = merged.value(QStringLiteral("order")).toString();

        if (!order.isEmpty())
        {
            order += QStringLiteral(",");
        }

        order += componentId;
        merged.insert(QStringLiteral("order"), order);
        merged.insert(QStringLiteral("incomingKeyCount"), incomingTokens.size());

        if (outputPayload)
        {
            *outputPayload = merged;
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

struct CapturedStep
{
    ExecutionContext ctx;
    ExecuteResult result;
};

// Thread-safe collector: ActorScheduler invokes the step callback from worker threads.
class StepCollector
{
public:
    void onStep(const ExecutionContext &ctx, const ExecuteResult &result)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_steps.push_back({ ctx, result });
    }

    std::vector<CapturedStep> steps() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_steps;
    }

private:
    mutable std::mutex m_mutex;
    std::vector<CapturedStep> m_steps;
};

// Captures a cme::GraphSnapshot from a GraphModel by reusing SequentialExecutionEngine's
// already-tested capture logic, keeping this test decoupled from GraphModel->proto details.
cme::GraphSnapshot captureSnapshot(GraphModel &graph,
                                   const QHash<QString, const IExecutionSemanticsProvider *> &providers,
                                   QString *error)
{
    SequentialExecutionEngine engine;

    if (!engine.prepare(&graph, providers, QVariantMap(), error))
    {
        return {};
    }

    return engine.graphSnapshot();
}

} // namespace

class tst_ActorMailboxEngine : public QObject
{
    Q_OBJECT

private slots:
    void linearGraphExecutesAllComponentsInParallelMode();
    void diamondGraphJoinGateWaitsForBothIncomingEdges();
    void providerErrorIsIsolatedAndDoesNotPropagateDownstream();
    void invalidGraphReferenceFailsAdapterBuild();
    void matchesSequentialEngineOutputOnDiamondGraph();
};

void tst_ActorMailboxEngine::linearGraphExecutesAllComponentsInParallelMode()
{
    GraphModel graph;
    buildLinearGraph(graph);

    ScriptedActorProvider provider;
    const auto providers = providerMapFor(provider);

    QString error;
    const cme::GraphSnapshot snapshot = captureSnapshot(graph, providers, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    StepCollector collector;
    ActorScheduler scheduler([&collector](const ExecutionContext &ctx, const ExecuteResult &result)
    {
        collector.onStep(ctx, result);
    });

    QStringList entryIds;
    QVERIFY2(GraphSnapshotActorAdapter::build(snapshot, providers, &scheduler, &entryIds, &error),
             qPrintable(error));
    QCOMPARE(entryIds, QStringList{ QStringLiteral("A") });

    scheduler.start();
    scheduler.seedEntryComponents(entryIds, QVariantMap());
    QVERIFY(scheduler.runUntilIdle(std::chrono::seconds(2)));
    scheduler.shutdown();

    const auto steps = collector.steps();
    QCOMPARE(steps.size(), size_t(4));

    for (const CapturedStep &step : steps)
    {
        QCOMPARE(step.result.status, ExecuteResult::Status::Ok);
    }
}

void tst_ActorMailboxEngine::diamondGraphJoinGateWaitsForBothIncomingEdges()
{
    GraphModel graph;
    buildDiamondGraph(graph);

    ScriptedActorProvider provider;
    const auto providers = providerMapFor(provider);

    QString error;
    const cme::GraphSnapshot snapshot = captureSnapshot(graph, providers, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    StepCollector collector;
    ActorScheduler scheduler([&collector](const ExecutionContext &ctx, const ExecuteResult &result)
    {
        collector.onStep(ctx, result);
    });

    QStringList entryIds;
    QVERIFY2(GraphSnapshotActorAdapter::build(snapshot, providers, &scheduler, &entryIds, &error),
             qPrintable(error));
    QCOMPARE(entryIds, QStringList{ QStringLiteral("A") });

    scheduler.start();
    scheduler.seedEntryComponents(entryIds, QVariantMap());
    QVERIFY(scheduler.runUntilIdle(std::chrono::seconds(2)));
    scheduler.shutdown();

    const auto steps = collector.steps();
    QCOMPARE(steps.size(), size_t(4));

    int dFireCount = 0;

    for (const CapturedStep &step : steps)
    {
        if (step.ctx.componentId == QStringLiteral("D"))
        {
            ++dFireCount;
            // D must only fire once it has received tokens from BOTH e3 (B->D) and e4 (C->D) -
            // this is the join-gate fix; the earlier PoC would fire D on the first predecessor.
            QCOMPARE(step.ctx.incomingTokens.size(), 2);
            QVERIFY(step.ctx.incomingTokens.contains(QStringLiteral("e3")));
            QVERIFY(step.ctx.incomingTokens.contains(QStringLiteral("e4")));
        }
    }

    QCOMPARE(dFireCount, 1);
}

void tst_ActorMailboxEngine::providerErrorIsIsolatedAndDoesNotPropagateDownstream()
{
    GraphModel graph;
    buildDiamondGraph(graph, QStringLiteral("faulty"));

    ScriptedActorProvider provider;
    const auto providers = providerMapFor(provider);

    QString error;
    const cme::GraphSnapshot snapshot = captureSnapshot(graph, providers, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    StepCollector collector;
    ActorScheduler scheduler([&collector](const ExecutionContext &ctx, const ExecuteResult &result)
    {
        collector.onStep(ctx, result);
    });

    QStringList entryIds;
    QVERIFY2(GraphSnapshotActorAdapter::build(snapshot, providers, &scheduler, &entryIds, &error),
             qPrintable(error));

    scheduler.start();
    scheduler.seedEntryComponents(entryIds, QVariantMap());
    // Must still reach quiescence quickly even though D can never complete its join gate -
    // the scheduler has no more work to do, it should not hang waiting on B's never-arriving token.
    QVERIFY(scheduler.runUntilIdle(std::chrono::seconds(2)));
    scheduler.shutdown();

    const auto steps = collector.steps();
    QCOMPARE(steps.size(), size_t(3)); // A, B(error), C - D never fires

    bool sawFailedB = false;

    for (const CapturedStep &step : steps)
    {
        QVERIFY(step.ctx.componentId != QStringLiteral("D"));

        if (step.ctx.componentId == QStringLiteral("B"))
        {
            sawFailedB = true;
            QCOMPARE(step.result.status, ExecuteResult::Status::Error);
            QCOMPARE(step.result.errorMessage, QStringLiteral("boom"));
        }
    }

    QVERIFY(sawFailedB);
}

void tst_ActorMailboxEngine::invalidGraphReferenceFailsAdapterBuild()
{
    cme::GraphSnapshot snapshot;
    cme::ComponentData *component = snapshot.add_components();
    component->set_id("A");
    component->set_type_id("start");

    cme::ConnectionData *connection = snapshot.add_connections();
    connection->set_id("e1");
    connection->set_source_id("A");
    connection->set_target_id("missing-target");

    ScriptedActorProvider provider;
    const auto providers = providerMapFor(provider);

    ActorScheduler scheduler([](const ExecutionContext &, const ExecuteResult &) {});
    QStringList entryIds;
    QString error;
    QVERIFY(!GraphSnapshotActorAdapter::build(snapshot, providers, &scheduler, &entryIds, &error));
    QVERIFY(!error.isEmpty());
}

void tst_ActorMailboxEngine::matchesSequentialEngineOutputOnDiamondGraph()
{
    GraphModel graph;
    buildDiamondGraph(graph);

    ScriptedActorProvider provider;
    const auto providers = providerMapFor(provider);

    // Reference: drive the same graph fully through SequentialExecutionEngine.
    SequentialExecutionEngine sequentialEngine;
    QString error;
    QVERIFY2(sequentialEngine.prepare(&graph, providers, QVariantMap(), &error), qPrintable(error));

    QVariantMap sequentialDOutput;

    while (sequentialEngine.hasReadyWork())
    {
        ExecutionContext ctx;
        ExecuteResult result;
        QCOMPARE(sequentialEngine.executeNext(QVariantMap(), &ctx, &result, &error),
                 IExecutionEngine::StepOutcome::Committed);

        if (ctx.componentId == QStringLiteral("D"))
        {
            sequentialDOutput = result.output;
        }
    }

    // Now drive an independent copy of the same graph through the actor engine.
    GraphModel actorGraph;
    buildDiamondGraph(actorGraph);
    const cme::GraphSnapshot snapshot = captureSnapshot(actorGraph, providers, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    StepCollector collector;
    ActorScheduler scheduler([&collector](const ExecutionContext &ctx, const ExecuteResult &result)
    {
        collector.onStep(ctx, result);
    });

    QStringList entryIds;
    QVERIFY2(GraphSnapshotActorAdapter::build(snapshot, providers, &scheduler, &entryIds, &error),
             qPrintable(error));

    scheduler.start();
    scheduler.seedEntryComponents(entryIds, QVariantMap());
    QVERIFY(scheduler.runUntilIdle(std::chrono::seconds(2)));
    scheduler.shutdown();

    QVariantMap actorDOutput;

    for (const CapturedStep &step : collector.steps())
    {
        if (step.ctx.componentId == QStringLiteral("D"))
        {
            actorDOutput = step.result.output;
        }
    }

    // Both engines merge incoming tokens via the same cme::helper::mergeIncomingTokens (sorted
    // by connection id), so D's final "order" must be identical regardless of engine/thread timing.
    // (Uses a scalar joined string rather than a QStringList so the comparison isn't confounded by
    // SequentialExecutionEngine's connection-payload proto round trip, which is lossy for multi-item
    // lists - see cme::helper::variantMapToMap.)
    QCOMPARE(actorDOutput.value(QStringLiteral("order")).toString(),
             sequentialDOutput.value(QStringLiteral("order")).toString());
}

QTEST_MAIN(tst_ActorMailboxEngine)
#include "tst_ActorMailboxEngine.moc"
