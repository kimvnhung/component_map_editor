#include <QtTest>

#include <algorithm>

#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/ComponentDiscovery.h"
#include "component_map_runtime/NetworkComponentSource.h"

#include <utility>

using namespace cme::runtime;

namespace
{
class ControlledNetworkSource final : public NetworkComponentSource
{
public:
    ControlledNetworkSource(QString id, bool trusted, int failuresBeforeSuccess,
                            QString endpoint = QStringLiteral("in-memory://catalog"))
        : m_id(std::move(id)), m_trusted(trusted), m_failuresBeforeSuccess(failuresBeforeSuccess),
          m_endpoint(std::move(endpoint))
    {
    }

    QString sourceId() const override { return m_id; }

    DiscoverySourceConfig configuration() const override
    {
        DiscoverySourceConfig config;
        config.sourceId = m_id;
        config.kind = DiscoverySourceKind::Network;
        config.endpoint = m_endpoint;
        config.trusted = m_trusted;
        config.retryPolicy.maxAttempts = 3;
        return config;
    }

    NetworkDiscoveryResult discover() override
    {
        ++calls;
        NetworkDiscoveryResult result;
        if (calls <= m_failuresBeforeSuccess)
        {
            result.diagnostics.append(ComponentDiagnostic{
                QStringLiteral("temporary_failure"), QStringLiteral("temporary"), {}, true});
            return result;
        }
        result.success = true;
        ComponentDescriptor descriptor;
        descriptor.identity = m_id + QStringLiteral(".component");
        descriptor.source = ComponentSource::Network;
        descriptor.sourceId = m_id;
        descriptor.status = ComponentStatus::Available;
        descriptor.trustState = TrustState::Trusted;
        descriptor.execution.mode = ExecutionMode::RemoteWorker;
        descriptor.execution.workerCapabilityReference = QStringLiteral("worker.capability");
        descriptor.execution.protocol = QStringLiteral("grpc");
        descriptor.execution.errorBehavior = QStringLiteral("structured");
        descriptor.execution.inputRequirements = {QStringLiteral("input")};
        descriptor.execution.outputGuarantees = {QStringLiteral("output")};
        result.descriptors.append(descriptor);
        return result;
    }

    int calls = 0;

private:
    QString m_id;
    bool m_trusted;
    int m_failuresBeforeSuccess;
    QString m_endpoint;
};

class FakeNetworkSource final : public NetworkComponentSource
{
public:
    QString sourceId() const override { return QStringLiteral("fake.source"); }

    DiscoverySourceConfig configuration() const override
    {
        DiscoverySourceConfig config;
        config.sourceId = sourceId();
        config.kind = DiscoverySourceKind::Network;
        config.endpoint = QStringLiteral("in-memory://catalog");
        return config;
    }

    NetworkDiscoveryResult discover() override
    {
        NetworkDiscoveryResult result;
        result.success = true;
        return result;
    }
};
}

class tst_ComponentDiscovery : public QObject
{
    Q_OBJECT

private slots:
    void validNetworkConfiguration();
    void invalidConfigurationsAreRejected();
    void refreshResultReportsUsableDescriptors();
    void providerExposesTransportIndependentResult();
    void refreshRetriesAndIsolatesUntrustedSources();
    void refreshKeepsSourceFailuresIndependent();
    void rejectsInvalidNetworkDescriptorAndDuplicates();

private:
    static ComponentDescriptor validNetworkDescriptor(const QString &sourceId,
                                                      const QString &identity)
    {
        ComponentDescriptor descriptor;
        descriptor.identity = identity;
        descriptor.source = ComponentSource::Network;
        descriptor.sourceId = sourceId;
        descriptor.status = ComponentStatus::Available;
        descriptor.trustState = TrustState::Trusted;
        descriptor.execution.mode = ExecutionMode::RemoteWorker;
        descriptor.execution.workerCapabilityReference = QStringLiteral("worker.capability");
        descriptor.execution.protocol = QStringLiteral("grpc");
        descriptor.execution.errorBehavior = QStringLiteral("structured");
        descriptor.execution.inputRequirements = {QStringLiteral("input")};
        descriptor.execution.outputGuarantees = {QStringLiteral("output")};
        return descriptor;
    };
};

void tst_ComponentDiscovery::validNetworkConfiguration()
{
    DiscoverySourceConfig config;
    config.sourceId = QStringLiteral("network.example");
    config.kind = DiscoverySourceKind::Network;
    config.endpoint = QStringLiteral("https://example.invalid/catalog");
    config.timeoutMs = 2500;
    config.retryPolicy.maxAttempts = 3;
    config.retryPolicy.initialBackoffMs = 50;
    config.retryPolicy.maxBackoffMs = 500;

    QVERIFY(config.isValid());
    QCOMPARE(discoverySourceKindName(config.kind), QStringLiteral("network"));
    QCOMPARE(discoveryHealthName(DiscoveryHealth::Healthy), QStringLiteral("healthy"));
}

void tst_ComponentDiscovery::invalidConfigurationsAreRejected()
{
    DiscoverySourceConfig missingId;
    QVERIFY(!missingId.isValid());

    DiscoverySourceConfig networkWithoutEndpoint;
    networkWithoutEndpoint.sourceId = QStringLiteral("network.example");
    networkWithoutEndpoint.kind = DiscoverySourceKind::Network;
    QVERIFY(!networkWithoutEndpoint.isValid());

    DiscoverySourceConfig invalidRetry;
    invalidRetry.sourceId = QStringLiteral("network.example");
    invalidRetry.kind = DiscoverySourceKind::Network;
    invalidRetry.endpoint = QStringLiteral("https://example.invalid/catalog");
    invalidRetry.retryPolicy.maxAttempts = 0;
    QVERIFY(!invalidRetry.isValid());
}

void tst_ComponentDiscovery::refreshResultReportsUsableDescriptors()
{
    DiscoveryRefreshResult result;
    result.sourceId = QStringLiteral("network.example");
    result.health = DiscoveryHealth::Healthy;
    result.success = true;

    ComponentDescriptor descriptor;
    descriptor.identity = QStringLiteral("network.component");
    descriptor.source = ComponentSource::Network;
    descriptor.sourceId = result.sourceId;
    result.descriptors.append(descriptor);

    QVERIFY(result.hasUsableDescriptors());
    result.success = false;
    QVERIFY(!result.hasUsableDescriptors());
}

void tst_ComponentDiscovery::providerExposesTransportIndependentResult()
{
    FakeNetworkSource source;
    QCOMPARE(source.sourceId(), QStringLiteral("fake.source"));
    QVERIFY(source.configuration().isValid());
    QVERIFY(source.discover().success);
}

void tst_ComponentDiscovery::refreshRetriesAndIsolatesUntrustedSources()
{
    ControlledNetworkSource trusted(QStringLiteral("trusted.source"), true, 2);
    ControlledNetworkSource untrusted(QStringLiteral("untrusted.source"), false, 0);
    ComponentDiscovery discovery;
    QVERIFY(discovery.addSource(&trusted));
    QVERIFY(discovery.addSource(&untrusted));

    const DiscoveryRefreshResult trustedResult = discovery.refreshSource(trusted.sourceId());
    QVERIFY(trustedResult.success);  // validated remote-worker fixture.
    QCOMPARE(trusted.calls, 3);
    QCOMPARE(discovery.sourceState(trusted.sourceId()).health, DiscoveryHealth::Healthy);

    const DiscoveryRefreshResult untrustedResult = discovery.refreshSource(untrusted.sourceId());
    QVERIFY(!untrustedResult.success);
    QCOMPARE(untrusted.calls, 0);
    QCOMPARE(untrustedResult.diagnostics.first().code, QStringLiteral("untrusted_source"));
}

void tst_ComponentDiscovery::refreshKeepsSourceFailuresIndependent()
{
    ControlledNetworkSource failing(QStringLiteral("failing.source"), true, 3);
    ControlledNetworkSource healthy(QStringLiteral("healthy.source"), true, 0);
    ComponentDiscovery discovery;
    QVERIFY(discovery.addSource(&failing));
    QVERIFY(discovery.addSource(&healthy));

    const QList<DiscoveryRefreshResult> results = discovery.refreshAll();
    QCOMPARE(results.size(), 2);
    QCOMPARE(results.at(0).sourceId, QStringLiteral("failing.source"));
    QCOMPARE(results.at(1).sourceId, QStringLiteral("healthy.source"));
    QVERIFY(!results.at(0).success);
    QVERIFY(results.at(1).success);
}

void tst_ComponentDiscovery::rejectsInvalidNetworkDescriptorAndDuplicates()
{
    DiscoverySourceConfig config;
    config.sourceId = QStringLiteral("trusted.source");
    config.kind = DiscoverySourceKind::Network;
    config.endpoint = QStringLiteral("in-memory://catalog");
    config.trusted = true;

    NetworkDiscoveryResult result;
    result.success = true;
    result.descriptors.append(validNetworkDescriptor(config.sourceId, QStringLiteral("network.component")));
    result.descriptors.append(validNetworkDescriptor(config.sourceId, QStringLiteral("network.component")));

    const QList<ComponentDiagnostic> duplicateDiagnostics =
        validateNetworkDiscoveryResult(result, config);
    QVERIFY(std::any_of(duplicateDiagnostics.cbegin(), duplicateDiagnostics.cend(),
                        [](const ComponentDiagnostic &diagnostic) {
                            return diagnostic.code == QStringLiteral("descriptor_conflict");
                        }));

    ComponentDescriptor invalid = validNetworkDescriptor(config.sourceId,
                                                          QStringLiteral("invalid.component"));
    invalid.trustState = TrustState::Unknown;
    const QList<ComponentDiagnostic> invalidDiagnostics =
        validateNetworkComponentDescriptor(invalid, config);
    QVERIFY(std::any_of(invalidDiagnostics.cbegin(), invalidDiagnostics.cend(),
                        [](const ComponentDiagnostic &diagnostic) {
                            return diagnostic.code == QStringLiteral("untrusted_descriptor");
                        }));
}

QTEST_MAIN(tst_ComponentDiscovery)
#include "tst_ComponentDiscovery.moc"
