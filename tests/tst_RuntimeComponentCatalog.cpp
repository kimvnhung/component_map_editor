#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/ComponentDiscoveryTypes.h"

#include <QtTest>

#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/ComponentDiscovery.h"
#include "component_map_runtime/NetworkComponentSource.h"

#include <algorithm>
#include <utility>

namespace
{
class CatalogNetworkSource final : public cme::runtime::NetworkComponentSource
{
public:
    CatalogNetworkSource(QString id, bool trusted, bool succeeds)
        : m_id(std::move(id)), m_trusted(trusted), m_succeeds(succeeds)
    {
    }

    QString sourceId() const override { return m_id; }

    cme::runtime::DiscoverySourceConfig configuration() const override
    {
        cme::runtime::DiscoverySourceConfig config;
        config.sourceId = m_id;
        config.kind = cme::runtime::DiscoverySourceKind::Network;
        config.endpoint = QStringLiteral("in-memory://catalog");
        config.trusted = m_trusted;
        return config;
    }

    cme::runtime::NetworkDiscoveryResult discover() override
    {
        cme::runtime::NetworkDiscoveryResult result;
        result.success = m_succeeds;
        if (m_succeeds)
        {
            cme::runtime::ComponentDescriptor descriptor;
            descriptor.identity = m_id + QStringLiteral(".component");
            descriptor.source = cme::runtime::ComponentSource::Network;
            descriptor.sourceId = m_id;
            descriptor.status = cme::runtime::ComponentStatus::Available;
            descriptor.trustState = cme::runtime::TrustState::Trusted;
            descriptor.execution.mode = cme::runtime::ExecutionMode::RemoteWorker;
            descriptor.execution.workerCapabilityReference = QStringLiteral("worker.capability");
            descriptor.execution.protocol = QStringLiteral("grpc");
            descriptor.execution.errorBehavior = QStringLiteral("structured");
            descriptor.execution.inputRequirements = {QStringLiteral("input")};
            descriptor.execution.outputGuarantees = {QStringLiteral("output")};
            result.descriptors.append(descriptor);
        }
        return result;
    }

private:
    QString m_id;
    bool m_trusted;
    bool m_succeeds;
};
}

using namespace cme::runtime;

class RuntimeComponentCatalogTests final : public QObject
{
    Q_OBJECT
private slots:
    void emptyCatalogReturnsSuccessfulSnapshot();
    void builtInDescriptorContainsSharedMetadata();
    void categoryAndSourceFiltersAreIndependent();
    void networkCatalogSourceIsolation();
};

void RuntimeComponentCatalogTests::networkCatalogSourceIsolation()
{
    ComponentCatalog catalog;
    ComponentDiscovery discovery(&catalog);
    CatalogNetworkSource healthy(QStringLiteral("healthy.source"), true, true);
    CatalogNetworkSource failing(QStringLiteral("failing.source"), true, false);
    QVERIFY(discovery.addSource(&healthy));
    QVERIFY(discovery.addSource(&failing));
    const auto results = discovery.refreshAll();
    QCOMPARE(results.size(), 2);
    QCOMPARE(results.at(0).sourceId, QStringLiteral("failing.source"));
    QCOMPARE(results.at(1).sourceId, QStringLiteral("healthy.source"));
    QVERIFY(!results.at(0).success);
    QVERIFY(results.at(1).success);
    QVERIFY(!catalog.snapshot().isEmpty());
}

static ComponentDescriptor builtIn(const QString &identity, const QString &category)
{
    ComponentDescriptor descriptor;
    descriptor.identity = identity;
    descriptor.sourceId = QStringLiteral("builtin.test");
    descriptor.status = ComponentStatus::Available;
    descriptor.trustState = TrustState::Trusted;
    descriptor.title = identity;
    descriptor.category = category;
    descriptor.execution.inputRequirements = {QStringLiteral("event")};
    descriptor.execution.outputGuarantees = {QStringLiteral("event")};
    descriptor.execution.errorBehavior = QStringLiteral("structured");
    descriptor.properties.append({QStringLiteral("value"), QStringLiteral("string"), false,
                                  QStringLiteral("default"), {}, QStringLiteral("Value"), {}});
    descriptor.ports.append({QStringLiteral("input"), QStringLiteral("input"), QStringLiteral("event"),
                             QStringLiteral("single"), false, {}, {}});
    return descriptor;
}

void RuntimeComponentCatalogTests::emptyCatalogReturnsSuccessfulSnapshot()
{
    ComponentCatalog catalog;
    QVERIFY(catalog.snapshot().isEmpty());
    QCOMPARE(catalog.snapshotVersion(), quint64(0));
}

void RuntimeComponentCatalogTests::builtInDescriptorContainsSharedMetadata()
{
    ComponentCatalog catalog;
    const auto value = builtIn(QStringLiteral("builtin.example"), QStringLiteral("factory"));
    QVERIFY(catalog.registerDescriptor(value));
    const auto entries = catalog.snapshot();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.constFirst().source, ComponentSource::Builtin);
    QCOMPARE(entries.constFirst().execution.mode, ExecutionMode::Builtin);
    QCOMPARE(entries.constFirst().properties.size(), 1);
    QCOMPARE(entries.constFirst().ports.size(), 1);
}

void RuntimeComponentCatalogTests::categoryAndSourceFiltersAreIndependent()
{
    ComponentCatalog catalog;
    QVERIFY(catalog.registerDescriptor(builtIn(QStringLiteral("a"), QStringLiteral("factory"))));
    auto network = builtIn(QStringLiteral("b"), QStringLiteral("factory"));
    network.source = ComponentSource::Network;
    network.sourceId = QStringLiteral("network.test");
    network.execution.mode = ExecutionMode::RemoteWorker;
    network.execution.workerCapabilityReference = QStringLiteral("worker");
    network.execution.protocol = QStringLiteral("event-v1");
    QVERIFY(catalog.registerDescriptor(network));

    CatalogFilter filter;
    filter.category = QStringLiteral("factory");
    filter.filterSource = true;
    filter.source = ComponentSource::Builtin;
    QCOMPARE(catalog.snapshot(filter).size(), 1);
    QCOMPARE(catalog.snapshot(filter).constFirst().identity, QStringLiteral("a"));
}

QTEST_MAIN(RuntimeComponentCatalogTests)
#include "tst_RuntimeComponentCatalog.moc"
