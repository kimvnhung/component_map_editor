#include <QtTest>

#include "component_map_runtime/ComponentDiscovery.h"
#include "component_map_runtime/NetworkComponentSource.h"

using namespace cme::runtime;

class tst_ComponentDiscovery : public QObject
{
    Q_OBJECT

private slots:
    void validNetworkConfiguration();
    void invalidConfigurationsAreRejected();
    void refreshResultReportsUsableDescriptors();
    void providerExposesTransportIndependentResult();
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
    config.trusted = false;

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

namespace
{
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

void tst_ComponentDiscovery::providerExposesTransportIndependentResult()
{
    FakeNetworkSource source;
    QCOMPARE(source.sourceId(), QStringLiteral("fake.source"));
    QVERIFY(source.configuration().isValid());
    QVERIFY(source.discover().success);
}

QTEST_MAIN(tst_ComponentDiscovery)
#include "tst_ComponentDiscovery.moc"
