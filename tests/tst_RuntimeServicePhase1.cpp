#include <QtTest>

#include "component_map_runtime/RuntimeService.h"

class tst_RuntimeServicePhase1 : public QObject
{
    Q_OBJECT

private slots:
    void invalidDescriptorIsRejected();
    void validDescriptorHasDeterministicLifecycle();
    void statusSnapshotRoundTripsDescriptor();
};

void tst_RuntimeServicePhase1::invalidDescriptorIsRejected()
{
    cme::runtime::RuntimeService service;
    QString error;

    QVERIFY(!service.validateConfiguration(&error));
    QCOMPARE(error, QStringLiteral("Runtime ID must not be empty."));
    QVERIFY(!service.start(&error));
    QCOMPARE(service.status(), cme::runtime::v1::RUNTIME_STATUS_FAILED);
    QCOMPARE(service.lastError(), error);
}

void tst_RuntimeServicePhase1::validDescriptorHasDeterministicLifecycle()
{
    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id("test-runtime");
    descriptor.set_protocol_version("1.0");
    descriptor.set_graph_version("graph-v1");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);

    cme::runtime::RuntimeService service(descriptor);
    QString error;
    QVERIFY(service.start(&error));
    QVERIFY(error.isEmpty());
    QCOMPARE(service.status(), cme::runtime::v1::RUNTIME_STATUS_RUNNING);
    QVERIFY(service.start(&error));

    QVERIFY(service.stop(&error));
    QVERIFY(error.isEmpty());
    QCOMPARE(service.status(), cme::runtime::v1::RUNTIME_STATUS_STOPPED);
    QVERIFY(service.stop(&error));
}

void tst_RuntimeServicePhase1::statusSnapshotRoundTripsDescriptor()
{
    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id("snapshot-runtime");
    descriptor.set_protocol_version("1.0");
    descriptor.set_graph_version("graph-v2");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_EMBEDDED);

    cme::runtime::RuntimeService service(descriptor);
    const cme::runtime::v1::RuntimeStatusSnapshot snapshot = service.statusSnapshot();

    QCOMPARE(snapshot.descriptor().runtime_id(), std::string("snapshot-runtime"));
    QCOMPARE(snapshot.descriptor().protocol_version(), std::string("1.0"));
    QCOMPARE(snapshot.descriptor().graph_version(), std::string("graph-v2"));
    QCOMPARE(snapshot.descriptor().mode(), cme::runtime::v1::RUNTIME_MODE_EMBEDDED);
    QCOMPARE(snapshot.status(), cme::runtime::v1::RUNTIME_STATUS_STOPPED);
}

QTEST_MAIN(tst_RuntimeServicePhase1)
#include "tst_RuntimeServicePhase1.moc"
