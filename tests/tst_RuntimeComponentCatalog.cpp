#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/ComponentDiscoveryTypes.h"

#include <QtTest>

using namespace cme::runtime;

class RuntimeComponentCatalogTests final : public QObject
{
    Q_OBJECT
private slots:
    void emptyCatalogReturnsSuccessfulSnapshot();
    void builtInDescriptorContainsSharedMetadata();
    void categoryAndSourceFiltersAreIndependent();
};

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
