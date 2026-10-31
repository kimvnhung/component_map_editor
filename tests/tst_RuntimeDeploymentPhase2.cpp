#include <QtTest>

#include <algorithm>
#include <memory>

#include "component_map_runtime/DeploymentManager.h"
#include "component_map_runtime/DeploymentValidator.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"
#include "component_map_runtime/RuntimeService.h"

namespace
{

class NoopHandler final : public cme::runtime::RuntimeHandler
{
public:
    explicit NoopHandler(QString type)
        : m_type(std::move(type))
    {
    }

    QString componentType() const override { return m_type; }
    bool process(const cme::runtime::RuntimeEvent &, QByteArray &,
                 cme::runtime::RuntimeEvent *, cme::runtime::RuntimeFailure *) override
    {
        return true;
    }

private:
    QString m_type;
};

cme::runtime::v1::DeploymentGraph validFixture()
{
    cme::runtime::v1::DeploymentGraph graph;
    graph.set_graph_version("graph-v1");

    auto *producer = graph.add_components();
    producer->set_instance_id("producer-01");
    producer->set_component_type("producer");
    producer->set_state_schema_version(1);
    auto *producerPort = producer->add_ports();
    producerPort->set_port_id("out");
    producerPort->set_direction(cme::runtime::v1::PORT_DIRECTION_OUTPUT);
    producerPort->set_message_type("fixture.event");

    auto *processor = graph.add_components();
    processor->set_instance_id("processor-01");
    processor->set_component_type("processor");
    processor->set_state_schema_version(2);
    auto *processorIn = processor->add_ports();
    processorIn->set_port_id("in");
    processorIn->set_direction(cme::runtime::v1::PORT_DIRECTION_INPUT);
    processorIn->set_message_type("fixture.event");
    auto *processorOut = processor->add_ports();
    processorOut->set_port_id("out");
    processorOut->set_direction(cme::runtime::v1::PORT_DIRECTION_OUTPUT);
    processorOut->set_message_type("fixture.event");

    auto *sink = graph.add_components();
    sink->set_instance_id("sink-01");
    sink->set_component_type("sink");
    sink->set_state_schema_version(1);
    auto *sinkIn = sink->add_ports();
    sinkIn->set_port_id("in");
    sinkIn->set_direction(cme::runtime::v1::PORT_DIRECTION_INPUT);
    sinkIn->set_message_type("fixture.event");

    auto *firstRoute = graph.add_routes();
    firstRoute->set_connection_id("connection-01");
    firstRoute->set_source_instance_id("producer-01");
    firstRoute->set_source_port_id("out");
    firstRoute->set_target_instance_id("processor-01");
    firstRoute->set_target_port_id("in");
    firstRoute->set_message_type("fixture.event");
    firstRoute->set_enabled(true);

    auto *secondRoute = graph.add_routes();
    secondRoute->set_connection_id("connection-02");
    secondRoute->set_source_instance_id("processor-01");
    secondRoute->set_source_port_id("out");
    secondRoute->set_target_instance_id("sink-01");
    secondRoute->set_target_port_id("in");
    secondRoute->set_message_type("fixture.event");
    secondRoute->set_enabled(true);
    return graph;
}

void registerHandlers(cme::runtime::RuntimeHandlerRegistry *registry)
{
    QVERIFY(registry->registerFactory(QStringLiteral("producer"),
                                      [] { return std::make_unique<NoopHandler>(QStringLiteral("producer")); }));
    QVERIFY(registry->registerFactory(QStringLiteral("processor"),
                                      [] { return std::make_unique<NoopHandler>(QStringLiteral("processor")); }));
    QVERIFY(registry->registerFactory(QStringLiteral("sink"),
                                      [] { return std::make_unique<NoopHandler>(QStringLiteral("sink")); }));
}

} // namespace

class tst_RuntimeDeploymentPhase2 : public QObject
{
    Q_OBJECT

private slots:
    void validFixtureRoundTripsAndValidates();
    void invalidRouteReturnsStableIssueCode();
    void duplicateIdsAndDirectionReturnStableIssueCodes();
    void messageTypeMismatchReturnsStableIssueCode();
    void missingHandlerIsRejected();
    void activationRequiresStoppedRuntime();
    void activeGraphVersionCannotBeRedeployed();
    void clonedInstanceKeepsIndependentConfiguration();

private:
    static bool hasIssueCode(const cme::runtime::v1::DeploymentValidationResult &result,
                             const char *code);
};

void tst_RuntimeDeploymentPhase2::validFixtureRoundTripsAndValidates()
{
    const auto original = validFixture();
    std::string bytes;
    QVERIFY(original.SerializeToString(&bytes));
    cme::runtime::v1::DeploymentGraph restored;
    QVERIFY(restored.ParseFromString(bytes));
    QCOMPARE(restored.graph_version(), std::string("graph-v1"));
    QCOMPARE(restored.components_size(), 3);
    QCOMPARE(restored.routes_size(), 2);

    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(restored);
    QVERIFY(result.valid());
    QCOMPARE(result.issues_size(), 0);
}

bool tst_RuntimeDeploymentPhase2::hasIssueCode(
    const cme::runtime::v1::DeploymentValidationResult &result, const char *code)
{
    return std::any_of(result.issues().begin(), result.issues().end(),
                       [code](const auto &issue) { return issue.code() == code; });
}

void tst_RuntimeDeploymentPhase2::invalidRouteReturnsStableIssueCode()
{
    auto graph = validFixture();
    graph.mutable_routes(0)->set_target_port_id("missing");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QVERIFY(hasIssueCode(result, "TARGET_PORT_NOT_FOUND"));
}

void tst_RuntimeDeploymentPhase2::duplicateIdsAndDirectionReturnStableIssueCodes()
{
    auto graph = validFixture();
    graph.mutable_components(1)->set_instance_id("producer-01");
    graph.mutable_routes(0)->set_source_port_id("in");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QVERIFY(hasIssueCode(result, "DUPLICATE_INSTANCE_ID"));
    QVERIFY(hasIssueCode(result, "SOURCE_PORT_NOT_FOUND"));
}

void tst_RuntimeDeploymentPhase2::messageTypeMismatchReturnsStableIssueCode()
{
    auto graph = validFixture();
    graph.mutable_routes(0)->set_message_type("other.event");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QVERIFY(hasIssueCode(result, "SOURCE_MESSAGE_TYPE_MISMATCH"));
    QVERIFY(hasIssueCode(result, "TARGET_MESSAGE_TYPE_MISMATCH"));
}

void tst_RuntimeDeploymentPhase2::missingHandlerIsRejected()
{
    auto graph = validFixture();
    graph.mutable_components(1)->set_component_type("unknown");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QCOMPARE(result.issues(0).code(), std::string("HANDLER_NOT_FOUND"));
}

void tst_RuntimeDeploymentPhase2::activeGraphVersionCannotBeRedeployed()
{
    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id("phase2-runtime");
    descriptor.set_protocol_version("1.0");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);
    cme::runtime::RuntimeService service(descriptor);
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    cme::runtime::DeploymentManager manager(&service, &registry);
    QVERIFY(manager.deploy(validFixture()).valid());
    QVERIFY(manager.activate().valid());
    const auto duplicate = manager.deploy(validFixture());
    QVERIFY(!duplicate.valid());
    QCOMPARE(duplicate.issues(0).code(), std::string("GRAPH_VERSION_ALREADY_ACTIVE"));
}

void tst_RuntimeDeploymentPhase2::clonedInstanceKeepsIndependentConfiguration()
{
    auto graph = validFixture();
    auto *clone = graph.add_components();
    clone->CopyFrom(graph.components(0));
    clone->set_instance_id("producer-02");
    (*clone->mutable_properties())["seed"] = "clone";
    (*graph.mutable_components(0)->mutable_properties())["seed"] = "original";

    QCOMPARE(graph.components(0).component_type(), graph.components(3).component_type());
    QCOMPARE(graph.components(0).instance_id(), std::string("producer-01"));
    QCOMPARE(graph.components(3).instance_id(), std::string("producer-02"));
    QCOMPARE(graph.components(0).properties().at("seed"), std::string("original"));
    QCOMPARE(graph.components(3).properties().at("seed"), std::string("clone"));
}

void tst_RuntimeDeploymentPhase2::activationRequiresStoppedRuntime()
{
    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id("phase2-runtime");
    descriptor.set_protocol_version("1.0");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);
    cme::runtime::RuntimeService service(descriptor);
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    cme::runtime::DeploymentManager manager(&service, &registry);
    QVERIFY(manager.deploy(validFixture()).valid());

    QString error;
    QVERIFY(service.start(&error));
    const auto rejected = manager.activate();
    QVERIFY(!rejected.valid());
    QCOMPARE(rejected.issues(0).code(), std::string("RUNTIME_MUST_BE_STOPPED"));

    QVERIFY(service.stop(&error));
    QVERIFY(manager.activate().valid());
    QVERIFY(manager.hasActiveGraph());
    QCOMPARE(manager.activeGraph().graph_version(), std::string("graph-v1"));
}

QTEST_MAIN(tst_RuntimeDeploymentPhase2)
#include "tst_RuntimeDeploymentPhase2.moc"
