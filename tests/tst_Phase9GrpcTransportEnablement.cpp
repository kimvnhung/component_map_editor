#include <QTest>

#include <atomic>

#include "transport/GrpcCommandTransport.h"

namespace {

class FakeCoreExecutor : public cme::transport::ITypedCommandExecutor
{
public:
    bool nextSuccess = true;
    QString nextError;
    int callCount = 0;

    bool execute(const cme::GraphCommandRequest &,
                 cme::GraphCommandResult *outResult,
                 QString *error) override
    {
        ++callCount;
        if (outResult) {
            outResult->set_success(nextSuccess);
            outResult->set_error_code(nextSuccess ? cme::ERROR_CODE_UNSPECIFIED
                                                  : cme::ERROR_CODE_UNKNOWN_COMMAND);
            outResult->set_error_message(nextError.toStdString());
        }
        if (!nextSuccess && error)
            *error = nextError;
        return nextSuccess;
    }
};

class FakeGrpcClient : public cme::transport::IGrpcCommandClient
{
public:
    cme::transport::GrpcCallStatus nextStatus = cme::transport::GrpcCallStatus::Ok;
    cme::GraphCommandResult nextResult;
    QString nextError;
    int callCount = 0;

    cme::transport::GrpcCallStatus execute(const cme::GraphCommandRequest &,
                                           const cme::transport::CommandCallOptions &,
                                           cme::GraphCommandResult *outResult,
                                           QString *error) override
    {
        ++callCount;
        if (outResult)
            *outResult = nextResult;
        if (error)
            *error = nextError;
        return nextStatus;
    }
};

cme::GraphCommandRequest makeTypedAddComponentRequest()
{
    cme::GraphCommandRequest req;
    auto *add = req.mutable_add_component();
    add->set_component_id("c1");
    add->set_type_id("process");
    add->set_x(10.0);
    add->set_y(20.0);
    return req;
}

} // namespace

class tst_Phase9GrpcTransportEnablement : public QObject
{
    Q_OBJECT

private slots:
    void transportOff_usesInProcessExecutor();
    void grpcSuccess_returnsSuccessResult();
    void grpcFailure_returnsFailureWithoutFallback();
    void grpcCancel_returnsCancelledFailure();
    void grpcTimeout_returnsDeadlineFailure();
};

void tst_Phase9GrpcTransportEnablement::transportOff_usesInProcessExecutor()
{
    FakeCoreExecutor core;
    FakeGrpcClient grpc;

    cme::transport::GrpcCommandTransportFacade facade(&core, &grpc);
    cme::transport::CommandTransportFeatureFlags flags;
    flags.grpcTransportEnabled = false;
    flags.fallbackToInProcessOnGrpcFailure = true;
    facade.setFeatureFlags(flags);

    cme::GraphCommandResult result;
    QString error;
    const bool ok = facade.execute(makeTypedAddComponentRequest(), &result, &error);

    QVERIFY(ok);
    QCOMPARE(core.callCount, 1);
    QCOMPARE(grpc.callCount, 0);
    QVERIFY(result.success());
    QVERIFY(error.isEmpty());
}

void tst_Phase9GrpcTransportEnablement::grpcSuccess_returnsSuccessResult()
{
    FakeCoreExecutor core;
    FakeGrpcClient grpc;
    grpc.nextStatus = cme::transport::GrpcCallStatus::Ok;
    grpc.nextResult.set_success(true);
    grpc.nextResult.set_error_code(cme::ERROR_CODE_UNSPECIFIED);

    cme::transport::GrpcCommandTransportFacade facade(&core, &grpc);
    cme::transport::CommandTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToInProcessOnGrpcFailure = false;
    facade.setFeatureFlags(flags);

    cme::GraphCommandResult result;
    const bool ok = facade.execute(makeTypedAddComponentRequest(), &result);

    QVERIFY(ok);
    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(core.callCount, 0);
    QVERIFY(result.success());
}

void tst_Phase9GrpcTransportEnablement::grpcFailure_returnsFailureWithoutFallback()
{
    FakeCoreExecutor core;
    FakeGrpcClient grpc;
    grpc.nextStatus = cme::transport::GrpcCallStatus::Failed;
    grpc.nextError = QStringLiteral("upstream rpc failure");

    cme::transport::GrpcCommandTransportFacade facade(&core, &grpc);
    cme::transport::CommandTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToInProcessOnGrpcFailure = false;
    facade.setFeatureFlags(flags);

    cme::GraphCommandResult result;
    QString error;
    const bool ok = facade.execute(makeTypedAddComponentRequest(), &result, &error);

    QVERIFY(!ok);
    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(core.callCount, 0);
    QVERIFY(!result.success());
    QVERIFY(result.error_message().find("rpc failure") != std::string::npos);
    QVERIFY(error.contains(QStringLiteral("rpc failure")));
}

void tst_Phase9GrpcTransportEnablement::grpcCancel_returnsCancelledFailure()
{
    FakeCoreExecutor core;
    FakeGrpcClient grpc;

    cme::transport::GrpcCommandTransportFacade facade(&core, &grpc);
    cme::transport::CommandTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToInProcessOnGrpcFailure = false;
    facade.setFeatureFlags(flags);

    std::atomic_bool cancelled(true);
    cme::transport::CommandCallOptions options;
    options.cancelRequested = &cancelled;

    cme::GraphCommandResult result;
    QString error;
    const bool ok = facade.execute(makeTypedAddComponentRequest(), &result, &error, options);

    QVERIFY(!ok);
    QCOMPARE(grpc.callCount, 0);
    QCOMPARE(core.callCount, 0);
    QVERIFY(!result.success());
    QVERIFY(result.error_message().find("cancel") != std::string::npos);
    QVERIFY(error.contains(QStringLiteral("cancel"), Qt::CaseInsensitive));
}

void tst_Phase9GrpcTransportEnablement::grpcTimeout_returnsDeadlineFailure()
{
    FakeCoreExecutor core;
    FakeGrpcClient grpc;
    grpc.nextStatus = cme::transport::GrpcCallStatus::DeadlineExceeded;
    grpc.nextError = QStringLiteral("deadline exceeded");

    cme::transport::GrpcCommandTransportFacade facade(&core, &grpc);
    cme::transport::CommandTransportFeatureFlags flags;
    flags.grpcTransportEnabled = true;
    flags.fallbackToInProcessOnGrpcFailure = false;
    facade.setFeatureFlags(flags);

    cme::transport::CommandCallOptions options;
    options.timeoutMs = 25;

    cme::GraphCommandResult result;
    QString error;
    const bool ok = facade.execute(makeTypedAddComponentRequest(), &result, &error, options);

    QVERIFY(!ok);
    QCOMPARE(grpc.callCount, 1);
    QCOMPARE(core.callCount, 0);
    QVERIFY(!result.success());
    QVERIFY(result.error_message().find("deadline") != std::string::npos);
    QVERIFY(error.contains(QStringLiteral("deadline"), Qt::CaseInsensitive));
}

QTEST_MAIN(tst_Phase9GrpcTransportEnablement)
#include "tst_Phase9GrpcTransportEnablement.moc"
