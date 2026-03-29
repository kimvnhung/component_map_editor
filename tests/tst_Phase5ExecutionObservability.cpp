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

class ObservabilityExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase5.observability");
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

    bool executeComponent(const QString &, const QString &, const QVariantMap &, const QVariantMap &,
                          QVariantMap *, QVariantMap *, QString *error) const override
    {
        if (error)
            *error = QStringLiteral("Phase 5 observability tests require v2 execution path.");
        return false;
    }

    bool executeComponentV2(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override
    {
        Q_UNUSED(error)

        QVariantMap out;
        if (componentType == QStringLiteral("source")
            || componentType == QStringLiteral("sourceA")
            || componentType == QStringLiteral("sourceB")) {
            out.insert(QStringLiteral("value"), componentId);
            out.insert(QStringLiteral("secret"), QStringLiteral("s3cr3t-%1").arg(componentId));
            out.insert(QStringLiteral("token"), QStringLiteral("tok-%1").arg(componentId));
        } else if (componentType == QStringLiteral("sink") || componentType == QStringLiteral("merge")) {
            QVariantMap merged;
            QStringList tokenIds = incomingTokens.keys();
            std::sort(tokenIds.begin(), tokenIds.end());
            for (const QString &tokenId : tokenIds)
                merged.insert(tokenId, incomingTokens.value(tokenId));
            out = merged;
        }

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            trace->insert(QStringLiteral("provider"), providerId());
        return true;
    }
};

} // namespace

class tst_Phase5ExecutionObservability : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void debugSnapshot_returnsStableStructure();
    void telemetry_incrementsForFanOutAndFanIn();
    void redaction_masksSensitiveFieldsInTimelineAndSnapshot();
};

void tst_Phase5ExecutionObservability::init()
{
    cme::execution::MigrationFlags::resetDefaults();
    cme::execution::MigrationFlags::setTokenTransportEnabled(true);
}

void tst_Phase5ExecutionObservability::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_Phase5ExecutionObservability::debugSnapshot_returnsStableStructure()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("S"), QStringLiteral("source")));
    graph.addComponent(makeComponent(graph, QStringLiteral("L"), QStringLiteral("sink")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("S"), QStringLiteral("L")));

    ObservabilityExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    sandbox.run();

    const QVariantMap snapshot = sandbox.debugSnapshot();
    QStringList topKeys = snapshot.keys();
    std::sort(topKeys.begin(), topKeys.end());
    QCOMPARE(topKeys, QStringList({
        QStringLiteral("components"),
        QStringLiteral("connections"),
        QStringLiteral("currentTick"),
        QStringLiteral("redactedFieldCountPreview"),
        QStringLiteral("sensitiveDebugKeys"),
        QStringLiteral("status"),
        QStringLiteral("telemetry"),
        QStringLiteral("tokenTransportEnabled")
    }));

    const QVariantList components = snapshot.value(QStringLiteral("components")).toList();
    const QVariantList connections = snapshot.value(QStringLiteral("connections")).toList();
    QCOMPARE(components.size(), 2);
    QCOMPARE(connections.size(), 1);

    const QVariantMap connectionEntry = connections.first().toMap();
    QStringList connectionKeys = connectionEntry.keys();
    std::sort(connectionKeys.begin(), connectionKeys.end());
    QCOMPARE(connectionKeys, QStringList({
        QStringLiteral("connectionId"),
        QStringLiteral("label"),
        QStringLiteral("payloadBytes"),
        QStringLiteral("payloadSummary"),
        QStringLiteral("sourceId"),
        QStringLiteral("targetId")
    }));
}

void tst_Phase5ExecutionObservability::telemetry_incrementsForFanOutAndFanIn()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("A"), QStringLiteral("sourceA")));
    graph.addComponent(makeComponent(graph, QStringLiteral("B"), QStringLiteral("sourceB")));
    graph.addComponent(makeComponent(graph, QStringLiteral("M"), QStringLiteral("merge")));
    graph.addConnection(makeConnection(graph, QStringLiteral("eA"), QStringLiteral("A"), QStringLiteral("M")));
    graph.addConnection(makeConnection(graph, QStringLiteral("eB"), QStringLiteral("B"), QStringLiteral("M")));

    ObservabilityExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sandbox.start());
    sandbox.run();

    const QVariantMap telemetry = sandbox.executionTelemetry();
    QCOMPARE(telemetry.value(QStringLiteral("tokenWriteCount")).toInt(), 2);
    QCOMPARE(telemetry.value(QStringLiteral("tokenReadCount")).toInt(), 2);
    QVERIFY(telemetry.value(QStringLiteral("payloadBytesWritten")).toLongLong() > 0);
    QVERIFY(telemetry.value(QStringLiteral("payloadBytesRead")).toLongLong() > 0);
    QVERIFY(telemetry.value(QStringLiteral("maxPayloadBytes")).toLongLong() > 0);
}

void tst_Phase5ExecutionObservability::redaction_masksSensitiveFieldsInTimelineAndSnapshot()
{
    GraphModel graph;
    graph.addComponent(makeComponent(graph, QStringLiteral("S"), QStringLiteral("source")));
    graph.addComponent(makeComponent(graph, QStringLiteral("L"), QStringLiteral("sink")));
    graph.addConnection(makeConnection(graph, QStringLiteral("e1"), QStringLiteral("S"), QStringLiteral("L")));

    ObservabilityExecutionProvider provider;
    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);
    sandbox.setExecutionSemanticsProviders({ &provider });
    sandbox.setSensitiveDebugKeys({ QStringLiteral("secret"), QStringLiteral("token") });

    QVERIFY(sandbox.start());
    sandbox.run();

    const QVariantMap snapshot = sandbox.debugSnapshot();
    const QVariantMap connectionEntry = snapshot.value(QStringLiteral("connections")).toList().first().toMap();
    const QVariantMap payloadSummary = connectionEntry.value(QStringLiteral("payloadSummary")).toMap();
    QCOMPARE(payloadSummary.value(QStringLiteral("secret")).toString(), QStringLiteral("<redacted>"));
    QCOMPARE(payloadSummary.value(QStringLiteral("token")).toString(), QStringLiteral("<redacted>"));
    QVERIFY(payloadSummary.value(QStringLiteral("secret")).toString() != QStringLiteral("s3cr3t-S"));

    bool foundStep = false;
    for (const QVariant &entryVar : sandbox.timeline()) {
        const QVariantMap entry = entryVar.toMap();
        if (entry.value(QStringLiteral("event")).toString() != QStringLiteral("stepExecuted"))
            continue;
        const QVariantMap summary = entry.value(QStringLiteral("outputPayloadSummary")).toMap();
        if (summary.contains(QStringLiteral("secret"))) {
            foundStep = true;
            QCOMPARE(summary.value(QStringLiteral("secret")).toString(), QStringLiteral("<redacted>"));
            QCOMPARE(summary.value(QStringLiteral("token")).toString(), QStringLiteral("<redacted>"));
        }
    }

    QVERIFY(foundStep);
    QVERIFY(sandbox.executionTelemetry().value(QStringLiteral("redactedFieldCount")).toInt() >= 2);
}

QTEST_MAIN(tst_Phase5ExecutionObservability)
#include "tst_Phase5ExecutionObservability.moc"
