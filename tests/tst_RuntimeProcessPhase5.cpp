#include <QtTest>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QProcess>
#include <QTemporaryDir>
#include <QThread>

#include <chrono>

#include <control.grpc.pb.h>
#include <event.grpc.pb.h>
#include <query.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#ifndef COMPONENT_MAP_RUNTIME_PROCESS_EXECUTABLE
#error "The runtime process executable path must be configured by CMake."
#endif

namespace
{
void addPort(cme::runtime::v1::ComponentInstance *component,
             const char *id, cme::runtime::v1::PortDirection direction)
{
    auto *port = component->add_ports();
    port->set_port_id(id);
    port->set_direction(direction);
    port->set_message_type("test.event");
}

cme::runtime::v1::DeploymentGraph graph()
{
    cme::runtime::v1::DeploymentGraph graph;
    graph.set_graph_version("grpc-v1");

    auto *producer = graph.add_components();
    producer->set_instance_id("producer-01");
    producer->set_component_type("producer");
    producer->set_state_schema_version(1);
    addPort(producer, "out", cme::runtime::v1::PORT_DIRECTION_OUTPUT);

    auto *processor = graph.add_components();
    processor->set_instance_id("processor-01");
    processor->set_component_type("processor");
    processor->set_state_schema_version(1);
    addPort(processor, "in", cme::runtime::v1::PORT_DIRECTION_INPUT);
    addPort(processor, "out", cme::runtime::v1::PORT_DIRECTION_OUTPUT);

    auto *sink = graph.add_components();
    sink->set_instance_id("sink-01");
    sink->set_component_type("sink");
    sink->set_state_schema_version(1);
    addPort(sink, "in", cme::runtime::v1::PORT_DIRECTION_INPUT);

    auto addRoute = [&graph](const char *id, const char *source, const char *sourcePort,
                             const char *target, const char *targetPort) {
        auto *route = graph.add_routes();
        route->set_connection_id(id);
        route->set_source_instance_id(source);
        route->set_source_port_id(sourcePort);
        route->set_target_instance_id(target);
        route->set_target_port_id(targetPort);
        route->set_message_type("test.event");
        route->set_enabled(true);
    };
    addRoute("route-producer-processor", "producer-01", "out", "processor-01", "in");
    addRoute("route-processor-sink", "processor-01", "out", "sink-01", "in");
    return graph;
}

std::shared_ptr<grpc::Channel> waitForRuntime(const QString &address)
{
    auto channel = grpc::CreateChannel(address.toStdString(), grpc::InsecureChannelCredentials());
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000)
    {
        const auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(250);
        if (!channel->WaitForConnected(deadline))
        {
            QThread::msleep(25);
            continue;
        }
        auto control = cme::runtime::v1::RuntimeControl::NewStub(channel);
        cme::runtime::v1::RuntimeStatusRequest request;
        cme::runtime::v1::RuntimeStatusResponse response;
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(100));
        if (control->GetRuntimeStatus(&context, request, &response).ok())
            return channel;
        QThread::msleep(25);
    }
    return {};
}
}

class tst_RuntimeProcessPhase5 : public QObject
{
    Q_OBJECT

private slots:
    void grpcVerticalSliceThroughProcess();
};

void tst_RuntimeProcessPhase5::grpcVerticalSliceThroughProcess()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const int port = 51000 + (QCoreApplication::applicationPid() % 1000);
    const QString address = QStringLiteral("127.0.0.1:%1").arg(port);
    QProcess runtime;
    runtime.setProgram(QStringLiteral(COMPONENT_MAP_RUNTIME_PROCESS_EXECUTABLE));
    runtime.setArguments({QStringLiteral("--address"), address,
                          QStringLiteral("--database"), directory.filePath(QStringLiteral("runtime.sqlite")),
                          QStringLiteral("--runtime-id"), QStringLiteral("grpc-process-runtime")});
    runtime.start();
    QVERIFY2(runtime.waitForStarted(3000), qPrintable(runtime.errorString()));

    const auto channel = waitForRuntime(address);
    QVERIFY2(channel, qPrintable(runtime.readAllStandardError()));

    auto control = cme::runtime::v1::RuntimeControl::NewStub(channel);
    auto events = cme::runtime::v1::RuntimeEvents::NewStub(channel);
    auto query = cme::runtime::v1::RuntimeQuery::NewStub(channel);

    cme::runtime::v1::DeployGraphRequest deployRequest;
    *deployRequest.mutable_graph() = graph();
    cme::runtime::v1::ControlResponse deployResponse;
    grpc::ClientContext deployContext;
    QVERIFY(control->DeployGraph(&deployContext, deployRequest, &deployResponse).ok());
    QVERIFY2(deployResponse.success(), qPrintable(QString::fromStdString(deployResponse.error_message())));

    cme::runtime::v1::ActivateGraphRequest activateRequest;
    cme::runtime::v1::ControlResponse activateResponse;
    grpc::ClientContext activateContext;
    QVERIFY(control->ActivateGraph(&activateContext, activateRequest, &activateResponse).ok());
    QVERIFY2(activateResponse.success(), qPrintable(QString::fromStdString(activateResponse.error_message())));

    cme::runtime::v1::EmptyRequest startRequest;
    cme::runtime::v1::ControlResponse startResponse;
    grpc::ClientContext startContext;
    QVERIFY(control->StartRuntime(&startContext, startRequest, &startResponse).ok());
    QVERIFY2(startResponse.success(), qPrintable(QString::fromStdString(startResponse.error_message())));

    cme::runtime::v1::PublishEventRequest eventRequest;
    eventRequest.set_event_id("grpc-event-1");
    eventRequest.set_idempotency_key("grpc-key-1");
    eventRequest.set_message_type("test.event");
    eventRequest.set_target_component_id("producer-01");
    eventRequest.set_payload("payload");
    cme::runtime::v1::PublishEventResponse eventResponse;
    grpc::ClientContext eventContext;
    QVERIFY(events->PublishEvent(&eventContext, eventRequest, &eventResponse).ok());
    QVERIFY(eventResponse.accepted());

    cme::runtime::v1::ComponentStatusRequest statusRequest;
    statusRequest.set_component_id("sink-01");
    cme::runtime::v1::ComponentStatusResponse statusResponse;
    grpc::ClientContext statusContext;
    QVERIFY(query->GetComponentStatus(&statusContext, statusRequest, &statusResponse).ok());
    QVERIFY(statusResponse.found());
    QCOMPARE(statusResponse.component_id(), std::string("sink-01"));
    QCOMPARE(statusResponse.component_type(), std::string("sink"));
    QCOMPARE(statusResponse.state(), std::string("payload"));
    QCOMPARE(statusResponse.mailbox_depth(), 0);

    cme::runtime::v1::QueryRequest queryRequest;
    queryRequest.set_limit(10);
    cme::runtime::v1::EventHistoryResponse history;
    grpc::ClientContext historyContext;
    QVERIFY(query->ListEventHistory(&historyContext, queryRequest, &history).ok());
    QCOMPARE(history.items_size(), 3);
    for (const auto &item : history.items())
    {
        QCOMPARE(item.event_id(), std::string("grpc-event-1"));
        QCOMPARE(item.status(), std::string("completed"));
    }

    cme::runtime::v1::RuntimeLogsResponse logs;
    grpc::ClientContext logsContext;
    QVERIFY(query->ListRuntimeLogs(&logsContext, queryRequest, &logs).ok());
    QVERIFY(logs.items_size() >= 3);

    cme::runtime::v1::EmptyRequest stopRequest;
    cme::runtime::v1::ControlResponse stopResponse;
    grpc::ClientContext stopContext;
    QVERIFY(control->StopRuntime(&stopContext, stopRequest, &stopResponse).ok());
    QVERIFY(stopResponse.success());

    runtime.kill();
    QVERIFY(runtime.waitForFinished(3000));
    QCOMPARE(runtime.exitStatus(), QProcess::CrashExit);
}

QTEST_MAIN(tst_RuntimeProcessPhase5)
#include "tst_RuntimeProcessPhase5.moc"
