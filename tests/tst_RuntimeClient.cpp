#include <QtTest>

#include <control.grpc.pb.h>
#include <event.grpc.pb.h>
#include <query.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include "RuntimeClient.h"

namespace
{

class FakeControl final : public cme::runtime::v1::RuntimeControl::Service
{
public:
    grpc::Status DeployGraph(grpc::ServerContext *, const cme::runtime::v1::DeployGraphRequest *request,
                             cme::runtime::v1::ControlResponse *response) override
    {
        deployedVersion = QString::fromStdString(request->graph().graph_version());
        response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status ActivateGraph(grpc::ServerContext *, const cme::runtime::v1::ActivateGraphRequest *request,
                               cme::runtime::v1::ControlResponse *response) override
    {
        activatedVersion = QString::fromStdString(request->graph_version());
        response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status StartRuntime(grpc::ServerContext *, const cme::runtime::v1::EmptyRequest *,
                              cme::runtime::v1::ControlResponse *response) override
    {
        response->set_success(true);
        response->set_status(cme::runtime::v1::RUNTIME_STATUS_RUNNING);
        return grpc::Status::OK;
    }

    grpc::Status StopRuntime(grpc::ServerContext *, const cme::runtime::v1::EmptyRequest *,
                             cme::runtime::v1::ControlResponse *response) override
    {
        response->set_success(true);
        response->set_status(cme::runtime::v1::RUNTIME_STATUS_STOPPED);
        return grpc::Status::OK;
    }

    grpc::Status GetRuntimeStatus(grpc::ServerContext *, const cme::runtime::v1::RuntimeStatusRequest *,
                                  cme::runtime::v1::RuntimeStatusResponse *response) override
    {
        response->set_success(true);
        response->set_runtime_id("fake-runtime");
        response->set_graph_version("v1");
        response->set_status(cme::runtime::v1::RUNTIME_STATUS_RUNNING);
        return grpc::Status::OK;
    }

    QString deployedVersion;
    QString activatedVersion;
};

class FakeEvents final : public cme::runtime::v1::RuntimeEvents::Service
{
public:
    grpc::Status PublishEvent(grpc::ServerContext *, const cme::runtime::v1::PublishEventRequest *request,
                              cme::runtime::v1::PublishEventResponse *response) override
    {
        eventId = QString::fromStdString(request->event_id());
        response->set_accepted(true);
        return grpc::Status::OK;
    }

    QString eventId;
};

class FakeQuery final : public cme::runtime::v1::RuntimeQuery::Service
{
public:
    grpc::Status GetComponentStatus(grpc::ServerContext *, const cme::runtime::v1::ComponentStatusRequest *,
                                    cme::runtime::v1::ComponentStatusResponse *response) override
    {
        response->set_found(true);
        response->set_component_id("component-1");
        return grpc::Status::OK;
    }

    grpc::Status ListEventHistory(grpc::ServerContext *, const cme::runtime::v1::QueryRequest *,
                                  cme::runtime::v1::EventHistoryResponse *response) override
    {
        response->add_items()->set_event_id("event-1");
        return grpc::Status::OK;
    }

    grpc::Status ListRuntimeLogs(grpc::ServerContext *, const cme::runtime::v1::QueryRequest *,
                                 cme::runtime::v1::RuntimeLogsResponse *response) override
    {
        response->add_items()->set_code("RUNTIME_OK");
        return grpc::Status::OK;
    }

    grpc::Status ListRejections(grpc::ServerContext *, const cme::runtime::v1::QueryRequest *,
                                cme::runtime::v1::RejectionsResponse *response) override
    {
        response->add_items()->set_reason_code("NONE");
        return grpc::Status::OK;
    }
};

class FakeServer
{
public:
    FakeServer()
    {
        builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
        builder.RegisterService(&control);
        builder.RegisterService(&events);
        builder.RegisterService(&query);
        server = builder.BuildAndStart();
    }

    ~FakeServer()
    {
        if (server)
            server->Shutdown();
    }

    QString address() const { return QStringLiteral("127.0.0.1:%1").arg(port); }

    grpc::ServerBuilder builder;
    int port = 0;
    std::unique_ptr<grpc::Server> server;
    FakeControl control;
    FakeEvents events;
    FakeQuery query;
};

} // namespace

class RuntimeClientTests : public QObject
{
    Q_OBJECT

private slots:
    void callsControlEventAndQueryServices();
    void reportsInvalidConfiguration();
    void mapsDeploymentDiagnosticsWithStablePaths();
    void normalizesDiagnosticCodes();
};

void RuntimeClientTests::mapsDeploymentDiagnosticsWithStablePaths()
{
    cme::runtime::v1::DeploymentValidationResult result;
    auto *issue = result.add_issues();
    issue->set_code("required_property_missing");
    issue->set_severity(cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_ERROR);
    issue->set_field_path("components[2].properties.mode");
    issue->set_message("Mode is required.");
    issue->set_component_identity("runtime.transform");
    issue->set_component_version("2.1.0");

    const QVariantList mapped = cme::editor::RuntimeClient::deploymentValidationIssues(result);
    QCOMPARE(mapped.size(), 1);
    const QVariantMap issueMap = mapped.first().toMap();
    QCOMPARE(issueMap.value(QStringLiteral("code")).toString(),
             QStringLiteral("REQUIRED_PROPERTY_MISSING"));
    QCOMPARE(issueMap.value(QStringLiteral("fieldPath")).toString(),
             QStringLiteral("components[2].properties.mode"));
    QCOMPARE(issueMap.value(QStringLiteral("propertyPath")).toString(), QStringLiteral("mode"));
    QCOMPARE(issueMap.value(QStringLiteral("componentIdentity")).toString(),
             QStringLiteral("runtime.transform"));
    QCOMPARE(issueMap.value(QStringLiteral("componentVersion")).toString(),
             QStringLiteral("2.1.0"));
}

void RuntimeClientTests::normalizesDiagnosticCodes()
{
    QCOMPARE(cme::editor::RuntimeClient::stableDiagnosticCode(
                 QStringLiteral("component-port/type mismatch")),
             QStringLiteral("COMPONENT_PORT_TYPE_MISMATCH"));
    QCOMPARE(cme::editor::RuntimeClient::componentPathFromFieldPath(
                 QStringLiteral("components[4].ports.input")),
             QStringLiteral("components[4]"));
    QCOMPARE(cme::editor::RuntimeClient::portPathFromFieldPath(
                 QStringLiteral("components[4].ports.input")),
             QStringLiteral("input"));
}



void RuntimeClientTests::callsControlEventAndQueryServices()
{
    FakeServer server;
    QVERIFY(server.server != nullptr);

    cme::editor::RuntimeClient client(server.address());
    cme::runtime::v1::DeploymentGraph graph;
    graph.set_graph_version("v1");
    cme::runtime::v1::ControlResponse controlResponse;
    QString error;
    QVERIFY2(client.deployGraph(graph, &controlResponse, &error), qPrintable(error));
    QCOMPARE(server.control.deployedVersion, QStringLiteral("v1"));

    QVERIFY2(client.activateGraph(QStringLiteral("v1"), &controlResponse, &error), qPrintable(error));
    QCOMPARE(server.control.activatedVersion, QStringLiteral("v1"));
    QVERIFY2(client.startRuntime(&controlResponse, &error), qPrintable(error));
    QVERIFY2(client.stopRuntime(&controlResponse, &error), qPrintable(error));

    cme::runtime::v1::RuntimeStatusResponse status;
    QVERIFY2(client.runtimeStatus(&status, &error), qPrintable(error));
    QCOMPARE(QString::fromStdString(status.runtime_id()), QStringLiteral("fake-runtime"));

    cme::runtime::v1::PublishEventRequest event;
    event.set_event_id("event-1");
    cme::runtime::v1::PublishEventResponse eventResponse;
    QVERIFY2(client.publishEvent(event, &eventResponse, &error), qPrintable(error));
    QCOMPARE(server.events.eventId, QStringLiteral("event-1"));

    cme::runtime::v1::ComponentStatusResponse component;
    QVERIFY2(client.componentStatus(QStringLiteral("component-1"), &component, &error), qPrintable(error));
    QVERIFY(component.found());

    cme::runtime::v1::EventHistoryResponse history;
    QVERIFY2(client.eventHistory(10, &history, &error), qPrintable(error));
    QCOMPARE(history.items_size(), 1);

    cme::runtime::v1::RuntimeLogsResponse logs;
    QVERIFY2(client.runtimeLogs(10, &logs, &error), qPrintable(error));
    QCOMPARE(logs.items_size(), 1);

    cme::runtime::v1::RejectionsResponse rejections;
    QVERIFY2(client.rejections(10, &rejections, &error), qPrintable(error));
    QCOMPARE(rejections.items_size(), 1);
}

void RuntimeClientTests::reportsInvalidConfiguration()
{
    cme::editor::RuntimeClient client{QString()};
    QString error;
    QVERIFY(!client.runtimeStatus(nullptr, &error));
    QVERIFY(error.contains(QStringLiteral("response")));

    cme::runtime::v1::RuntimeStatusResponse response;
    QVERIFY(!client.runtimeStatus(&response, &error));
    QVERIFY(error.contains(QStringLiteral("address")));
}

QTEST_MAIN(RuntimeClientTests)
#include "tst_RuntimeClient.moc"
