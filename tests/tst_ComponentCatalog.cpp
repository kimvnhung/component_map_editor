#include "component_map_runtime/ComponentCatalog.h"

#include <QtTest>

using namespace cme::runtime;

class ComponentCatalogTests final : public QObject
{
    Q_OBJECT
private slots:
    void registrationAndSnapshotAreDeterministic();
    void equivalentRegistrationIsIdempotent();
    void conflictingRegistrationIsRejected();
    void filteringAndStatusTransitionsWork();
};

static ComponentDescriptor descriptor(const QString &identity, const QString &sourceId)
{
    ComponentDescriptor result;
    result.identity = identity;
    result.sourceId = sourceId;
    result.status = ComponentStatus::Available;
    result.title = identity;
    result.execution.inputRequirements = {QStringLiteral("event")};
    result.execution.outputGuarantees = {QStringLiteral("event")};
    result.execution.errorBehavior = QStringLiteral("structured");
    return result;
}

void ComponentCatalogTests::registrationAndSnapshotAreDeterministic()
{
    ComponentCatalog catalog;
    QVERIFY(catalog.registerDescriptor(descriptor(QStringLiteral("z"), QStringLiteral("s"))));
    QVERIFY(catalog.registerDescriptor(descriptor(QStringLiteral("a"), QStringLiteral("s"))));
    const auto entries = catalog.snapshot();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[0].identity, QStringLiteral("a"));
    QCOMPARE(entries[1].identity, QStringLiteral("z"));
    QCOMPARE(catalog.snapshotVersion(), quint64(2));
}

void ComponentCatalogTests::equivalentRegistrationIsIdempotent()
{
    ComponentCatalog catalog;
    const auto value = descriptor(QStringLiteral("same"), QStringLiteral("s"));
    QVERIFY(catalog.registerDescriptor(value));
    ComponentDiagnostic diagnostic;
    QVERIFY(!catalog.registerDescriptor(value, &diagnostic));
    QCOMPARE(diagnostic.code, QStringLiteral("already_registered"));
    QCOMPARE(catalog.snapshot().size(), 1);
}

void ComponentCatalogTests::conflictingRegistrationIsRejected()
{
    ComponentCatalog catalog;
    auto first = descriptor(QStringLiteral("same"), QStringLiteral("s"));
    auto second = first;
    second.title = QStringLiteral("changed");
    QVERIFY(catalog.registerDescriptor(first));
    ComponentDiagnostic diagnostic;
    QVERIFY(!catalog.registerDescriptor(second, &diagnostic));
    QCOMPARE(diagnostic.code, QStringLiteral("descriptor_conflict"));
}

void ComponentCatalogTests::filteringAndStatusTransitionsWork()
{
    ComponentCatalog catalog;
    auto value = descriptor(QStringLiteral("one"), QStringLiteral("network"));
    value.source = ComponentSource::Network;
    value.trustState = TrustState::Trusted;
    value.execution.mode = ExecutionMode::RemoteWorker;
    value.execution.workerCapabilityReference = QStringLiteral("worker");
    value.execution.protocol = QStringLiteral("event-v1");
    QVERIFY(catalog.registerDescriptor(value));

    CatalogFilter filter;
    filter.source = ComponentSource::Network;
    filter.filterSource = true;
    QCOMPARE(catalog.snapshot(filter).size(), 1);

    const auto key = componentKey(value);
    QVERIFY(catalog.transitionStatus(key, ComponentStatus::Unavailable));
    filter.status = ComponentStatus::Unavailable;
    filter.filterStatus = true;
    QCOMPARE(catalog.snapshot(filter).size(), 1);
    QVERIFY(catalog.remove(key));
    QVERIFY(catalog.snapshot().isEmpty());
}

QTEST_MAIN(ComponentCatalogTests)
#include "tst_ComponentCatalog.moc"
