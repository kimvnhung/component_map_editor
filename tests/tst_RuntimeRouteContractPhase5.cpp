#include <QtTest>

#include "component_map_runtime/DeploymentValidator.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"

namespace
{
cme::runtime::v1::DeploymentGraph validGraph()
{
    cme::runtime::v1::DeploymentGraph graph;
    graph.set_graph_id("factory");
    graph.set_graph_version("1.0");
    auto *source = graph.add_components();
    source->set_instance_id("source");
    source->set_component_type("source");
    source->set_state_schema_version(1);
    auto *sourcePort = source->add_ports();
    sourcePort->set_port_id("out");
    sourcePort->set_direction(cme::runtime::v1::PORT_DIRECTION_OUTPUT);
    sourcePort->set_message_type("test.event");
    sourcePort->set_cardinality(cme::runtime::v1::PORT_CARDINALITY_SINGLE);

    auto *target = graph.add_components();
    target->set_instance_id("target");
    target->set_component_type("target");
    target->set_state_schema_version(1);
    auto *targetPort = target->add_ports();
    targetPort->set_port_id("in");
    targetPort->set_direction(cme::runtime::v1::PORT_DIRECTION_INPUT);
    targetPort->set_message_type("test.event");
    targetPort->set_required(true);
    targetPort->set_cardinality(cme::runtime::v1::PORT_CARDINALITY_SINGLE);

    auto *route = graph.add_routes();
    route->set_connection_id("route-1");
    route->set_source_instance_id("source");
    route->set_source_port_id("out");
    route->set_target_instance_id("target");
    route->set_target_port_id("in");
    route->set_message_type("test.event");
    route->set_enabled(true);
    route->set_delivery_mode(cme::runtime::v1::DELIVERY_MODE_AT_MOST_ONCE);
    route->set_ordering_key("event");
    return graph;
}

void registerTypes(cme::runtime::RuntimeHandlerRegistry *registry)
{
    QVERIFY(registry->registerFactory(QStringLiteral("source"), [] {
        class Handler final : public cme::runtime::RuntimeHandler {
        public:
            QString componentType() const override { return QStringLiteral("source"); }
            bool process(const cme::runtime::RuntimeEvent &, QByteArray &, cme::runtime::RuntimeEvent *,
                         cme::runtime::RuntimeFailure *) override { return true; }
        };
        return std::make_unique<Handler>();
    }));
    QVERIFY(registry->registerFactory(QStringLiteral("target"), [] {
        class Handler final : public cme::runtime::RuntimeHandler {
        public:
            QString componentType() const override { return QStringLiteral("target"); }
            bool process(const cme::runtime::RuntimeEvent &, QByteArray &, cme::runtime::RuntimeEvent *,
                         cme::runtime::RuntimeFailure *) override { return true; }
        };
        return std::make_unique<Handler>();
    }));
}
}

class tst_RuntimeRouteContractPhase5 : public QObject
{
    Q_OBJECT

private slots:
    void validTypedRoutePasses();
    void duplicateAndDirectionErrorsAreReported();
    void incompatibleRouteAndPolicyErrorsAreReported();
    void tokenKeyMigrationProducesWarning();
};

void tst_RuntimeRouteContractPhase5::validTypedRoutePasses()
{
    cme::runtime::RuntimeHandlerRegistry registry;
    registerTypes(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(validGraph());
    QVERIFY2(result.valid(), qPrintable(QString::fromStdString(result.SerializeAsString())));
}

void tst_RuntimeRouteContractPhase5::duplicateAndDirectionErrorsAreReported()
{
    auto graph = validGraph();
    *graph.add_components() = graph.components(0);
    graph.mutable_components(2)->set_instance_id("source");
    graph.mutable_components(2)->mutable_ports(0)->set_port_id("out-2");
    graph.mutable_components(2)->mutable_ports(0)->set_cardinality(cme::runtime::v1::PORT_CARDINALITY_SINGLE);
    graph.mutable_routes(0)->set_source_port_id("missing");
    cme::runtime::RuntimeHandlerRegistry registry;
    registerTypes(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QString codes;
    for (const auto &issue : result.issues()) codes += QString::fromStdString(issue.code()) + ',';
    QVERIFY(codes.contains(QStringLiteral("DUPLICATE_PORT_ID")) || codes.contains(QStringLiteral("DUPLICATE_INSTANCE_ID")));
    QVERIFY(codes.contains(QStringLiteral("SOURCE_PORT_NOT_FOUND")));
}

void tst_RuntimeRouteContractPhase5::incompatibleRouteAndPolicyErrorsAreReported()
{
    auto graph = validGraph();
    graph.mutable_routes(0)->set_message_type("other.event");
    graph.mutable_routes(0)->set_delivery_mode(cme::runtime::v1::DELIVERY_MODE_UNSPECIFIED);
    cme::runtime::RuntimeHandlerRegistry registry;
    registerTypes(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(!result.valid());
    QString codes;
    for (const auto &issue : result.issues()) codes += QString::fromStdString(issue.code()) + ',';
    QVERIFY(codes.contains(QStringLiteral("SOURCE_MESSAGE_TYPE_MISMATCH")));
    QVERIFY(codes.contains(QStringLiteral("DELIVERY_MODE_DEFAULTED")));
}

void tst_RuntimeRouteContractPhase5::tokenKeyMigrationProducesWarning()
{
    auto graph = validGraph();
    graph.mutable_components(0)->mutable_ports(0)->set_token_key("legacy.token");
    graph.mutable_components(0)->mutable_ports(0)->clear_message_type();
    graph.mutable_components(1)->mutable_ports(0)->set_token_key("legacy.token");
    graph.mutable_components(1)->mutable_ports(0)->clear_message_type();
    graph.mutable_routes(0)->set_token_key("legacy.token");
    graph.mutable_routes(0)->clear_message_type();
    cme::runtime::RuntimeHandlerRegistry registry;
    registerTypes(&registry);
    const auto result = cme::runtime::DeploymentValidator(&registry).validate(graph);
    QVERIFY(result.valid());
    QVERIFY(result.issues_size() >= 1);
    bool foundTokenWarning = false;
    for (const auto &issue : result.issues())
    {
        if (QString::fromStdString(issue.code()) == QStringLiteral("TOKEN_KEY_DEPRECATED"))
            foundTokenWarning = issue.severity() == cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_WARNING;
    }
    QVERIFY(foundTokenWarning);
}

QTEST_MAIN(tst_RuntimeRouteContractPhase5)
#include "tst_RuntimeRouteContractPhase5.moc"
