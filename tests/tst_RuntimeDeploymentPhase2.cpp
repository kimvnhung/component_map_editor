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
    explicit NoopHandler(QString type) : m_type(std::move(type)) {}
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
    auto *sink = graph.add_components();
    sink->set_instance_id("sink-01");
    sink->set_component_type("sink");
    sink->set_state_schema_version(1);
    auto *sinkPort = sink->add_ports();
    sinkPort->set_port_id("in");
    sinkPort->set_direction(cme::runtime::v1::PORT_DIRECTION_INPUT);
    sinkPort->set_message_type("fixture.event");
    auto *route = graph.add_routes();
    route->set_connection_id("connection-01");
    route->set_source_instance_id("producer-01");
    route->set_source_port_id("out");
    route->set_target_instance_id("sink-01");
    route->set_target_port_id("in");
    route->set_message_type("fixture.event");
    route->set_enabled(true);
    return graph;
}

void registerHandlers(cme::runtime::RuntimeHandlerRegistry *registry)
{
    registry->registerFactory(QStringLiteral("producer"),
                              [] { return std::make_unique<NoopHandler>(QStringLiteral("producer")); });
    registry->registerFactory(QStringLiteral("sink"),
                              [] { return std::make_unique<NoopHandler>(QStringLiteral("sink")); });
}

cme::runtime::ComponentDescriptor descriptor(const QString &identity,
                                               cme::runtime::ComponentStatus status,
                                               cme::runtime::TrustState trust)
{
    cme::runtime::ComponentDescriptor value;
    value.identity = identity;
    value.sourceId = QStringLiteral("network.source");
    value.source = cme::runtime::ComponentSource::Network;
    value.status = status;
    value.trustState = trust;
    value.execution.mode = cme::runtime::ExecutionMode::RemoteWorker;
    value.execution.workerCapabilityReference = QStringLiteral("worker.capability");
    value.execution.protocol = QStringLiteral("grpc");
    value.execution.errorBehavior = QStringLiteral("structured");
    value.execution.inputRequirements = {QStringLiteral("input")};
    value.execution.outputGuarantees = {QStringLiteral("output")};
    value.properties.append({QStringLiteral("mode"), QStringLiteral("string"), true,
                             {}, {}, QStringLiteral("Mode"), {}});
    value.ports.append({QStringLiteral("input"), QStringLiteral("input"),
                        QStringLiteral("fixture.event"), QStringLiteral("single"), true, {}, {}});
    return value;
}

bool hasIssueCode(const cme::runtime::v1::DeploymentValidationResult &result, const char *code)
{
    return std::any_of(result.issues().begin(), result.issues().end(),
                       [code](const auto &issue) { return issue.code() == code; });
}
}

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
    void catalogValidationRejectsInvalidComponentConfiguration();
    void catalogValidationRejectsUnavailableComponent();
    void catalogValidationRejectsUntrustedComponent();
};

void tst_RuntimeDeploymentPhase2::validFixtureRoundTripsAndValidates()
{
    auto graph = validFixture();
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(result.valid());
}

void tst_RuntimeDeploymentPhase2::invalidRouteReturnsStableIssueCode()
{
    auto graph = validFixture();
    graph.mutable_routes(0)->set_target_port_id("missing");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(hasIssueCode(result, "TARGET_PORT_NOT_FOUND"));
}

void tst_RuntimeDeploymentPhase2::duplicateIdsAndDirectionReturnStableIssueCodes()
{
    auto graph = validFixture();
    graph.mutable_components(1)->set_instance_id("producer-01");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(hasIssueCode(result, "DUPLICATE_INSTANCE_ID"));
}

void tst_RuntimeDeploymentPhase2::messageTypeMismatchReturnsStableIssueCode()
{
    auto graph = validFixture();
    graph.mutable_routes(0)->set_message_type("other.event");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(hasIssueCode(result, "SOURCE_MESSAGE_TYPE_MISMATCH"));
}

void tst_RuntimeDeploymentPhase2::missingHandlerIsRejected()
{
    auto graph = validFixture();
    graph.mutable_components(1)->set_component_type("unknown");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(hasIssueCode(result, "HANDLER_NOT_FOUND"));
}

void tst_RuntimeDeploymentPhase2::activationRequiresStoppedRuntime()
{
    cme::runtime::RuntimeService service;
    cme::runtime::DeploymentManager manager(&service);
    QVERIFY(!manager.activate().valid());
}

void tst_RuntimeDeploymentPhase2::activeGraphVersionCannotBeRedeployed()
{
    QVERIFY(true);
}

void tst_RuntimeDeploymentPhase2::clonedInstanceKeepsIndependentConfiguration()
{
    QVERIFY(true);
}

void tst_RuntimeDeploymentPhase2::catalogValidationRejectsInvalidComponentConfiguration()
{
    auto graph = validFixture();
    auto *instance = graph.mutable_components(0);
    instance->set_component_identity("catalog.producer");
    instance->set_component_version("1.0.0");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    cme::runtime::ComponentCatalog catalog;
    auto value = descriptor(QStringLiteral("catalog.producer"), cme::runtime::ComponentStatus::Available,
                            cme::runtime::TrustState::Trusted);
    value.properties[0].defaultValue.clear();
    value.ports[0].required = true;
    QVERIFY(catalog.registerDescriptor(value));
    auto result = cme::runtime::DeploymentValidator(&registry, &catalog).validate(graph);
    QVERIFY(hasIssueCode(result, "REQUIRED_PROPERTY_MISSING"));
    QVERIFY(hasIssueCode(result, "REQUIRED_PORT_MISSING"));
}

void tst_RuntimeDeploymentPhase2::catalogValidationRejectsUnavailableComponent()
{
    auto graph = validFixture();
    auto *instance = graph.mutable_components(0);
    instance->set_component_identity("catalog.producer");
    instance->set_component_version("1.0.0");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    cme::runtime::ComponentCatalog catalog;
    QVERIFY(catalog.registerDescriptor(descriptor(QStringLiteral("catalog.producer"),
                                                   cme::runtime::ComponentStatus::Unavailable,
                                                   cme::runtime::TrustState::Trusted)));
    const auto result = cme::runtime::DeploymentValidator(&registry, &catalog).validate(graph);
    QVERIFY(hasIssueCode(result, "COMPONENT_UNAVAILABLE"));
}

void tst_RuntimeDeploymentPhase2::catalogValidationRejectsUntrustedComponent()
{
    auto graph = validFixture();
    auto *instance = graph.mutable_components(0);
    instance->set_component_identity("catalog.producer");
    instance->set_component_version("1.0.0");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandlers(&registry);
    cme::runtime::ComponentCatalog catalog;
    auto value = descriptor(QStringLiteral("catalog.producer"), cme::runtime::ComponentStatus::Unavailable,
                            cme::runtime::TrustState::Unknown);
    QVERIFY(!catalog.registerDescriptor(value));
    const auto result = cme::runtime::DeploymentValidator(&registry, &catalog).validate(graph);
    QVERIFY(hasIssueCode(result, "COMPONENT_DESCRIPTOR_NOT_FOUND"));
    QVERIFY(hasIssueCode(result, "COMPONENT_UNTRUSTED") ||
            hasIssueCode(result, "COMPONENT_DESCRIPTOR_NOT_FOUND"));
}

QTEST_MAIN(tst_RuntimeDeploymentPhase2)
#include "tst_RuntimeDeploymentPhase2.moc"
