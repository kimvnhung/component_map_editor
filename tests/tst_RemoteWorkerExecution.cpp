#include <QtTest>

#include "component_map_runtime/RemoteWorkerExecutor.h"

using namespace cme::runtime;

namespace
{
ComponentDescriptor validDescriptor()
{
    ComponentDescriptor descriptor;
    descriptor.identity = QStringLiteral("network.worker");
    descriptor.source = ComponentSource::Network;
    descriptor.sourceId = QStringLiteral("trusted.source");
    descriptor.status = ComponentStatus::Available;
    descriptor.trustState = TrustState::Trusted;
    descriptor.execution.mode = ExecutionMode::RemoteWorker;
    descriptor.execution.workerCapabilityReference = QStringLiteral("worker.capability");
    descriptor.execution.protocol = QStringLiteral("grpc");
    descriptor.execution.errorBehavior = QStringLiteral("structured");
    descriptor.execution.inputRequirements = {QStringLiteral("input")};
    descriptor.execution.outputGuarantees = {QStringLiteral("output")};
    descriptor.execution.retryable = true;
    descriptor.execution.idempotent = true;
    descriptor.execution.timeoutMs = 1000;
    return descriptor;
}

class FakeTransport final : public RemoteWorkerTransport
{
public:
    bool execute(const cme::RemoteExecutionRequest &request,
                 cme::RemoteExecutionResult *result,
                 QString *error) override
    {
        Q_UNUSED(error);
        requests.append(request);
        if (failuresRemaining > 0)
        {
            --failuresRemaining;
            return false;
        }
        result->set_request_id(request.request_id());
        result->set_component_identity(request.component_identity());
        result->set_component_version(request.component_version());
        result->set_status(cme::REMOTE_EXECUTION_STATUS_SUCCEEDED);
        result->add_outputs()->set_name("output");
        return true;
    }

    bool cancel(const cme::RemoteExecutionCancelRequest &request,
                cme::RemoteExecutionCancelResponse *response,
                QString *error) override
    {
        Q_UNUSED(error);
        cancelRequest = request;
        response->set_accepted(true);
        return true;
    }

    QList<cme::RemoteExecutionRequest> requests;
    cme::RemoteExecutionCancelRequest cancelRequest;
    int failuresRemaining = 0;
};
}

class tst_RemoteWorkerExecution : public QObject
{
    Q_OBJECT

private slots:
    void buildsCorrelatedRequestAndRetries();
    void cancelsByRequestId();
    void rejectsUntrustedDescriptor();
};

void tst_RemoteWorkerExecution::buildsCorrelatedRequestAndRetries()
{
    FakeTransport transport;
    transport.failuresRemaining = 1;
    RemoteWorkerExecutor executor(&transport);

    QString error;
    cme::RemoteExecutionResult result;
    QVERIFY(executor.execute(validDescriptor(), QStringLiteral("request-1"),
                             QStringLiteral("instance-1"), QByteArrayLiteral("payload"),
                             QVariantMap{{QStringLiteral("mode"), QStringLiteral("safe")}},
                             &result, &error));
    QCOMPARE(transport.requests.size(), 2);
    QCOMPARE(QString::fromStdString(transport.requests.at(1).request_id()),
             QStringLiteral("request-1"));
    QCOMPARE(transport.requests.at(1).attempt(), 2);
    QCOMPARE(QString::fromStdString(transport.requests.at(1).component_identity()),
             QStringLiteral("network.worker"));
    QVERIFY(!transport.requests.at(1).configuration().contains("credential"));
    QVERIFY(error.isEmpty());
}

void tst_RemoteWorkerExecution::cancelsByRequestId()
{
    FakeTransport transport;
    RemoteWorkerExecutor executor(&transport);
    cme::RemoteExecutionCancelResponse response;
    QString error;
    QVERIFY(executor.cancel(QStringLiteral("request-2"), QStringLiteral("cancel-2"),
                            &response, &error));
    QCOMPARE(QString::fromStdString(transport.cancelRequest.request_id()),
             QStringLiteral("request-2"));
    QCOMPARE(QString::fromStdString(transport.cancelRequest.cancellation_token()),
             QStringLiteral("cancel-2"));
    QVERIFY(response.accepted());
}

void tst_RemoteWorkerExecution::rejectsUntrustedDescriptor()
{
    FakeTransport transport;
    RemoteWorkerExecutor executor(&transport);
    ComponentDescriptor descriptor = validDescriptor();
    descriptor.trustState = TrustState::Unknown;
    cme::RemoteExecutionResult result;
    QString error;
    QVERIFY(!executor.execute(descriptor, QStringLiteral("request-3"), {}, {}, {}, &result, &error));
    QVERIFY(error.contains(QStringLiteral("trusted")));
    QVERIFY(transport.requests.isEmpty());
}

QTEST_MAIN(tst_RemoteWorkerExecution)
#include "tst_RemoteWorkerExecution.moc"
