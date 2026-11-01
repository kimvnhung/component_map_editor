#include <QtTest/QtTest>

#include "DeploymentGraphExporter.h"

class DeploymentGraphExporterTests : public QObject
{
    Q_OBJECT

private slots:
    void exportsComponentsRoutesAndProperties();
    void usesDeploymentMetadataFromConnection();
    void rejectsMissingRequiredGraphData();
    void doesNotOverwriteOutputOnFailure();
};

void DeploymentGraphExporterTests::exportsComponentsRoutesAndProperties()
{
    GraphModel graph;
    graph.setId(QStringLiteral("orders"));

    auto *source = new ComponentModel(QStringLiteral("source"), QStringLiteral("Source"), 10.0, 20.0,
                                      QStringLiteral("#fff"), QStringLiteral("producer"));
    source->setDynamicProperty(QStringLiteral("threshold"), 42);
    graph.addComponent(source);

    auto *sink = new ComponentModel(QStringLiteral("sink"), QStringLiteral("Sink"), 30.0, 40.0,
                                     QStringLiteral("#fff"), QStringLiteral("consumer"));
    graph.addComponent(sink);

    auto *connection = new ConnectionModel(QStringLiteral("source-to-sink"), QStringLiteral("source"),
                                           QStringLiteral("sink"));
    graph.addConnection(connection);

    cme::editor::DeploymentGraphExporter::Options options;
    options.graphVersion = QStringLiteral("v1");

    cme::runtime::v1::DeploymentGraph deployment;
    QString error;
    QVERIFY2(cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error),
             qPrintable(error));

    QCOMPARE(QString::fromStdString(deployment.graph_id()), QStringLiteral("orders"));
    QCOMPARE(QString::fromStdString(deployment.graph_version()), QStringLiteral("v1"));
    QCOMPARE(deployment.components_size(), 2);
    QCOMPARE(deployment.routes_size(), 1);
    QCOMPARE(QString::fromStdString(deployment.components(0).component_type()), QStringLiteral("producer"));
    QCOMPARE(QString::fromStdString(deployment.components(0).properties().at("threshold")), QStringLiteral("42"));
    QCOMPARE(deployment.components(0).state_schema_version(), 1U);
    QCOMPARE(QString::fromStdString(deployment.routes(0).source_port_id()), QStringLiteral("out"));
    QCOMPARE(QString::fromStdString(deployment.routes(0).target_port_id()), QStringLiteral("in"));
    QCOMPARE(QString::fromStdString(deployment.routes(0).message_type()), QStringLiteral("event"));
    QVERIFY(deployment.routes(0).enabled());
}

void DeploymentGraphExporterTests::usesDeploymentMetadataFromConnection()
{
    GraphModel graph;
    graph.setId(QStringLiteral("typed-graph"));
    graph.addComponent(new ComponentModel(QStringLiteral("a"), QString(), 0.0, 0.0,
                                           QString(), QStringLiteral("a-type")));
    graph.addComponent(new ComponentModel(QStringLiteral("b"), QString(), 0.0, 0.0,
                                           QString(), QStringLiteral("b-type")));

    auto *connection = new ConnectionModel(QStringLiteral("r"), QStringLiteral("a"), QStringLiteral("b"));
    connection->setProperty("sourcePort", QStringLiteral("result"));
    connection->setProperty("targetPort", QStringLiteral("input"));
    connection->setProperty("messageType", QStringLiteral("orders.created"));
    graph.addConnection(connection);

    cme::editor::DeploymentGraphExporter::Options options;
    options.graphVersion = QStringLiteral("2026-11-01");
    options.defaultSourcePort = QStringLiteral("wrong-out");
    options.defaultTargetPort = QStringLiteral("wrong-in");
    options.defaultMessageType = QStringLiteral("wrong.type");

    cme::runtime::v1::DeploymentGraph deployment;
    QString error;
    QVERIFY2(cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error),
             qPrintable(error));

    const auto &route = deployment.routes(0);
    QCOMPARE(QString::fromStdString(route.source_port_id()), QStringLiteral("result"));
    QCOMPARE(QString::fromStdString(route.target_port_id()), QStringLiteral("input"));
    QCOMPARE(QString::fromStdString(route.message_type()), QStringLiteral("orders.created"));
}

void DeploymentGraphExporterTests::rejectsMissingRequiredGraphData()
{
    GraphModel graph;
    cme::editor::DeploymentGraphExporter::Options options;
    options.graphVersion = QStringLiteral("v1");
    cme::runtime::v1::DeploymentGraph deployment;
    QString error;

    QVERIFY(!cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error));
    QVERIFY(error.contains(QStringLiteral("graph_id")));

    graph.setId(QStringLiteral("graph"));
    options.graphVersion.clear();
    error.clear();
    QVERIFY(!cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error));
    QVERIFY(error.contains(QStringLiteral("graph_version")));
}

void DeploymentGraphExporterTests::doesNotOverwriteOutputOnFailure()
{
    GraphModel graph;
    graph.setId(QStringLiteral("graph"));
    graph.addComponent(new ComponentModel(QStringLiteral("invalid"), QString(), 0.0, 0.0,
                                           QString(), QString()));
    graph.addComponent(new ComponentModel(QStringLiteral("valid"), QString(), 0.0, 0.0,
                                           QString(), QStringLiteral("type")));

    cme::editor::DeploymentGraphExporter::Options options;
    options.graphVersion = QStringLiteral("v1");
    cme::runtime::v1::DeploymentGraph deployment;
    deployment.set_graph_id("previous");
    deployment.set_graph_version("previous-v1");
    QString error;

    QVERIFY(!cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error));
    QCOMPARE(QString::fromStdString(deployment.graph_id()), QStringLiteral("previous"));
    QCOMPARE(QString::fromStdString(deployment.graph_version()), QStringLiteral("previous-v1"));
}

QTEST_MAIN(DeploymentGraphExporterTests)
#include "tst_DeploymentGraphExporter.moc"
