#include <QtTest>

#include <algorithm>
#include <utility>
#include <memory>

#include "component_map_runtime/DeploymentValidator.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"

namespace
{
class NoopHandler final : public cme::runtime::RuntimeHandler
{
public:
    explicit NoopHandler(QString type) : m_type(std::move(type)) {}
    QString componentType() const override { return m_type; }
    bool process(const cme::runtime::RuntimeEvent &, QByteArray &,
                 cme::runtime::RuntimeEvent *, cme::runtime::RuntimeFailure *) override { return true; }
private:
    QString m_type;
};

void addPort(cme::runtime::v1::ComponentInstance *component, const char *id,
             cme::runtime::v1::PortDirection direction)
{
    auto *port = component->add_ports();
    port->set_port_id(id);
    port->set_direction(direction);
    port->set_message_type("fixture.event");
}

cme::runtime::v1::DeploymentGraph fixture()
{
    cme::runtime::v1::DeploymentGraph graph;
    graph.set_graph_id("root");
    graph.set_graph_version("root-v1");

    auto *definition = graph.add_graph_definitions();
    definition->set_graph_id("child");
    definition->set_graph_version("child-v1");
    auto *child = definition->add_components();
    child->set_instance_id("worker");
    child->set_component_type("worker");
    child->set_state_schema_version(1);
    addPort(child, "in", cme::runtime::v1::PORT_DIRECTION_INPUT);
    auto *exposed = definition->add_exposed_ports();
    exposed->set_port_id("in");
    exposed->set_direction(cme::runtime::v1::PORT_DIRECTION_INPUT);
    exposed->set_message_type("fixture.event");
    exposed->set_internal_instance_id("worker");
    exposed->set_internal_port_id("in");

    auto *instance = graph.add_components();
    instance->set_instance_id("child-instance");
    instance->set_composite_graph_id("child");
    instance->set_composite_graph_version("child-v1");
    instance->set_state_schema_version(1);
    addPort(instance, "in", cme::runtime::v1::PORT_DIRECTION_INPUT);
    return graph;
}

void registerWorker(cme::runtime::RuntimeHandlerRegistry *registry)
{
    QVERIFY(registry->registerFactory(QStringLiteral("worker"), [] {
        return std::make_unique<NoopHandler>(QStringLiteral("worker"));
    }));
}
} // namespace

class tst_RuntimeNestedGraphPhase6 : public QObject
{
    Q_OBJECT
private slots:
    void validCompositeReferenceIsAccepted();
    void missingDefinitionIsRejected();
    void dependencyCycleIsRejected();
};

void tst_RuntimeNestedGraphPhase6::validCompositeReferenceIsAccepted()
{
    auto graph = fixture();
    cme::runtime::RuntimeHandlerRegistry registry;
    registerWorker(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY2(result.valid(), qPrintable(QString::fromStdString(result.DebugString())));
}

void tst_RuntimeNestedGraphPhase6::missingDefinitionIsRejected()
{
    auto graph = fixture();
    graph.clear_graph_definitions();
    cme::runtime::RuntimeHandlerRegistry registry;
    registerWorker(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QVERIFY(std::any_of(result.issues().begin(), result.issues().end(), [](const auto &issue) {
        return issue.code() == "GRAPH_DEFINITION_NOT_FOUND";
    }));
}

void tst_RuntimeNestedGraphPhase6::dependencyCycleIsRejected()
{
    auto graph = fixture();
    auto *definition = graph.add_graph_definitions();
    definition->set_graph_id("cycle");
    definition->set_graph_version("v1");
    auto *child = definition->add_components();
    child->set_instance_id("self");
    child->set_composite_graph_id("cycle");
    child->set_composite_graph_version("v1");
    child->set_state_schema_version(1);
    cme::runtime::RuntimeHandlerRegistry registry;
    registerWorker(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QVERIFY(std::any_of(result.issues().begin(), result.issues().end(), [](const auto &issue) {
        return issue.code() == "GRAPH_DEPENDENCY_CYCLE";
    }));
}

QTEST_MAIN(tst_RuntimeNestedGraphPhase6)
#include "tst_RuntimeNestedGraphPhase6.moc"
