#include <QtTest/QtTest>

#include "DeploymentGraphExporter.h"

class DeploymentGraphExporterTests : public QObject
{
    Q_OBJECT

private slots:
    void exportsComponentsRoutesAndProperties();
    void usesDeploymentMetadataFromConnection();
    void rejectsMissingRequiredGraphData();
    void rejectsRoutesWithoutExplicitMetadata();
    void exportsCompositeReference();
    void doesNotOverwriteOutputOnFailure();
};

void DeploymentGraphExporterTests::exportsComponentsRoutesAndProperties()
{
    GraphModel graph;
    graph.setId(QStringLiteral("orders"));

    auto *source = new ComponentModel(QStringLiteral("source"), QStringLiteral("Source"), 10.0, 20.0,
                                      QStringLiteral("#fff"), QStringLiteral("producer"));
    source->setDynamicProperty(QStringLiteral("threshold"), 42);
    source->setDynamicProperty(QStringLiteral("outputPorts"), QVariantList{
        QVariantMap{{QStringLiteral("id"), QStringLiteral("out")},
                    {QStringLiteral("messageType"), QStringLiteral("orders.created")}}});
    graph.addComponent(source);

    auto *sink = new ComponentModel(QStringLiteral("sink"), QStringLiteral("Sink"), 30.0, 40.0,
                                     QStringLiteral("#fff"), QStringLiteral("consumer"));
    sink->setDynamicProperty(QStringLiteral("inputPorts"), QVariantList{
        QVariantMap{{QStringLiteral("id"), QStringLiteral("in")},
                    {QStringLiteral("messageType"), QStringLiteral("orders.created")}}});
    graph.addComponent(sink);

    auto *connection = new ConnectionModel(QStringLiteral("source-to-sink"), QStringLiteral("source"),
                                           QStringLiteral("sink"));
    connection->setProperty("sourcePort", QStringLiteral("out"));
    connection->setProperty("targetPort", QStringLiteral("in"));
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

    const auto &sourceComponent = deployment.components(0);
    QCOMPARE(QString::fromStdString(sourceComponent.component_type()), QStringLiteral("producer"));
    QCOMPARE(QString::fromStdString(sourceComponent.properties().at("threshold")), QStringLiteral("42"));
    QCOMPARE(sourceComponent.state_schema_version(), 1U);
    QCOMPARE(sourceComponent.ports_size(), 1);
    QCOMPARE(QString::fromStdString(sourceComponent.ports(0).port_id()), QStringLiteral("out"));
    QCOMPARE(sourceComponent.ports(0).direction(), cme::runtime::v1::PORT_DIRECTION_OUTPUT);
    QCOMPARE(QString::fromStdString(sourceComponent.ports(0).message_type()), QStringLiteral("orders.created"));

    const auto &sinkComponent = deployment.components(1);
    QCOMPARE(sinkComponent.ports_size(), 1);
    QCOMPARE(QString::fromStdString(sinkComponent.ports(0).port_id()), QStringLiteral("in"));
    QCOMPARE(sinkComponent.ports(0).direction(), cme::runtime::v1::PORT_DIRECTION_INPUT);

    QCOMPARE(QString::fromStdString(deployment.routes(0).source_port_id()), QStringLiteral("out"));
    QCOMPARE(QString::fromStdString(deployment.routes(0).target_port_id()), QStringLiteral("in"));
    QCOMPARE(QString::fromStdString(deployment.routes(0).message_type()), QStringLiteral("orders.created"));
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

    cme::runtime::v1::DeploymentGraph deployment;
    QString error;
    QVERIFY2(cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error),
             qPrintable(error));

    const auto &route = deployment.routes(0);
    QCOMPARE(QString::fromStdString(route.source_port_id()), QStringLiteral("result"));
    QCOMPARE(QString::fromStdString(route.target_port_id()), QStringLiteral("input"));
    QCOMPARE(QString::fromStdString(route.message_type()), QStringLiteral("orders.created"));
}

void DeploymentGraphExporterTests::rejectsRoutesWithoutExplicitMetadata()
{
    GraphModel graph;
    graph.setId(QStringLiteral("strict-graph"));
    auto *source = new ComponentModel(QStringLiteral("a"), QString(), 0.0, 0.0,
                                      QString(), QStringLiteral("a-type"));
    source->setDynamicProperty(QStringLiteral("outputPorts"),
                               QVariantList{QVariantMap{{QStringLiteral("id"), QStringLiteral("out")},
                                                        {QStringLiteral("messageType"),
                                                         QStringLiteral("orders.created")}}});
    auto *sink = new ComponentModel(QStringLiteral("b"), QString(), 0.0, 0.0,
                                    QString(), QStringLiteral("b-type"));
    sink->setDynamicProperty(QStringLiteral("inputPorts"),
                             QVariantList{QVariantMap{{QStringLiteral("id"), QStringLiteral("in")},
                                                      {QStringLiteral("messageType"),
                                                       QStringLiteral("orders.created")}}});
    graph.addComponent(source);
    graph.addComponent(sink);
    graph.addConnection(new ConnectionModel(QStringLiteral("r"), QStringLiteral("a"), QStringLiteral("b")));

    cme::editor::DeploymentGraphExporter::Options options;
    options.graphVersion = QStringLiteral("v1");
    cme::runtime::v1::DeploymentGraph deployment;
    QString error;

    QVERIFY(!cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error));
    QVERIFY(error.contains(QStringLiteral("source_port_id")));

    options.requireExplicitRouteMetadata = false;
    error.clear();
    QVERIFY2(cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error),
             qPrintable(error));
    QCOMPARE(deployment.routes_size(), 1);
    QCOMPARE(QString::fromStdString(deployment.routes(0).source_port_id()), QStringLiteral("out"));
    QCOMPARE(QString::fromStdString(deployment.routes(0).target_port_id()), QStringLiteral("in"));
    QCOMPARE(QString::fromStdString(deployment.routes(0).message_type()),
             QStringLiteral("orders.created"));
}

void DeploymentGraphExporterTests::exportsCompositeReference()
{
    GraphModel graph;
    graph.setId(QStringLiteral("parent"));
    auto *child = new ComponentModel(QStringLiteral("child"), QString(), 0.0, 0.0,
                                      QString(), QStringLiteral("composite-scope"));
    child->setDynamicProperty(QStringLiteral("compositeGraphId"), QStringLiteral("child-graph"));
    child->setDynamicProperty(QStringLiteral("compositeGraphVersion"), QStringLiteral("3"));
    graph.addComponent(child);

    cme::editor::DeploymentGraphExporter::Options options;
    options.graphVersion = QStringLiteral("v1");
    cme::runtime::v1::DeploymentGraph deployment;
    QString error;
    QVERIFY2(cme::editor::DeploymentGraphExporter::exportGraph(&graph, options, &deployment, &error),
             qPrintable(error));

    QCOMPARE(QString::fromStdString(deployment.components(0).composite_graph_id()), QStringLiteral("child-graph"));
    QCOMPARE(QString::fromStdString(deployment.components(0).composite_graph_version()), QStringLiteral("3"));
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
