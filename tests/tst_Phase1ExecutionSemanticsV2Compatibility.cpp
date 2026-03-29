#include <QtTest>

#include "extensions/contracts/IExecutionSemanticsProvider.h"
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

void buildLinearGraph(GraphModel &graph)
{
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("start")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("process")));
    graph.addComponent(makeComponent(graph, QStringLiteral("C"), QStringLiteral("stop")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("B"), QStringLiteral("C")));
}

class V1OnlyExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase1.v1only.provider");
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

        ++m_v1CallCount;
        m_lastInputState = inputState;

        QVariantMap state = inputState;
        QStringList order = state.value(QStringLiteral("order")).toStringList();
        order.append(componentId);
        state.insert(QStringLiteral("order"), order);
        state.insert(QStringLiteral("lastType"), componentType);

        if (outputState)
            *outputState = state;

        if (trace)
            trace->insert(QStringLiteral("path"), QStringLiteral("v1"));

        return true;
    }

    int v1CallCount() const
    {
        return m_v1CallCount;
    }

    QVariantMap lastInputState() const
    {
        return m_lastInputState;
    }

private:
    mutable int m_v1CallCount = 0;
    mutable QVariantMap m_lastInputState;
};

} // namespace

class tst_Phase1ExecutionSemanticsV2Compatibility : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void v1Fallback_mergesIncomingTokensAndDelegates();
    void sandboxDeterminism_unchangedForV1Providers();
};

void tst_Phase1ExecutionSemanticsV2Compatibility::init()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_Phase1ExecutionSemanticsV2Compatibility::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_Phase1ExecutionSemanticsV2Compatibility::v1Fallback_mergesIncomingTokensAndDelegates()
{
    V1OnlyExecutionProvider provider;

    cme::execution::IncomingTokens incomingTokens;
    incomingTokens.insert(QStringLiteral("edgeA"), QVariantMap{
                                                     { QStringLiteral("fromA"), 10 }
                                                 });
    incomingTokens.insert(QStringLiteral("edgeB"), QVariantMap{
                                                     { QStringLiteral("fromB"), QStringLiteral("payload") }
                                                 });

    cme::execution::ExecutionPayload output;
    QVariantMap trace;
    QString error;

    QVERIFY(provider.executeComponentV2(QStringLiteral("process"),
                                        QStringLiteral("node-1"),
                                        QVariantMap{},
                                        incomingTokens,
                                        &output,
                                        &trace,
                                        &error));

    QCOMPARE(provider.v1CallCount(), 1);
    QCOMPARE(provider.lastInputState().value(QStringLiteral("fromA")).toInt(), 10);
    QCOMPARE(provider.lastInputState().value(QStringLiteral("fromB")).toString(), QStringLiteral("payload"));
    QCOMPARE(output.value(QStringLiteral("fromA")).toInt(), 10);
    QCOMPARE(output.value(QStringLiteral("fromB")).toString(), QStringLiteral("payload"));
    QCOMPARE(trace.value(QStringLiteral("path")).toString(), QStringLiteral("v1"));
}

void tst_Phase1ExecutionSemanticsV2Compatibility::sandboxDeterminism_unchangedForV1Providers()
{
    GraphModel graph;
    buildLinearGraph(graph);

    V1OnlyExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 99 } }));
    sandbox.run();
    const QVariantList firstTimeline = sandbox.timeline();
    const QVariantMap firstState = sandbox.executionState();

    QVERIFY(sandbox.start(QVariantMap{ { QStringLiteral("seed"), 99 } }));
    sandbox.run();
    const QVariantList secondTimeline = sandbox.timeline();
    const QVariantMap secondState = sandbox.executionState();

    QCOMPARE(secondTimeline, firstTimeline);
    QCOMPARE(secondState, firstState);

    const QVariantMap firstStartEvent = firstTimeline.first().toMap();
    QCOMPARE(firstStartEvent.value(QStringLiteral("tokenTransportEnabled")).toBool(), true);
    QCOMPARE(firstTimeline.first().toMap().value(QStringLiteral("event")).toString(), QStringLiteral("simulationStarted"));
    QCOMPARE(firstTimeline.last().toMap().value(QStringLiteral("event")).toString(), QStringLiteral("simulationCompleted"));
}

QTEST_MAIN(tst_Phase1ExecutionSemanticsV2Compatibility)
#include "tst_Phase1ExecutionSemanticsV2Compatibility.moc"
