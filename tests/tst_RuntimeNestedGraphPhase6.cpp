#include <QtTest>

#include <algorithm>
#include <memory>
#include <utility>

#include "component_map_runtime/DeploymentValidator.h"
#include "component_map_runtime/FlattenCompiler.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"
#include "component_map_runtime/RuntimePersistence.h"
#include <QTemporaryDir>

namespace
{
using namespace cme::runtime;
using namespace cme::runtime::v1;

class NoopHandler final : public RuntimeHandler
{
public:
    explicit NoopHandler(QString type) : m_type(std::move(type)) {}
    QString componentType() const override { return m_type; }
    bool process(const RuntimeEvent &, QByteArray &, RuntimeEvent *, RuntimeFailure *) override { return true; }
private:
    QString m_type;
};

void addPort(ComponentInstance *component, const char *id, PortDirection direction)
{
    auto *port = component->add_ports();
    port->set_port_id(id);
    port->set_direction(direction);
    port->set_message_type("fixture.event");
}

void addRoute(DeploymentGraph *graph, const char *id, const char *source, const char *sourcePort,
              const char *target, const char *targetPort)
{
    auto *route = graph->add_routes();
    route->set_connection_id(id);
    route->set_source_instance_id(source);
    route->set_source_port_id(sourcePort);
    route->set_target_instance_id(target);
    route->set_target_port_id(targetPort);
    route->set_message_type("fixture.event");
}

DeploymentGraph flatGraph()
{
    DeploymentGraph graph;
    graph.set_graph_id("root");
    graph.set_graph_version("root-v1");
    auto *source = graph.add_components();
    source->set_instance_id("source");
    source->set_component_type("source");
    source->set_state_schema_version(1);
    addPort(source, "out", PORT_DIRECTION_OUTPUT);
    auto *target = graph.add_components();
    target->set_instance_id("target");
    target->set_component_type("target");
    target->set_state_schema_version(1);
    addPort(target, "in", PORT_DIRECTION_INPUT);
    addRoute(&graph, "source-target", "source", "out", "target", "in");
    return graph;
}

DeploymentGraph compositeGraph()
{
    DeploymentGraph graph;
    graph.set_graph_id("root");
    graph.set_graph_version("root-v1");

    auto *definition = graph.add_graph_definitions();
    definition->set_graph_id("child");
    definition->set_graph_version("child-v1");
    auto *worker = definition->add_components();
    worker->set_instance_id("worker");
    worker->set_component_type("worker");
    worker->set_state_schema_version(1);
    addPort(worker, "in", PORT_DIRECTION_INPUT);
    auto *exposed = definition->add_exposed_ports();
    exposed->set_port_id("in");
    exposed->set_direction(PORT_DIRECTION_INPUT);
    exposed->set_message_type("fixture.event");
    exposed->set_internal_instance_id("worker");
    exposed->set_internal_port_id("in");

    auto *source = graph.add_components();
    source->set_instance_id("source");
    source->set_component_type("source");
    source->set_state_schema_version(1);
    addPort(source, "out", PORT_DIRECTION_OUTPUT);
    auto *instance = graph.add_components();
    instance->set_instance_id("child-instance");
    instance->set_composite_graph_id("child");
    instance->set_composite_graph_version("child-v1");
    instance->set_state_schema_version(1);
    addPort(instance, "in", PORT_DIRECTION_INPUT);
    addRoute(&graph, "source-child", "source", "out", "child-instance", "in");
    return graph;
}

void registerWorker(RuntimeHandlerRegistry *registry)
{
    QVERIFY(registry->registerFactory(QStringLiteral("worker"), [] {
        return std::make_unique<NoopHandler>(QStringLiteral("worker"));
    }));
    QVERIFY(registry->registerFactory(QStringLiteral("source"), [] {
        return std::make_unique<NoopHandler>(QStringLiteral("source"));
    }));
}

bool hasIssue(const FlattenResult &result, const QString &code)
{
    return std::any_of(result.issues.cbegin(), result.issues.cend(), [&](const auto &item) {
        return item.code == code;
    });
}
}

class tst_RuntimeNestedGraphPhase6 : public QObject
{
    Q_OBJECT
private slots:
    void validCompositeReferenceIsAccepted();
    void missingDefinitionIsRejected();
    void dependencyCycleIsRejected();
    void flatGraphCompilesDeterministically();
    void compositeBoundaryCompilesToLeafRoute();
    void snapshotAndQualifiedStateRemainIndependent();
};

void tst_RuntimeNestedGraphPhase6::snapshotAndQualifiedStateRemainIndependent()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    RuntimePersistence persistence;
    QString error;
    QVERIFY2(persistence.open(directory.filePath(QStringLiteral("nested.sqlite")),
                              QStringLiteral("nested-runtime"), &error), qPrintable(error));

    DeploymentSnapshot snapshot;
    snapshot.deploymentId = QStringLiteral("deployment-1");
    snapshot.rootGraphId = QStringLiteral("root");
    snapshot.rootGraphVersion = QStringLiteral("root-v1");
    snapshot.dependencyManifest = QStringLiteral("child@v1");
    snapshot.definitionHashes = QStringLiteral("child@v1=hash");
    snapshot.executionPlanHash = QStringLiteral("plan-hash");
    snapshot.serializedDefinitions = QByteArray("definitions");
    QVERIFY2(persistence.saveDeploymentSnapshot(snapshot, &error), qPrintable(error));

    QVERIFY(persistence.saveComponentState(QStringLiteral("deployment-1"),
                                           QStringLiteral("root/one/worker"),
                                           QStringLiteral("worker"), QByteArray("one"), 1, &error));
    QVERIFY(persistence.saveComponentState(QStringLiteral("deployment-1"),
                                           QStringLiteral("root/two/worker"),
                                           QStringLiteral("worker"), QByteArray("two"), 1, &error));
    const auto records = persistence.loadComponentStateRecords(&error);
    QCOMPARE(records.size(), 2);
    QCOMPARE(records.at(0).deploymentId, QStringLiteral("deployment-1"));
    QVERIFY(records.at(0).qualifiedPath != records.at(1).qualifiedPath);

    DeploymentSnapshot restored;
    QVERIFY2(persistence.loadDeploymentSnapshot(QStringLiteral("deployment-1"), &restored, &error),
             qPrintable(error));
    QCOMPARE(restored.executionPlanHash, QStringLiteral("plan-hash"));
    QCOMPARE(restored.serializedDefinitions, QByteArray("definitions"));
}



void tst_RuntimeNestedGraphPhase6::validCompositeReferenceIsAccepted()
{
    auto graph = compositeGraph();
    RuntimeHandlerRegistry registry;
    registerWorker(&registry);
    const auto result = DeploymentValidator(&registry).validate(graph);
    QVERIFY2(result.valid(), qPrintable(QString::fromStdString(result.DebugString())));
}

void tst_RuntimeNestedGraphPhase6::missingDefinitionIsRejected()
{
    auto graph = compositeGraph();
    graph.clear_graph_definitions();
    RuntimeHandlerRegistry registry;
    registerWorker(&registry);
    const auto result = DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QVERIFY(std::any_of(result.issues().begin(), result.issues().end(), [](const auto &item) {
        return item.code() == "GRAPH_DEFINITION_NOT_FOUND";
    }));
}

void tst_RuntimeNestedGraphPhase6::dependencyCycleIsRejected()
{
    auto graph = compositeGraph();
    auto *definition = graph.add_graph_definitions();
    definition->set_graph_id("cycle");
    definition->set_graph_version("v1");
    auto *child = definition->add_components();
    child->set_instance_id("self");
    child->set_composite_graph_id("cycle");
    child->set_composite_graph_version("v1");
    child->set_state_schema_version(1);
    RuntimeHandlerRegistry registry;
    registerWorker(&registry);
    const auto result = DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QVERIFY(std::any_of(result.issues().begin(), result.issues().end(), [](const auto &item) {
        return item.code() == "GRAPH_DEPENDENCY_CYCLE";
    }));
}

void tst_RuntimeNestedGraphPhase6::flatGraphCompilesDeterministically()
{
    const auto graph = flatGraph();
    const auto first = FlattenCompiler().compile(graph);
    const auto second = FlattenCompiler().compile(graph);
    QVERIFY2(first.valid, qPrintable(first.issues.isEmpty() ? QString() : first.issues.first().message));
    QCOMPARE(first.plan.leaves.size(), 2);
    QCOMPARE(first.plan.routes.size(), 1);
    QCOMPARE(first.plan.hash, second.plan.hash);
    QCOMPARE(first.plan.leaves.at(0).qualifiedPath, QStringLiteral("root/source"));
}

void tst_RuntimeNestedGraphPhase6::compositeBoundaryCompilesToLeafRoute()
{
    const auto result = FlattenCompiler().compile(compositeGraph());
    QVERIFY2(result.valid, qPrintable(result.issues.isEmpty() ? QString() : result.issues.first().message));
    QCOMPARE(result.plan.leaves.size(), 2);
    QCOMPARE(result.plan.routes.size(), 1);
    QCOMPARE(result.plan.routes.first().sourceLeafPath, QStringLiteral("root/source"));
    QCOMPARE(result.plan.routes.first().targetLeafPath, QStringLiteral("root/child-instance/worker"));
}

QTEST_MAIN(tst_RuntimeNestedGraphPhase6)
#include "tst_RuntimeNestedGraphPhase6.moc"
