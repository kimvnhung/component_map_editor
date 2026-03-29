#include <QTest>
#include <QThread>

#include <atomic>

#include "transport/GrpcTemplateDescriptorTransport.h"

namespace {

class FakeLocalTemplateSource : public cme::transport::ITemplateDescriptorLocalSource
{
public:
    bool nextSuccess = true;
    QString nextError;
    int callCount = 0;

    bool fetch(const cme::transport::TemplateDescriptorFetchRequest &request,
               cme::transport::TemplateDescriptorFetchResponse *out,
               QString *error) override
    {
        ++callCount;
        if (out) {
            if (request.kind == cme::transport::TemplateDescriptorKind::ComponentType)
                out->componentTypeBundle.set_provider_id("local." + request.providerId.toStdString());
            if (request.kind == cme::transport::TemplateDescriptorKind::ConnectionPolicy)
                out->connectionPolicyBundle.set_provider_id("local." + request.providerId.toStdString());
            if (request.kind == cme::transport::TemplateDescriptorKind::PropertySchema)
                out->propertySchemaBundle.set_provider_id("local." + request.providerId.toStdString());
        }
        if (!nextSuccess && error)
            *error = nextError;
        return nextSuccess;
    }
};

class FakeGrpcTemplateClient : public cme::transport::IGrpcTemplateDescriptorClient
{
public:
    cme::transport::DescriptorCallStatus nextStatus = cme::transport::DescriptorCallStatus::Ok;
    QString nextError;
    int simulatedLatencyMs = 0;
    int callCount = 0;

    cme::transport::DescriptorCallStatus fetch(
        const cme::transport::TemplateDescriptorFetchRequest &request,
        const cme::transport::DescriptorCallOptions &,
        cme::transport::TemplateDescriptorFetchResponse *out,
        QString *error) override
    {
        ++callCount;
        if (simulatedLatencyMs > 0)
            QThread::msleep(static_cast<unsigned long>(simulatedLatencyMs));

        if (out) {
            if (request.kind == cme::transport::TemplateDescriptorKind::ComponentType)
                out->componentTypeBundle.set_provider_id("grpc." + request.providerId.toStdString());
            if (request.kind == cme::transport::TemplateDescriptorKind::ConnectionPolicy)
                out->connectionPolicyBundle.set_provider_id("grpc." + request.providerId.toStdString());
            if (request.kind == cme::transport::TemplateDescriptorKind::PropertySchema)
                out->propertySchemaBundle.set_provider_id("grpc." + request.providerId.toStdString());
        }

        if (error)
            *error = nextError;
        return nextStatus;
    }
};

cme::transport::TemplateDescriptorFetchRequest makeRequest(cme::transport::TemplateDescriptorKind kind)
{
    cme::transport::TemplateDescriptorFetchRequest request;
    request.kind = kind;
    request.providerId = QStringLiteral("sample.workflow");
    return request;
}

} // namespace

class tst_Phase11TemplateDescriptorTransport : public QObject
{
    Q_OBJECT

private slots:
    void transportOff_usesLocalSource();
    void grpcSuccess_underLatencySlo_usesGrpcResponse();
    void grpcFailure_fallsBackToLocal_whenEnabled();
    void grpcFailure_returnsError_whenFallbackDisabled();
    void grpcLatencySloViolation_fallsBackToLocal();
    void grpcOutage_gracefulFallbackKeepsRequestSuccessful();
};

void tst_Phase11TemplateDescriptorTransport::transportOff_usesLocalSource()
{
    FakeLocalTemplateSource local;
    FakeGrpcTemplateClient grpc;

    cme::transport::GrpcTemplateDescriptorTransportFacade facade(&local, &grpc);
    cme::transport::DescriptorTransportFeatureFlags flags;
    flags.grpcTransportEnabled = false;
    facade.setFeatureFlags(flags);

    cme::transport::TemplateDescriptorFetchResponse response;
    QString error;
    const bool ok = facade.fetch(makeRequest(cme::transport::TemplateDescriptorKind::ComponentType),
                                 &response,
                                 &error);

    QVERIFY(ok);
    QCOMPARE(local.callCount, 1);
    QCOMPARE(grpc.callCount, 0);
    QCOMPARE(QString::fromStdString(response.componentTypeBundle.provider_id()),
             QStringLiteral("local.sample.workflow"));
    QVERIFY(error.isEmpty());
}

void tst_Phase11TemplateDescriptorTransport::grpcSuccess_underLatencySlo_usesGrpcResponse()
{
    FakeLocalTemplateSource local;
    FakeGrpcTemplateClient grpc;
    grpc.nextStatus = cme::transport::DescriptorCallStatus::Ok;
    grpc.simulatedLatencyMs = 2;

    cme::transport::GrpcTemplateDescriptorTransportFacade facade(&local, &grpc);
    cme::transport::DescriptorTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToLocalOnGrpcFailure = true;
    flags.enforceLatencySlo = true;
    flags.maxLatencyMs = 25;
    facade.setFeatureFlags(flags);

    cme::transport::TemplateDescriptorFetchResponse response;
    QVERIFY(facade.fetch(makeRequest(cme::transport::TemplateDescriptorKind::PropertySchema), &response));

    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(local.callCount, 0);
    QCOMPARE(QString::fromStdString(response.propertySchemaBundle.provider_id()),
             QStringLiteral("grpc.sample.workflow"));

    const cme::transport::DescriptorTransportStats stats = facade.stats();
    QCOMPARE(stats.latencySloViolations, 0);
    QVERIFY(stats.lastLatencyMs >= 0);
}

void tst_Phase11TemplateDescriptorTransport::grpcFailure_fallsBackToLocal_whenEnabled()
{
    FakeLocalTemplateSource local;
    FakeGrpcTemplateClient grpc;
    grpc.nextStatus = cme::transport::DescriptorCallStatus::Failed;
    grpc.nextError = QStringLiteral("rpc failure");

    cme::transport::GrpcTemplateDescriptorTransportFacade facade(&local, &grpc);
    cme::transport::DescriptorTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToLocalOnGrpcFailure = true;
    facade.setFeatureFlags(flags);

    cme::transport::TemplateDescriptorFetchResponse response;
    QString error;
    const bool ok = facade.fetch(makeRequest(cme::transport::TemplateDescriptorKind::ConnectionPolicy),
                                 &response,
                                 &error);

    QVERIFY(ok);
    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(local.callCount, 1);
    QCOMPARE(QString::fromStdString(response.connectionPolicyBundle.provider_id()),
             QStringLiteral("local.sample.workflow"));

    const cme::transport::DescriptorTransportStats stats = facade.stats();
    QCOMPARE(stats.grpcFailures, 1);
    QCOMPARE(stats.fallbacks, 1);
}

void tst_Phase11TemplateDescriptorTransport::grpcFailure_returnsError_whenFallbackDisabled()
{
    FakeLocalTemplateSource local;
    FakeGrpcTemplateClient grpc;
    grpc.nextStatus = cme::transport::DescriptorCallStatus::Unavailable;
    grpc.nextError = QStringLiteral("transport unavailable");

    cme::transport::GrpcTemplateDescriptorTransportFacade facade(&local, &grpc);
    cme::transport::DescriptorTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToLocalOnGrpcFailure = false;
    facade.setFeatureFlags(flags);

    cme::transport::TemplateDescriptorFetchResponse response;
    QString error;
    const bool ok = facade.fetch(makeRequest(cme::transport::TemplateDescriptorKind::ComponentType),
                                 &response,
                                 &error);

    QVERIFY(!ok);
    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(local.callCount, 0);
    QVERIFY(error.contains(QStringLiteral("unavailable"), Qt::CaseInsensitive));
}

void tst_Phase11TemplateDescriptorTransport::grpcLatencySloViolation_fallsBackToLocal()
{
    FakeLocalTemplateSource local;
    FakeGrpcTemplateClient grpc;
    grpc.nextStatus = cme::transport::DescriptorCallStatus::Ok;
    grpc.simulatedLatencyMs = 20;

    cme::transport::GrpcTemplateDescriptorTransportFacade facade(&local, &grpc);
    cme::transport::DescriptorTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToLocalOnGrpcFailure = true;
    flags.enforceLatencySlo = true;
    flags.maxLatencyMs = 5;
    facade.setFeatureFlags(flags);

    cme::transport::TemplateDescriptorFetchResponse response;
    QString error;
    const bool ok = facade.fetch(makeRequest(cme::transport::TemplateDescriptorKind::ComponentType),
                                 &response,
                                 &error);

    QVERIFY(ok);
    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(local.callCount, 1);
    QCOMPARE(QString::fromStdString(response.componentTypeBundle.provider_id()),
             QStringLiteral("local.sample.workflow"));

    const cme::transport::DescriptorTransportStats stats = facade.stats();
    QCOMPARE(stats.latencySloViolations, 1);
    QCOMPARE(stats.fallbacks, 1);
    QVERIFY(stats.lastLatencyMs >= flags.maxLatencyMs);
}

void tst_Phase11TemplateDescriptorTransport::grpcOutage_gracefulFallbackKeepsRequestSuccessful()
{
    FakeLocalTemplateSource local;
    FakeGrpcTemplateClient grpc;
    grpc.nextStatus = cme::transport::DescriptorCallStatus::Unavailable;
    grpc.nextError = QStringLiteral("service down");

    cme::transport::GrpcTemplateDescriptorTransportFacade facade(&local, &grpc);
    cme::transport::DescriptorTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToLocalOnGrpcFailure = true;
    facade.setFeatureFlags(flags);

    cme::transport::TemplateDescriptorFetchResponse response;
    QString error;
    const bool ok = facade.fetch(makeRequest(cme::transport::TemplateDescriptorKind::PropertySchema),
                                 &response,
                                 &error);

    QVERIFY(ok);
    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(local.callCount, 1);
    QCOMPARE(QString::fromStdString(response.propertySchemaBundle.provider_id()),
             QStringLiteral("local.sample.workflow"));
    QVERIFY(error.isEmpty());
}

QTEST_MAIN(tst_Phase11TemplateDescriptorTransport)
#include "tst_Phase11TemplateDescriptorTransport.moc"
