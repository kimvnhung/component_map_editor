#include <QtTest>
#include <QTemporaryDir>

#include <memory>

#include <control.grpc.pb.h>
#include <event.grpc.pb.h>
#include <query.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include "component_map_runtime/RuntimeGrpcServer.h"

namespace
{
class Handler final : public cme::runtime::RuntimeHandler
{
public:
    explicit Handler(QString type) : m_type(std::move(type)) {}
    QString componentType() const override { return m_type; }
    bool process(const cme::runtime::RuntimeEvent &event, QByteArray &state,
                 cme::runtime::RuntimeEvent *output, cme::runtime::RuntimeFailure *) override
    {
        state.append(event.payload);
        *output = event;
        output->sourceComponentId = event.targetComponentId;
        return true;
    }
private:
    QString m_type;
};

cme::runtime::v1::DeploymentGraph graph()
{
    cme::runtime::v1::DeploymentGraph graph;
    graph.set_graph_version("grpc-v1");
    auto *component = graph.add_components();
    component->set_instance_id("processor-01");
    component->set_component_type("processor");
    component->set_state_schema_version(1);
    auto *port = component->add_ports();
    port->set_port_id("in");
    port->set_direction(cme::runtime::v1::PORT_DIRECTION_INPUT);
    port->set_message_type("test.event");
    return graph;
}
}

class tst_RuntimeProcessPhase5 : public QObject
{
    Q_OBJECT
private slots:
    void grpcVerticalSlice();
};

void tst_RuntimeProcessPhase5::grpcVerticalSlice()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id("grpc-runtime");
    descriptor.set_protocol_version("1.0");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);
    cme::runtime::RuntimeService service(descriptor);
    cme::runtime::RuntimeHandlerRegistry registry;
    QVERIFY(registry.registerFactory(QStringLiteral("processor"),
                                     [] { return std::make_unique<Handler>(QStringLiteral("processor")); }));
    cme::runtime::DeploymentManager deployment(&service, &registry);
    cme::runtime::RuntimePersistence persistence;
    QString error;
    QVERIFY(persistence.open(directory.filePath(QStringLiteral("runtime.sqlite")),
                             QStringLiteral("grpc-runtime"), &error));
    cme::runtime::RuntimeEngine engine(&service, &deployment, &registry, 4, &persistence);
    cme::runtime::RuntimeGrpcServer server(&engine, &deployment, &service, &persistence);
    QVERIFY(server.start(QStringLiteral("127.0.0.1:50091"), &error));

    auto channel = grpc::CreateChannel("127.0.0.1:50091", grpc::InsecureChannelCredentials());
    auto control = cme::runtime::v1::RuntimeControl::NewStub(channel);
    auto events = cme::runtime::v1::RuntimeEvents::NewStub(channel);
    auto query = cme::runtime::v1::RuntimeQuery::NewStub(channel);

    cme::runtime::v1::DeployGraphRequest deployRequest;
    *deployRequest.mutable_graph() = graph();
    cme::runtime::v1::ControlResponse controlResponse;
    grpc::ClientContext context;
    QVERIFY(control->DeployGraph(&context, deployRequest, &controlResponse).ok());
    QVERIFY(controlResponse.success());

    cme::runtime::v1::ActivateGraphRequest activateRequest;
    cme::runtime::v1::ControlResponse activateResponse;
    grpc::ClientContext activateContext;
    QVERIFY(control->ActivateGraph(&activateContext, activateRequest, &activateResponse).ok());
    QVERIFY(activateResponse.success());

    cme::runtime::v1::EmptyRequest startRequest;
    cme::runtime::v1::ControlResponse startResponse;
    grpc::ClientContext startContext;
    QVERIFY(control->StartRuntime(&startContext, startRequest, &startResponse).ok());
    QVERIFY(startResponse.success());

    cme::runtime::v1::PublishEventRequest eventRequest;
    eventRequest.set_event_id("grpc-event-1");
    eventRequest.set_idempotency_key("grpc-key-1");
    eventRequest.set_message_type("test.event");
    eventRequest.set_target_component_id("processor-01");
    eventRequest.set_payload("payload");
    cme::runtime::v1::PublishEventResponse eventResponse;
    grpc::ClientContext eventContext;
    QVERIFY(events->PublishEvent(&eventContext, eventRequest, &eventResponse).ok());
    QVERIFY(eventResponse.accepted());

    cme::runtime::v1::QueryRequest queryRequest;
    queryRequest.set_limit(10);
    cme::runtime::v1::EventHistoryResponse history;
    grpc::ClientContext historyContext;
    QVERIFY(query->ListEventHistory(&historyContext, queryRequest, &history).ok());
    QCOMPARE(history.items_size(), 1);
    QCOMPARE(history.items(0).event_id(), std::string("grpc-event-1"));

    cme::runtime::v1::RuntimeLogsResponse logs;
    grpc::ClientContext logsContext;
    QVERIFY(query->ListRuntimeLogs(&logsContext, queryRequest, &logs).ok());

    cme::runtime::v1::EmptyRequest stopRequest;
    cme::runtime::v1::ControlResponse stopResponse;
    grpc::ClientContext stopContext;
    QVERIFY(control->StopRuntime(&stopContext, stopRequest, &stopResponse).ok());
    QVERIFY(stopResponse.success());
    server.shutdown();
}

QTEST_MAIN(tst_RuntimeProcessPhase5)
#include "tst_RuntimeProcessPhase5.moc"
