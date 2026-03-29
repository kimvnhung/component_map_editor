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

class TokenProbeExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase2.token.probe");
    }

    QStringList supportedComponentTypes() const override
    {
        return {
            QStringLiteral("source"),
            QStringLiteral("sourceA"),
            QStringLiteral("sourceB"),
            QStringLiteral("sink"),
            QStringLiteral("merge")
        };
    }

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QVariantMap *trace,
                          QString *error) const override
    {
        Q_UNUSED(componentType)
        Q_UNUSED(componentId)
        Q_UNUSED(componentSnapshot)
        Q_UNUSED(inputState)
        Q_UNUSED(outputState)
        Q_UNUSED(trace)
        if (error)
            *error = QStringLiteral("v1 path should not be used for this test provider");
        return false;
    }

    bool executeComponentV2(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &componentSnapshot,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override
    {
        Q_UNUSED(componentSnapshot)
        Q_UNUSED(error)

        m_seenIncomingByComponent.insert(componentId, incomingTokens);
        ++m_v2CallCount;

        cme::execution::ExecutionPayload out;
        if (componentType == QStringLiteral("source")) {
            out.insert(QStringLiteral("shared"), QStringLiteral("S"));
            out.insert(QStringLiteral("producer"), componentId);
        } else if (componentType == QStringLiteral("sourceA")) {
            out.insert(QStringLiteral("shared"), QStringLiteral("A"));
            out.insert(QStringLiteral("producer"), componentId);
        } else if (componentType == QStringLiteral("sourceB")) {
            out.insert(QStringLiteral("shared"), QStringLiteral("B"));
            out.insert(QStringLiteral("producer"), componentId);
        } else if (componentType == QStringLiteral("sink") || componentType == QStringLiteral("merge")) {
            out.insert(QStringLiteral("incomingCount"), incomingTokens.size());
        }

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            trace->insert(QStringLiteral("path"), QStringLiteral("v2"));
        return true;
    }

    cme::execution::IncomingTokens seenIncoming(const QString &componentId) const
    {
        return m_seenIncomingByComponent.value(componentId);
    }

    int v2CallCount() const
    {
        return m_v2CallCount;
    }

private:
    mutable QHash<QString, cme::execution::IncomingTokens> m_seenIncomingByComponent;
    mutable int m_v2CallCount = 0;
};

} // namespace

class tst_Phase2TokenRouting : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void fanOut_duplicatesPayloadToAllOutgoingEdges();
    void fanIn_receivesDistinctIncomingTokens();
    void noCrossBranchKeyCollision_sameKeyPreservedPerEdge();
    void timelineSequence_noRegressionWithTokenRouting();
};

void tst_Phase2TokenRouting::init()
{
    cme::execution::MigrationFlags::resetDefaults();
    cme::execution::MigrationFlags::setTokenTransportEnabled(true);
}

void tst_Phase2TokenRouting::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_Phase2TokenRouting::fanOut_duplicatesPayloadToAllOutgoingEdges()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("S"), QStringLiteral("source")));
    graph.addComponent(makeComponent(graph, QStringLiteral("L"), QStringLiteral("sink")));
    graph.addComponent(makeComponent(graph, QStringLiteral("R"), QStringLiteral("sink")));

    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("S"), QStringLiteral("L")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e2"), QStringLiteral("S"), QStringLiteral("R")));

    TokenProbeExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    sandbox.run();

    const cme::execution::IncomingTokens incomingL = provider.seenIncoming(QStringLiteral("L"));
    const cme::execution::IncomingTokens incomingR = provider.seenIncoming(QStringLiteral("R"));

    QCOMPARE(incomingL.size(), 1);
    QCOMPARE(incomingR.size(), 1);
    QCOMPARE(incomingL.value(QStringLiteral("e1")).value(QStringLiteral("shared")).toString(), QStringLiteral("S"));
    QCOMPARE(incomingR.value(QStringLiteral("e2")).value(QStringLiteral("shared")).toString(), QStringLiteral("S"));
}

void tst_Phase2TokenRouting::fanIn_receivesDistinctIncomingTokens()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("sourceA")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("sourceB")));
    graph.addComponent(makeComponent(graph, QStringLiteral("M"), QStringLiteral("merge")));

    graph.addConnection(makeConnection(graph, QStringLiteral("eA"), QStringLiteral("A"), QStringLiteral("M")));
    graph.addConnection(makeConnection(graph, QStringLiteral("eB"), QStringLiteral("B"), QStringLiteral("M")));

    TokenProbeExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    sandbox.run();

    const cme::execution::IncomingTokens incomingM = provider.seenIncoming(QStringLiteral("M"));
    QCOMPARE(incomingM.size(), 2);
    QCOMPARE(incomingM.value(QStringLiteral("eA")).value(QStringLiteral("producer")).toString(), QStringLiteral("A"));
    QCOMPARE(incomingM.value(QStringLiteral("eB")).value(QStringLiteral("producer")).toString(), QStringLiteral("B"));
}

void tst_Phase2TokenRouting::noCrossBranchKeyCollision_sameKeyPreservedPerEdge()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("sourceA")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("sourceB")));
    graph.addComponent(makeComponent(graph, QStringLiteral("M"), QStringLiteral("merge")));

    graph.addConnection(makeConnection(graph, QStringLiteral("edgeA"), QStringLiteral("A"), QStringLiteral("M")));
    graph.addConnection(makeConnection(graph, QStringLiteral("edgeB"), QStringLiteral("B"), QStringLiteral("M")));

    TokenProbeExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    sandbox.run();

    const cme::execution::IncomingTokens incomingM = provider.seenIncoming(QStringLiteral("M"));
    QCOMPARE(incomingM.size(), 2);

    const QString sharedA = incomingM.value(QStringLiteral("edgeA")).value(QStringLiteral("shared")).toString();
    const QString sharedB = incomingM.value(QStringLiteral("edgeB")).value(QStringLiteral("shared")).toString();

    QCOMPARE(sharedA, QStringLiteral("A"));
    QCOMPARE(sharedB, QStringLiteral("B"));
    QVERIFY(sharedA != sharedB);
}

void tst_Phase2TokenRouting::timelineSequence_noRegressionWithTokenRouting()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("S"), QStringLiteral("source")));
    graph.addComponent(makeComponent(graph, QStringLiteral("L"), QStringLiteral("sink")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("S"), QStringLiteral("L")));

    TokenProbeExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    sandbox.run();

    const QVariantList timeline = sandbox.timeline();
    QVERIFY(timeline.size() >= 3);
    QCOMPARE(timeline.first().toMap().value(QStringLiteral("event")).toString(), QStringLiteral("simulationStarted"));
    QCOMPARE(timeline.last().toMap().value(QStringLiteral("event")).toString(), QStringLiteral("simulationCompleted"));

    int stepCount = 0;
    for (const QVariant &entry : timeline) {
        if (entry.toMap().value(QStringLiteral("event")).toString() == QStringLiteral("stepExecuted"))
            ++stepCount;
    }

    QCOMPARE(stepCount, 2);
    QVERIFY(provider.v2CallCount() >= 2);
}

QTEST_MAIN(tst_Phase2TokenRouting)
#include "tst_Phase2TokenRouting.moc"
