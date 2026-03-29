#include <QtTest>

#include <QElapsedTimer>

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

class LeafExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase4.leaf");
    }

    QStringList supportedComponentTypes() const override
    {
        return {
            QStringLiteral("outer/source"),
            QStringLiteral("outer/sink"),
            QStringLiteral("inner/pass"),
            QStringLiteral("inner/source"),
            QStringLiteral("inner/sink")
        };
    }

    bool executeComponent(const QString &, const QString &, const QVariantMap &, const QVariantMap &,
                          QVariantMap *, QVariantMap *, QString *error) const override
    {
        if (error)
            *error = QStringLiteral("Phase 4 tests require v2 execution path.");
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

        m_seenIncoming.insert(componentId, incomingTokens);

        QVariantMap out;
        if (componentType == QStringLiteral("outer/source")
            || componentType == QStringLiteral("inner/source")) {
            out.insert(QStringLiteral("value"), componentId);
        } else if (componentType == QStringLiteral("inner/pass")) {
            const QString tokenKey = incomingTokens.keys().isEmpty() ? QString() : incomingTokens.keys().first();
            out = incomingTokens.value(tokenKey);
        } else if (componentType == QStringLiteral("outer/sink")
                   || componentType == QStringLiteral("inner/sink")) {
            QVariantMap merged;
            QStringList tokenKeys = incomingTokens.keys();
            std::sort(tokenKeys.begin(), tokenKeys.end());
            for (const QString &tokenKey : tokenKeys)
                merged.insert(incomingTokens.value(tokenKey));
            out = merged;
        }

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            trace->insert(QStringLiteral("provider"), providerId());
        return true;
    }

    cme::execution::IncomingTokens seenIncoming(const QString &componentId) const
    {
        return m_seenIncoming.value(componentId);
    }

private:
    mutable QHash<QString, cme::execution::IncomingTokens> m_seenIncoming;
};

cme::runtime::CompositeGraphDefinition makeSimpleCompositeDefinition()
{
    using namespace cme::runtime;

    CompositeGraphDefinition definition;
    definition.componentTypeId = QStringLiteral("composite/simple");
    definition.components = {
        { QStringLiteral("entry"), CompositeExecutionProvider::entryComponentType(), QStringLiteral("entry"),
          QVariantMap{ { QStringLiteral("portKey"), QStringLiteral("in") } } },
        { QStringLiteral("pass"), QStringLiteral("inner/pass"), QStringLiteral("pass"), {} },
        { QStringLiteral("exit"), CompositeExecutionProvider::exitComponentType(), QStringLiteral("exit"),
          QVariantMap{ { QStringLiteral("portKey"), QStringLiteral("out") } } }
    };
    definition.connections = {
        { QStringLiteral("inner-e1"), QStringLiteral("entry"), QStringLiteral("pass"), {} },
        { QStringLiteral("inner-e2"), QStringLiteral("pass"), QStringLiteral("exit"), {} }
    };
    definition.inputMappings = {
        { QStringLiteral("outer-e1"), QStringLiteral("entry"), QStringLiteral("in") }
    };
    definition.outputMappings = {
        { QStringLiteral("exit"), QStringLiteral("out"), QStringLiteral("result") }
    };
    return definition;
}

cme::runtime::CompositeGraphDefinition makeNestedCompositeOuterDefinition()
{
    using namespace cme::runtime;

    CompositeGraphDefinition definition;
    definition.componentTypeId = QStringLiteral("composite/outer");
    definition.components = {
        { QStringLiteral("entry"), CompositeExecutionProvider::entryComponentType(), QStringLiteral("entry"),
          QVariantMap{ { QStringLiteral("portKey"), QStringLiteral("in") } } },
        { QStringLiteral("innerComposite"), QStringLiteral("composite/simple"), QStringLiteral("innerComposite"), {} },
        { QStringLiteral("exit"), CompositeExecutionProvider::exitComponentType(), QStringLiteral("exit"),
          QVariantMap{ { QStringLiteral("portKey"), QStringLiteral("out") } } }
    };
    definition.connections = {
        { QStringLiteral("nest-e1"), QStringLiteral("entry"), QStringLiteral("innerComposite"), {} },
        { QStringLiteral("nest-e2"), QStringLiteral("innerComposite"), QStringLiteral("exit"), {} }
    };
    definition.inputMappings = {
        { QStringLiteral("outer-e1"), QStringLiteral("entry"), QStringLiteral("in") }
    };
    definition.outputMappings = {
        { QStringLiteral("exit"), QStringLiteral("out"), QStringLiteral("result") }
    };
    return definition;
}

GraphModel *buildOuterGraph(QObject *parent, const QString &compositeType)
{
    auto *graph = new GraphModel(parent);
    graph->addComponent(makeComponent(*graph, QStringLiteral("source"), QStringLiteral("outer/source")));
    graph->addComponent(makeComponent(*graph, QStringLiteral("composite"), compositeType));
    graph->addComponent(makeComponent(*graph, QStringLiteral("sink"), QStringLiteral("outer/sink")));

    graph->addConnection(makeConnection(*graph, QStringLiteral("outer-e1"), QStringLiteral("source"), QStringLiteral("composite")));
    graph->addConnection(makeConnection(*graph, QStringLiteral("outer-e2"), QStringLiteral("composite"), QStringLiteral("sink")));
    return graph;
}

} // namespace

class tst_Phase4CompositeExecutionProvider : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void endToEnd_outerGraphToCompositeToSink();
    void nestedComposite_depthTwoDeterministic();
    void recursiveComposite_rejectedWithClearReason();
    void performanceWithinToleranceAgainstBaseline();
};

void tst_Phase4CompositeExecutionProvider::init()
{
    cme::execution::MigrationFlags::resetDefaults();
    cme::execution::MigrationFlags::setTokenTransportEnabled(true);
}

void tst_Phase4CompositeExecutionProvider::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_Phase4CompositeExecutionProvider::endToEnd_outerGraphToCompositeToSink()
{
    QObject owner;
    std::unique_ptr<GraphModel> outerGraph(buildOuterGraph(&owner, QStringLiteral("composite/simple")));

    LeafExecutionProvider leafProvider;
    cme::runtime::CompositeExecutionProvider compositeProvider;
    compositeProvider.setDefinitions({ makeSimpleCompositeDefinition() });
    compositeProvider.setDelegateProviders({ &leafProvider });

    QVERIFY(compositeProvider.isValid());

    GraphExecutionSandbox sandbox;
    sandbox.setGraph(outerGraph.get());
    sandbox.setExecutionSemanticsProviders({ &leafProvider, &compositeProvider });

    QVERIFY(sandbox.start());
    sandbox.run();

    const cme::execution::IncomingTokens sinkIncoming = leafProvider.seenIncoming(QStringLiteral("sink"));
    QCOMPARE(sinkIncoming.size(), 1);
    const QVariantMap compositeOut = sinkIncoming.value(QStringLiteral("outer-e2"));
    const QVariantMap result = compositeOut.value(QStringLiteral("result")).toMap();
    QCOMPARE(result.value(QStringLiteral("value")).toString(), QStringLiteral("source"));
}

void tst_Phase4CompositeExecutionProvider::nestedComposite_depthTwoDeterministic()
{
    QObject owner;
    std::unique_ptr<GraphModel> outerGraph(buildOuterGraph(&owner, QStringLiteral("composite/outer")));

    LeafExecutionProvider leafProvider;
    cme::runtime::CompositeExecutionProvider compositeProvider;
    compositeProvider.setDefinitions({ makeSimpleCompositeDefinition(), makeNestedCompositeOuterDefinition() });
    compositeProvider.setDelegateProviders({ &leafProvider });

    QVERIFY(compositeProvider.isValid());

    GraphExecutionSandbox sandbox;
    sandbox.setGraph(outerGraph.get());
    sandbox.setExecutionSemanticsProviders({ &leafProvider, &compositeProvider });

    QVERIFY(sandbox.start());
    sandbox.run();
    const QVariantMap firstState = sandbox.executionState();
    const QVariantList firstTimeline = sandbox.timeline();

    QVERIFY(sandbox.start());
    sandbox.run();
    const QVariantMap secondState = sandbox.executionState();
    const QVariantList secondTimeline = sandbox.timeline();

    QCOMPARE(secondState, firstState);
    QCOMPARE(secondTimeline, firstTimeline);
}

void tst_Phase4CompositeExecutionProvider::recursiveComposite_rejectedWithClearReason()
{
    using namespace cme::runtime;

    CompositeGraphDefinition recursive;
    recursive.componentTypeId = QStringLiteral("composite/self");
    recursive.components = {
        { QStringLiteral("self"), QStringLiteral("composite/self"), QStringLiteral("self"), {} }
    };

    CompositeExecutionProvider provider;
    provider.setDefinitions({ recursive });

    QVERIFY(!provider.isValid());
    QVERIFY(!provider.diagnostics().isEmpty());
    QCOMPARE(provider.diagnostics().first(),
             QStringLiteral("Composite graph recursion detected: composite/self -> composite/self"));
}

void tst_Phase4CompositeExecutionProvider::performanceWithinToleranceAgainstBaseline()
{
    const int iterations = 40;

    LeafExecutionProvider leafProvider;
    cme::runtime::CompositeExecutionProvider compositeProvider;
    compositeProvider.setDefinitions({ makeSimpleCompositeDefinition() });
    compositeProvider.setDelegateProviders({ &leafProvider });

    QObject baselineOwner;
    GraphModel baselineGraph(&baselineOwner);
    baselineGraph.addComponent(makeComponent(baselineGraph, QStringLiteral("source"), QStringLiteral("outer/source")));
    baselineGraph.addComponent(makeComponent(baselineGraph, QStringLiteral("pass"), QStringLiteral("inner/pass")));
    baselineGraph.addComponent(makeComponent(baselineGraph, QStringLiteral("sink"), QStringLiteral("outer/sink")));
    baselineGraph.addConnection(makeConnection(baselineGraph, QStringLiteral("b1"), QStringLiteral("source"), QStringLiteral("pass")));
    baselineGraph.addConnection(makeConnection(baselineGraph, QStringLiteral("b2"), QStringLiteral("pass"), QStringLiteral("sink")));

    GraphExecutionSandbox baselineSandbox;
    baselineSandbox.setGraph(&baselineGraph);
    baselineSandbox.setExecutionSemanticsProviders({ &leafProvider });

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < iterations; ++i) {
        QVERIFY(baselineSandbox.start());
        baselineSandbox.run();
    }
    const qint64 baselineNs = timer.nsecsElapsed();

    QObject compositeOwner;
    std::unique_ptr<GraphModel> outerGraph(buildOuterGraph(&compositeOwner, QStringLiteral("composite/simple")));
    GraphExecutionSandbox compositeSandbox;
    compositeSandbox.setGraph(outerGraph.get());
    compositeSandbox.setExecutionSemanticsProviders({ &leafProvider, &compositeProvider });

    timer.restart();
    for (int i = 0; i < iterations; ++i) {
        QVERIFY(compositeSandbox.start());
        compositeSandbox.run();
    }
    const qint64 compositeNs = timer.nsecsElapsed();

    QVERIFY(baselineNs > 0);
    QVERIFY(compositeNs > 0);
    QVERIFY2(compositeNs < baselineNs * 25,
             qPrintable(QStringLiteral("Composite execution exceeded tolerance (baseline=%1ns composite=%2ns)")
                            .arg(baselineNs)
                            .arg(compositeNs)));
}

QTEST_MAIN(tst_Phase4CompositeExecutionProvider)
#include "tst_Phase4CompositeExecutionProvider.moc"
