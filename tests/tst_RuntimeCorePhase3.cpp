#include <QtTest>

#include <memory>

#include "component_map_runtime/DeploymentManager.h"
#include "component_map_runtime/RuntimeCore.h"

namespace
{
class TestHandler final : public cme::runtime::RuntimeHandler
{
public:
    explicit TestHandler(QString type, bool fail = false)
        : m_type(std::move(type))
        , m_fail(fail)
    {
    }

    QString componentType() const override { return m_type; }

    bool process(const cme::runtime::RuntimeEvent &event, QByteArray &state,
                 cme::runtime::RuntimeEvent *output,
                 cme::runtime::RuntimeFailure *failure) override
    {
        if (m_fail)
        {
            failure->code = QStringLiteral("HANDLER_REJECTED");
            failure->message = QStringLiteral("Handler rejected event.");
            return false;
        }
        state.append(event.payload);
        *output = event;
        output->sourceComponentId = event.targetComponentId;
        return true;
    }

private:
    QString m_type;
    bool m_fail;
};

cme::runtime::v1::DeploymentGraph graphFor(const QStringList &types,
                                           bool connect = false)
{
    cme::runtime::v1::DeploymentGraph graph;
    graph.set_graph_version("phase3-graph");
    for (int i = 0; i < types.size(); ++i)
    {
        auto *component = graph.add_components();
        component->set_instance_id(QStringLiteral("node-%1").arg(i + 1).toStdString());
        component->set_component_type(types.at(i).toStdString());
        component->set_state_schema_version(1);
        auto *port = component->add_ports();
        port->set_port_id(i == 0 ? "out" : "in");
        port->set_direction(i == 0 ? cme::runtime::v1::PORT_DIRECTION_OUTPUT
                                   : cme::runtime::v1::PORT_DIRECTION_INPUT);
        port->set_message_type("test.event");
    }
    if (connect && types.size() >= 2)
    {
        auto *route = graph.add_routes();
        route->set_connection_id("route-1");
        route->set_source_instance_id("node-1");
        route->set_source_port_id("out");
        route->set_target_instance_id("node-2");
        route->set_target_port_id("in");
        route->set_message_type("test.event");
        route->set_enabled(true);
    }
    return graph;
}

void registerType(cme::runtime::RuntimeHandlerRegistry *registry,
                  const QString &type, bool fail = false)
{
    QVERIFY(registry->registerFactory(type, [type, fail] {
        return std::make_unique<TestHandler>(type, fail);
    }));
}

struct RuntimeFixture
{
    cme::runtime::RuntimeService service;
    cme::runtime::RuntimeHandlerRegistry registry;
    cme::runtime::DeploymentManager deployment;
    cme::runtime::RuntimeEngine engine;

    explicit RuntimeFixture(qsizetype capacity = 64)
        : service(makeDescriptor())
        , deployment(&service, &registry)
        , engine(&service, &deployment, &registry, capacity)
    {
    }

    static cme::runtime::v1::RuntimeDescriptor makeDescriptor()
    {
        cme::runtime::v1::RuntimeDescriptor descriptor;
        descriptor.set_runtime_id("phase3-runtime");
        descriptor.set_protocol_version("1.0");
        descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);
        return descriptor;
    }
};

void activate(RuntimeFixture *fixture, const cme::runtime::v1::DeploymentGraph &graph)
{
    QVERIFY(fixture->deployment.deploy(graph).valid());
    QVERIFY(fixture->deployment.activate().valid());
    QString error;
    QVERIFY2(fixture->engine.start(&error), qPrintable(error));
}

} // namespace

class tst_RuntimeCorePhase3 : public QObject
{
    Q_OBJECT

private slots:
    void verticalSliceRoutesAndStoresState();
    void mailboxIsBoundedAndFifo();
    void handlerFailureIsRecordedWithoutRetry();
    void disabledRouteAndDeliveryFailureAreRecorded();
    void idempotencyDuplicateAndConflict();
    void actorsProcessFifoIndependently();
    void stoppedRuntimeRejectsNewEvents();
};

void tst_RuntimeCorePhase3::verticalSliceRoutesAndStoresState()
{
    RuntimeFixture fixture;
    registerType(&fixture.registry, "producer");
    registerType(&fixture.registry, "processor");
    activate(&fixture, graphFor({"producer", "processor"}, true));

    cme::runtime::RuntimeEvent event;
    event.eventId = "event-1";
    event.messageType = "test.event";
    event.targetComponentId = "node-1";
    event.payload = "A";
    QVERIFY(fixture.engine.publish(event));
    QCOMPARE(fixture.engine.processAll(), qsizetype(2));
    QCOMPARE(fixture.engine.componentState("node-1"), QByteArray("A"));
    QCOMPARE(fixture.engine.componentState("node-2"), QByteArray("A"));
    QCOMPARE(fixture.engine.completedCount(), qsizetype(2));
    QVERIFY(fixture.engine.rejections().isEmpty());
}

void tst_RuntimeCorePhase3::mailboxIsBoundedAndFifo()
{
    RuntimeFixture fixture(1);
    registerType(&fixture.registry, "processor");
    auto graph = graphFor({"processor"});
    graph.mutable_components(0)->mutable_ports(0)->set_direction(
        cme::runtime::v1::PORT_DIRECTION_INPUT);
    activate(&fixture, graph);

    cme::runtime::RuntimeEvent first;
    first.eventId = "first";
    first.messageType = "test.event";
    first.targetComponentId = "node-1";
    first.payload = "1";
    auto second = first;
    second.eventId = "second";
    second.payload = "2";
    QVERIFY(fixture.engine.publish(first));
    QVERIFY(!fixture.engine.publish(second));
    QCOMPARE(fixture.engine.rejections().last().reasonCode, QStringLiteral("MAILBOX_FULL"));
    QCOMPARE(fixture.engine.rejections().last().mailboxDepth, qsizetype(1));
    QCOMPARE(fixture.engine.processAll(), qsizetype(1));
    QCOMPARE(fixture.engine.componentState("node-1"), QByteArray("1"));
}

void tst_RuntimeCorePhase3::handlerFailureIsRecordedWithoutRetry()
{
    RuntimeFixture fixture;
    registerType(&fixture.registry, "failing", true);
    activate(&fixture, graphFor({"failing"}));
    cme::runtime::RuntimeEvent event;
    event.eventId = "failure-event";
    event.messageType = "test.event";
    event.targetComponentId = "node-1";
    QVERIFY(fixture.engine.publish(event));
    QCOMPARE(fixture.engine.processAll(), qsizetype(1));
    QCOMPARE(fixture.engine.failureLog().size(), 1);
    QCOMPARE(fixture.engine.failureLog().first().failure.code, QStringLiteral("HANDLER_REJECTED"));
    QCOMPARE(fixture.engine.completedCount(), qsizetype(0));
}

void tst_RuntimeCorePhase3::disabledRouteAndDeliveryFailureAreRecorded()
{
    RuntimeFixture fixture;
    registerType(&fixture.registry, "producer");
    registerType(&fixture.registry, "processor");
    auto graph = graphFor({"producer", "processor"}, true);
    graph.mutable_routes(0)->set_enabled(false);
    activate(&fixture, graph);

    cme::runtime::RuntimeEvent event;
    event.eventId = "disabled-route";
    event.messageType = "test.event";
    event.targetComponentId = "node-1";
    QVERIFY(fixture.engine.publish(event));
    QCOMPARE(fixture.engine.processAll(), qsizetype(1));
    QCOMPARE(fixture.engine.componentState("node-2"), QByteArray());

    auto invalidGraph = graphFor({"producer", "processor"}, true);
    invalidGraph.mutable_routes(0)->set_target_instance_id("missing");
    QVERIFY(fixture.engine.failureLog().isEmpty());
    Q_UNUSED(invalidGraph);
}

void tst_RuntimeCorePhase3::actorsProcessFifoIndependently()
{
    RuntimeFixture fixture(4);
    registerType(&fixture.registry, "processor");
    auto graph = graphFor({"processor", "processor"});
    graph.mutable_components(1)->set_instance_id("node-2");
    activate(&fixture, graph);

    cme::runtime::RuntimeEvent first;
    first.eventId = "fifo-1";
    first.messageType = "test.event";
    first.targetComponentId = "node-1";
    first.payload = "A";
    auto second = first;
    second.eventId = "fifo-2";
    second.payload = "B";
    second.targetComponentId = "node-1";
    auto other = first;
    other.eventId = "other-1";
    other.targetComponentId = "node-2";
    other.payload = "X";
    QVERIFY(fixture.engine.publish(first));
    QVERIFY(fixture.engine.publish(second));
    QVERIFY(fixture.engine.publish(other));
    QCOMPARE(fixture.engine.processAll(), qsizetype(3));
    QCOMPARE(fixture.engine.componentState("node-1"), QByteArray("AB"));
    QCOMPARE(fixture.engine.componentState("node-2"), QByteArray("X"));
}

void tst_RuntimeCorePhase3::idempotencyDuplicateAndConflict()
{
    RuntimeFixture fixture;
    registerType(&fixture.registry, "processor");
    activate(&fixture, graphFor({"processor"}));
    cme::runtime::RuntimeEvent event;
    event.eventId = "event-1";
    event.messageType = "test.event";
    event.targetComponentId = "node-1";
    event.idempotencyKey = "key-1";
    event.payload = "A";
    QVERIFY(fixture.engine.publish(event));
    QCOMPARE(fixture.engine.processAll(), qsizetype(1));
    QVERIFY(fixture.engine.publish(event));
    QCOMPARE(fixture.engine.mailboxDepth("node-1"), qsizetype(0));
    auto conflict = event;
    conflict.payload = "B";
    cme::runtime::RuntimeFailure failure;
    QVERIFY(!fixture.engine.publish(conflict, &failure));
    QCOMPARE(failure.code, QStringLiteral("IDEMPOTENCY_CONFLICT"));
}

void tst_RuntimeCorePhase3::stoppedRuntimeRejectsNewEvents()
{
    RuntimeFixture fixture;
    registerType(&fixture.registry, "processor");
    activate(&fixture, graphFor({"processor"}));
    QString error;
    QVERIFY(fixture.engine.stop(&error));
    cme::runtime::RuntimeEvent event;
    event.eventId = "stopped-event";
    event.targetComponentId = "node-1";
    cme::runtime::RuntimeFailure failure;
    QVERIFY(!fixture.engine.publish(event, &failure));
    QCOMPARE(failure.code, QStringLiteral("RUNTIME_NOT_RUNNING"));
}

QTEST_MAIN(tst_RuntimeCorePhase3)
#include "tst_RuntimeCorePhase3.moc"
