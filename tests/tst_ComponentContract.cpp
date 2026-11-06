#include "component_map_runtime/ComponentContract.h"

#include <QtTest>

using namespace cme::runtime;

class ComponentContractTests final : public QObject
{
    Q_OBJECT
private slots:
    void validBuiltinDescriptorPasses();
    void duplicateIdsAreRejected();
    void invalidDefaultIsRejected();
    void networkRequiresTrustAndBinding();
    void unsupportedVersionShapeIsRejected();
};

static ComponentDescriptor builtin()
{
    ComponentDescriptor descriptor;
    descriptor.identity = QStringLiteral("example.component");
    descriptor.sourceId = QStringLiteral("builtin.example");
    descriptor.status = ComponentStatus::Available;
    descriptor.title = QStringLiteral("Example");
    descriptor.execution.inputRequirements = {QStringLiteral("event")};
    descriptor.execution.outputGuarantees = {QStringLiteral("event")};
    descriptor.execution.errorBehavior = QStringLiteral("structured");
    descriptor.properties.append({QStringLiteral("name"), QStringLiteral("string"), true,
                                  QStringLiteral("demo"), {}, QStringLiteral("Name"), {}});
    descriptor.ports.append({QStringLiteral("in"), QStringLiteral("input"), QStringLiteral("event"),
                             QStringLiteral("single"), true, {}, {}});
    return descriptor;
}

void ComponentContractTests::validBuiltinDescriptorPasses()
{
    const auto errors = validateComponentDescriptor(builtin());
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join(';')));
}

void ComponentContractTests::duplicateIdsAreRejected()
{
    auto descriptor = builtin();
    descriptor.properties.append(descriptor.properties.constFirst());
    const auto errors = validateComponentDescriptor(descriptor);
    QVERIFY(errors.join(';').contains(QStringLiteral("duplicate id")));
}

void ComponentContractTests::invalidDefaultIsRejected()
{
    auto descriptor = builtin();
    descriptor.properties[0].constraints.insert(QStringLiteral("enum"), QStringLiteral("one,two"));
    descriptor.properties[0].defaultValue = QStringLiteral("three");
    QVERIFY(validateComponentDescriptor(descriptor).join(';').contains(QStringLiteral("not in enum")));
}

void ComponentContractTests::networkRequiresTrustAndBinding()
{
    auto descriptor = builtin();
    descriptor.source = ComponentSource::Network;
    descriptor.sourceId = QStringLiteral("network.example");
    descriptor.execution.mode = ExecutionMode::RemoteWorker;
    const auto errors = validateComponentDescriptor(descriptor);
    QVERIFY(errors.join(';').contains(QStringLiteral("not trusted")));
    QVERIFY(errors.join(';').contains(QStringLiteral("capability reference")));
}

void ComponentContractTests::unsupportedVersionShapeIsRejected()
{
    auto descriptor = builtin();
    descriptor.contractVersion = QStringLiteral("v2");
    QVERIFY(validateComponentDescriptor(descriptor).join(';').contains(QStringLiteral("semantic version")));
}

QTEST_MAIN(ComponentContractTests)
#include "tst_ComponentContract.moc"
