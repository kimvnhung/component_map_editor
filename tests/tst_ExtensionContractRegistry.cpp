#include <QtTest/QtTest>

#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IComponentTypeProvider.h"
#include "extensions/contracts/IConnectionPolicyProvider.h"
#include "extensions/contracts/ExtensionManifest.h"
#include "extensions/sample_pack/SampleComponentTypeProvider.h"
#include "extensions/sample_pack/SampleExtensionPack.h"
#include "extensions/sample_pack/SampleValidationProvider.h"

namespace {

struct StubComponentTypeProviderById : public IComponentTypeProvider
{
    explicit StubComponentTypeProviderById(const QString &id) : m_id(id) {}
    QString providerId() const override { return m_id; }
    QStringList componentTypeIds() const override { return {}; }
    QVariantMap componentTypeDescriptor(const QString &) const override { return {}; }
    QVariantMap defaultComponentProperties(const QString &) const override { return {}; }
    QString m_id;
};

struct StubConnectionPolicyProviderById : public IConnectionPolicyProvider
{
    explicit StubConnectionPolicyProviderById(const QString &id) : m_id(id) {}
    QString providerId() const override { return m_id; }
    bool canConnect(const cme::ConnectionPolicyContext &, QString *) const override { return true; }
    QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &, const QVariantMap &raw) const override { return raw; }
    QString m_id;
};

ExtensionManifest validManifest(const QString &id = QStringLiteral("test.pack"))
{
    ExtensionManifest manifest;
    manifest.extensionId = id;
    manifest.displayName = QStringLiteral("Test Pack");
    manifest.extensionVersion = QStringLiteral("1.0.0");
    manifest.minCoreApi = { 1, 0, 0 };
    manifest.maxCoreApi = { 1, 99, 99 };
    return manifest;
}

cme::ConnectionPolicyContext makePolicyContext(const QString &sourceType,
                                               const QString &targetType)
{
    cme::ConnectionPolicyContext context;
    context.set_source_type_id(sourceType.toStdString());
    context.set_target_type_id(targetType.toStdString());
    return context;
}

ExtensionContractRegistry registryV1()
{
    return ExtensionContractRegistry({ 1, 0, 0 });
}

QVariantMap minimalValidSnapshot()
{
    QVariantMap startComponent;
    startComponent[QStringLiteral("id")] = QStringLiteral("n1");
    startComponent[QStringLiteral("type")] = QStringLiteral("start");

    QVariantMap processComponent;
    processComponent[QStringLiteral("id")] = QStringLiteral("n2");
    processComponent[QStringLiteral("type")] = QStringLiteral("process");

    QVariantMap stopComponent;
    stopComponent[QStringLiteral("id")] = QStringLiteral("n3");
    stopComponent[QStringLiteral("type")] = QStringLiteral("stop");

    QVariantMap conn1;
    conn1[QStringLiteral("id")] = QStringLiteral("e1");
    conn1[QStringLiteral("sourceId")] = QStringLiteral("n1");
    conn1[QStringLiteral("targetId")] = QStringLiteral("n2");

    QVariantMap conn2;
    conn2[QStringLiteral("id")] = QStringLiteral("e2");
    conn2[QStringLiteral("sourceId")] = QStringLiteral("n2");
    conn2[QStringLiteral("targetId")] = QStringLiteral("n3");

    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{ startComponent, processComponent, stopComponent };
    snapshot[QStringLiteral("connections")] = QVariantList{ conn1, conn2 };
    return snapshot;
}

bool hasIssueWithCode(const QVariantList &issues, const QString &code)
{
    for (const QVariant &value : issues) {
        if (value.toMap().value(QStringLiteral("code")).toString() == code)
            return true;
    }
    return false;
}

} // namespace

class ExtensionContractRegistryTests : public QObject
{
    Q_OBJECT

private slots:
    void manifestValidationRejectsEmptyId();
    void manifestValidationRejectsEmptyDisplayName();
    void manifestValidationRejectsEmptyVersion();
    void manifestValidationRejectsInvalidApiRange();
    void compatibilityRejectsCoreTooOld();
    void compatibilityRejectsCoreMajorTooNew();
    void compatibilityAcceptsCoreBelowMaxMinor();
    void duplicateManifestRejected();

    void samplePackRegistersSuccessfully();
    void samplePackManifestIsPresent();
    void samplePackSecondRegisterFails();

    void componentTypeProviderReturnsThreeTypes();
    void componentTypeDescriptorContainsRequiredKeys();
    void componentTypeDefaultsForProcess();
    void componentTypeDefaultsForStartHasInputNumber();
    void componentTypeDefaultsForStopIsEmpty();

    void connectionPolicyAllowsStartToProcess();
    void connectionPolicyAllowsProcessToProcess();
    void connectionPolicyAllowsProcessToStop();
    void connectionPolicyDeniesStopToProcess();
    void connectionPolicyAllowsProcessToStopWithProperties();
    void connectionPolicyDeniesStartAsTarget();
    void connectionPolicyDeniesStopAsSource();
    void connectionPolicyDeniesStartToStop();
    void connectionPolicyDeniesStartToStopWithProperties();
    void connectionPolicyNormalizeAddsFlowType();
    void connectionPolicyNormalizePreservesExistingType();

    void propertySchemaTargetsCoverAllComponentTypes();
    void propertySchemaForProcessHasDynamicLayoutEntries();
    void propertySchemaEntriesContainRequiredKeys();
    void propertySchemaForStartHasInputNumberEntry();
    void propertySchemaForFlowConnectionHasLabelEntry();
    void propertySchemaForUnknownTargetIsEmpty();

    void validationAcceptsMinimalValidGraph();
    void validationDetectsMissingStartComponent();
    void validationDetectsMultipleStartComponents();
    void validationDetectsMissingStopComponent();
    void validationDetectsDanglingConnectionSource();
    void validationDetectsDanglingConnectionTarget();
    void validationWarnsAboutIsolatedComponent();

    void actionProviderExposesSetTaskPriorityId();
    void actionDescriptorContainsRequiredKeys();
    void actionInvokeProducesCorrectCommandRequest();
    void actionInvokeFailsWithMissingComponentId();
    void actionInvokeFailsWithInvalidPriority();
    void actionInvokeFailsWithUnknownActionId();

    void componentTypeProvidersReturnedInRegistrationOrder();
    void connectionPolicyProvidersReturnedInRegistrationOrder();
};

void ExtensionContractRegistryTests::manifestValidationRejectsEmptyId()
{
    ExtensionManifest manifest = validManifest();
    manifest.extensionId = QString();
    QString error;
    QVERIFY(!manifest.isValid(&error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::manifestValidationRejectsEmptyDisplayName()
{
    ExtensionManifest manifest = validManifest();
    manifest.displayName = QString();
    QVERIFY(!manifest.isValid());
}

void ExtensionContractRegistryTests::manifestValidationRejectsEmptyVersion()
{
    ExtensionManifest manifest = validManifest();
    manifest.extensionVersion = QString();
    QVERIFY(!manifest.isValid());
}

void ExtensionContractRegistryTests::manifestValidationRejectsInvalidApiRange()
{
    ExtensionManifest manifest = validManifest();
    manifest.minCoreApi = { 2, 0, 0 };
    manifest.maxCoreApi = { 1, 0, 0 };
    QVERIFY(!manifest.isValid());
}

void ExtensionContractRegistryTests::compatibilityRejectsCoreTooOld()
{
    ExtensionContractRegistry registry({ 0, 9, 0 });
    QString error;
    QVERIFY(!registry.registerManifest(validManifest(), &error));
    QVERIFY(error.contains(QStringLiteral("older")) || error.contains(QStringLiteral("Too")));
}

void ExtensionContractRegistryTests::compatibilityRejectsCoreMajorTooNew()
{
    ExtensionContractRegistry registry({ 2, 0, 0 });
    QString error;
    QVERIFY(!registry.registerManifest(validManifest(), &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::compatibilityAcceptsCoreBelowMaxMinor()
{
    ExtensionContractRegistry registry({ 1, 5, 0 });
    QVERIFY(registry.registerManifest(validManifest()));
}

void ExtensionContractRegistryTests::duplicateManifestRejected()
{
    auto registry = registryV1();
    QVERIFY(registry.registerManifest(validManifest(QStringLiteral("dup.pack"))));
    QString error;
    QVERIFY(!registry.registerManifest(validManifest(QStringLiteral("dup.pack")), &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::samplePackRegistersSuccessfully()
{
    auto registry = registryV1();
    SampleExtensionPack pack;
    QString error;
    QVERIFY2(pack.registerAll(registry, &error), qPrintable(error));
}

void ExtensionContractRegistryTests::samplePackManifestIsPresent()
{
    auto registry = registryV1();
    SampleExtensionPack pack;
    QVERIFY(pack.registerAll(registry));
    QVERIFY(registry.hasManifest(QString::fromLatin1(SampleExtensionPack::ExtensionId)));
    const ExtensionManifest manifest = registry.manifest(QString::fromLatin1(SampleExtensionPack::ExtensionId));
    QCOMPARE(manifest.extensionId, QString::fromLatin1(SampleExtensionPack::ExtensionId));
    QCOMPARE(manifest.displayName, QString::fromLatin1(SampleExtensionPack::DisplayName));
}

void ExtensionContractRegistryTests::samplePackSecondRegisterFails()
{
    auto registry = registryV1();
    SampleExtensionPack pack;
    QVERIFY(pack.registerAll(registry));
    QString error;
    QVERIFY(!pack.registerAll(registry, &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::componentTypeProviderReturnsThreeTypes()
{
    SampleComponentTypeProvider provider;
    QCOMPARE(provider.componentTypeIds().size(), 3);
    QVERIFY(provider.componentTypeIds().contains(QString::fromLatin1(SampleComponentTypeProvider::TypeStart)));
    QVERIFY(provider.componentTypeIds().contains(QString::fromLatin1(SampleComponentTypeProvider::TypeProcess)));
    QVERIFY(provider.componentTypeIds().contains(QString::fromLatin1(SampleComponentTypeProvider::TypeStop)));
}

void ExtensionContractRegistryTests::componentTypeDescriptorContainsRequiredKeys()
{
    SampleComponentTypeProvider provider;
    for (const QString &typeId : provider.componentTypeIds()) {
        const QVariantMap descriptor = provider.componentTypeDescriptor(typeId);
        QVERIFY2(!descriptor.isEmpty(), qPrintable(typeId));
        QVERIFY2(descriptor.contains(QStringLiteral("id")), qPrintable(typeId));
        QVERIFY2(descriptor.contains(QStringLiteral("title")), qPrintable(typeId));
        QVERIFY2(descriptor.contains(QStringLiteral("defaultWidth")), qPrintable(typeId));
        QVERIFY2(descriptor.contains(QStringLiteral("defaultHeight")), qPrintable(typeId));
        QVERIFY2(descriptor.contains(QStringLiteral("defaultColor")), qPrintable(typeId));
        QVERIFY2(descriptor.contains(QStringLiteral("allowIncoming")), qPrintable(typeId));
        QVERIFY2(descriptor.contains(QStringLiteral("allowOutgoing")), qPrintable(typeId));
    }
}

void ExtensionContractRegistryTests::componentTypeDefaultsForProcess()
{
    SampleComponentTypeProvider provider;
    const QVariantMap defaults = provider.defaultComponentProperties(
        QString::fromLatin1(SampleComponentTypeProvider::TypeProcess));
    QVERIFY(defaults.contains(QStringLiteral("addValue")));
    QCOMPARE(defaults.value(QStringLiteral("addValue")).toInt(), 9);
    QVERIFY(defaults.contains(QStringLiteral("description")));
}

void ExtensionContractRegistryTests::componentTypeDefaultsForStartHasInputNumber()
{
    SampleComponentTypeProvider provider;
    const QVariantMap defaults = provider.defaultComponentProperties(
        QString::fromLatin1(SampleComponentTypeProvider::TypeStart));
    QVERIFY(defaults.contains(QStringLiteral("inputNumber")));
    QCOMPARE(defaults.value(QStringLiteral("inputNumber")).toInt(), 0);
}

void ExtensionContractRegistryTests::componentTypeDefaultsForStopIsEmpty()
{
    SampleComponentTypeProvider provider;
    QVERIFY(provider.defaultComponentProperties(QString::fromLatin1(SampleComponentTypeProvider::TypeStop)).isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyAllowsStartToProcess()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("start"), QStringLiteral("process"));
    QVERIFY(provider.canConnect(context, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsProcessToProcess()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("process"), QStringLiteral("process"));
    QVERIFY(provider.canConnect(context, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsProcessToStop()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("process"), QStringLiteral("stop"));
    QVERIFY(provider.canConnect(context, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStopToProcess()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("stop"), QStringLiteral("process"));
    QVERIFY(!provider.canConnect(context, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsProcessToStopWithProperties()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("process"), QStringLiteral("stop"));
    QVERIFY(provider.canConnect(context, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartAsTarget()
{
    SampleConnectionPolicyProvider provider;
    QString reason;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("process"), QStringLiteral("start"));
    QVERIFY(!provider.canConnect(context, &reason));
    QVERIFY(!reason.isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStopAsSource()
{
    SampleConnectionPolicyProvider provider;
    QString reason;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("stop"), QStringLiteral("process"));
    QVERIFY(!provider.canConnect(context, &reason));
    QVERIFY(!reason.isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartToStop()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("start"), QStringLiteral("stop"));
    QVERIFY(!provider.canConnect(context, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartToStopWithProperties()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("start"), QStringLiteral("stop"));
    QVERIFY(!provider.canConnect(context, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyNormalizeAddsFlowType()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("process"), QStringLiteral("process"));
    const QVariantMap result = provider.normalizeConnectionProperties(
        context, {});
    QCOMPARE(result.value(QStringLiteral("type")).toString(), QStringLiteral("flow"));
}

void ExtensionContractRegistryTests::connectionPolicyNormalizePreservesExistingType()
{
    SampleConnectionPolicyProvider provider;
    const cme::ConnectionPolicyContext context = makePolicyContext(QStringLiteral("process"), QStringLiteral("process"));
    QVariantMap raw;
    raw[QStringLiteral("type")] = QStringLiteral("dependency");
    const QVariantMap result = provider.normalizeConnectionProperties(
        context, raw);
    QCOMPARE(result.value(QStringLiteral("type")).toString(), QStringLiteral("dependency"));
}

void ExtensionContractRegistryTests::propertySchemaTargetsCoverAllComponentTypes()
{
    SamplePropertySchemaProvider provider;
    const QStringList targets = provider.schemaTargets();
    QVERIFY(targets.contains(QStringLiteral("component/start")));
    QVERIFY(targets.contains(QStringLiteral("component/process")));
    QVERIFY(targets.contains(QStringLiteral("component/stop")));
    QVERIFY(targets.contains(QStringLiteral("connection/flow")));
}

void ExtensionContractRegistryTests::propertySchemaForProcessHasDynamicLayoutEntries()
{
    SamplePropertySchemaProvider provider;
    const QVariantList schema = provider.propertySchema(QStringLiteral("component/process"));
    QVERIFY(schema.size() >= 6);

    bool hasSection = false;
    bool hasOrder = false;
    bool hasNumericBehavior = false;

    for (const QVariant &value : schema) {
        const QVariantMap row = value.toMap();
        hasSection = hasSection || row.contains(QStringLiteral("section"));
        hasOrder = hasOrder || row.contains(QStringLiteral("order"));
        if (row.value(QStringLiteral("key")).toString() == QStringLiteral("addValue"))
            hasNumericBehavior = true;
    }

    QVERIFY(hasSection);
    QVERIFY(hasOrder);
    QVERIFY(hasNumericBehavior);
}

void ExtensionContractRegistryTests::propertySchemaEntriesContainRequiredKeys()
{
    SamplePropertySchemaProvider provider;
    for (const QString &target : provider.schemaTargets()) {
        const QVariantList schema = provider.propertySchema(target);
        for (const QVariant &value : schema) {
            const QVariantMap entry = value.toMap();
            QVERIFY2(entry.contains(QStringLiteral("key")), qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("type")), qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("title")), qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("required")), qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("defaultValue")), qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("editor")), qPrintable(target));
        }
    }
}

void ExtensionContractRegistryTests::propertySchemaForStartHasInputNumberEntry()
{
    SamplePropertySchemaProvider provider;
    const QVariantList schema = provider.propertySchema(QStringLiteral("component/start"));
    bool found = false;
    for (const QVariant &value : schema) {
        if (value.toMap().value(QStringLiteral("key")).toString() == QStringLiteral("inputNumber"))
            found = true;
    }
    QVERIFY(found);
}

void ExtensionContractRegistryTests::propertySchemaForFlowConnectionHasLabelEntry()
{
    SamplePropertySchemaProvider provider;
    const QVariantList schema = provider.propertySchema(QStringLiteral("connection/flow"));
    QVERIFY(!schema.isEmpty());
    QCOMPARE(schema.first().toMap().value(QStringLiteral("key")).toString(),
             QStringLiteral("label"));
}

void ExtensionContractRegistryTests::propertySchemaForUnknownTargetIsEmpty()
{
    SamplePropertySchemaProvider provider;
    QVERIFY(provider.propertySchema(QStringLiteral("component/unknown")).isEmpty());
}

void ExtensionContractRegistryTests::validationAcceptsMinimalValidGraph()
{
    SampleValidationProvider provider;
    const QVariantList issues = provider.validateGraph(minimalValidSnapshot());
    QVERIFY(!hasIssueWithCode(issues, QStringLiteral("W001")));
    QVERIFY(!hasIssueWithCode(issues, QStringLiteral("W002")));
    QVERIFY(!hasIssueWithCode(issues, QStringLiteral("W003")));
}

void ExtensionContractRegistryTests::validationDetectsMissingStartComponent()
{
    SampleValidationProvider provider;
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{};
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(provider.validateGraph(snapshot), QStringLiteral("W001")));
}

void ExtensionContractRegistryTests::validationDetectsMultipleStartComponents()
{
    SampleValidationProvider provider;
    QVariantMap c1;
    c1[QStringLiteral("id")] = QStringLiteral("s1");
    c1[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap c2;
    c2[QStringLiteral("id")] = QStringLiteral("s2");
    c2[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap stopComponent;
    stopComponent[QStringLiteral("id")] = QStringLiteral("e1");
    stopComponent[QStringLiteral("type")] = QStringLiteral("stop");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{ c1, c2, stopComponent };
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(provider.validateGraph(snapshot), QStringLiteral("W001")));
}

void ExtensionContractRegistryTests::validationDetectsMissingStopComponent()
{
    SampleValidationProvider provider;
    QVariantMap startComponent;
    startComponent[QStringLiteral("id")] = QStringLiteral("n1");
    startComponent[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{ startComponent };
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(provider.validateGraph(snapshot), QStringLiteral("W002")));
}

void ExtensionContractRegistryTests::validationDetectsDanglingConnectionSource()
{
    SampleValidationProvider provider;
    QVariantMap snapshot = minimalValidSnapshot();
    QVariantMap extraConn;
    extraConn[QStringLiteral("id")] = QStringLiteral("bad");
    extraConn[QStringLiteral("sourceId")] = QStringLiteral("nonexistent");
    extraConn[QStringLiteral("targetId")] = QStringLiteral("n1");
    QVariantList conns = snapshot[QStringLiteral("connections")].toList();
    conns.append(extraConn);
    snapshot[QStringLiteral("connections")] = conns;
    QVERIFY(hasIssueWithCode(provider.validateGraph(snapshot), QStringLiteral("W003")));
}

void ExtensionContractRegistryTests::validationDetectsDanglingConnectionTarget()
{
    SampleValidationProvider provider;
    QVariantMap snapshot = minimalValidSnapshot();
    QVariantMap extraConn;
    extraConn[QStringLiteral("id")] = QStringLiteral("bad2");
    extraConn[QStringLiteral("sourceId")] = QStringLiteral("n1");
    extraConn[QStringLiteral("targetId")] = QStringLiteral("ghost");
    QVariantList conns = snapshot[QStringLiteral("connections")].toList();
    conns.append(extraConn);
    snapshot[QStringLiteral("connections")] = conns;
    QVERIFY(hasIssueWithCode(provider.validateGraph(snapshot), QStringLiteral("W003")));
}

void ExtensionContractRegistryTests::validationWarnsAboutIsolatedComponent()
{
    SampleValidationProvider provider;
    QVariantMap startComponent;
    startComponent[QStringLiteral("id")] = QStringLiteral("n1");
    startComponent[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap stopComponent;
    stopComponent[QStringLiteral("id")] = QStringLiteral("n3");
    stopComponent[QStringLiteral("type")] = QStringLiteral("stop");
    QVariantMap isolatedComponent;
    isolatedComponent[QStringLiteral("id")] = QStringLiteral("iso");
    isolatedComponent[QStringLiteral("type")] = QStringLiteral("process");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{ startComponent, stopComponent, isolatedComponent };
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(provider.validateGraph(snapshot), QStringLiteral("W004")));
}

void ExtensionContractRegistryTests::actionProviderExposesSetTaskPriorityId()
{
    SampleActionProvider provider;
    QVERIFY(provider.actionIds().contains(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority)));
}

void ExtensionContractRegistryTests::actionDescriptorContainsRequiredKeys()
{
    SampleActionProvider provider;
    const QVariantMap descriptor = provider.actionDescriptor(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority));
    QVERIFY(descriptor.contains(QStringLiteral("id")));
    QVERIFY(descriptor.contains(QStringLiteral("title")));
    QVERIFY(descriptor.contains(QStringLiteral("category")));
    QVERIFY(descriptor.contains(QStringLiteral("icon")));
    QVERIFY(descriptor.contains(QStringLiteral("enabled")));
}

void ExtensionContractRegistryTests::actionInvokeProducesCorrectCommandRequest()
{
    SampleActionProvider provider;
    QVariantMap context;
    context[QStringLiteral("componentId")] = QStringLiteral("process-42");
    context[QStringLiteral("newPriority")] = QStringLiteral("high");
    QVariantMap commandRequest;
    QString error;
    QVERIFY2(provider.invokeAction(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority),
        context, &commandRequest, &error), qPrintable(error));
    QCOMPARE(commandRequest.value(QStringLiteral("commandType")).toString(),
             QStringLiteral("setComponentProperty"));
    QCOMPARE(commandRequest.value(QStringLiteral("componentId")).toString(),
             QStringLiteral("process-42"));
    QCOMPARE(commandRequest.value(QStringLiteral("propertyName")).toString(),
             QStringLiteral("priority"));
    QCOMPARE(commandRequest.value(QStringLiteral("newValue")).toString(),
             QStringLiteral("high"));
}

void ExtensionContractRegistryTests::actionInvokeFailsWithMissingComponentId()
{
    SampleActionProvider provider;
    QVariantMap context;
    context[QStringLiteral("newPriority")] = QStringLiteral("low");
    QString error;
    QVERIFY(!provider.invokeAction(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority),
        context, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::actionInvokeFailsWithInvalidPriority()
{
    SampleActionProvider provider;
    QVariantMap context;
    context[QStringLiteral("componentId")] = QStringLiteral("process-1");
    context[QStringLiteral("newPriority")] = QStringLiteral("critical");
    QString error;
    QVERIFY(!provider.invokeAction(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority),
        context, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::actionInvokeFailsWithUnknownActionId()
{
    SampleActionProvider provider;
    QString error;
    QVERIFY(!provider.invokeAction(QStringLiteral("nonexistent"), {}, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::componentTypeProvidersReturnedInRegistrationOrder()
{
    StubComponentTypeProviderById pA(QStringLiteral("provider.a"));
    StubComponentTypeProviderById pB(QStringLiteral("provider.b"));
    StubComponentTypeProviderById pC(QStringLiteral("provider.c"));

    ExtensionContractRegistry reg(registryV1().coreApiVersion());
    QVERIFY(reg.registerComponentTypeProvider(&pA));
    QVERIFY(reg.registerComponentTypeProvider(&pB));
    QVERIFY(reg.registerComponentTypeProvider(&pC));

    const QList<const IComponentTypeProvider *> providers = reg.componentTypeProviders();
    QCOMPARE(providers.size(), 3);
    QCOMPARE(providers.at(0)->providerId(), QStringLiteral("provider.a"));
    QCOMPARE(providers.at(1)->providerId(), QStringLiteral("provider.b"));
    QCOMPARE(providers.at(2)->providerId(), QStringLiteral("provider.c"));
}

void ExtensionContractRegistryTests::connectionPolicyProvidersReturnedInRegistrationOrder()
{
    StubConnectionPolicyProviderById pX(QStringLiteral("policy.x"));
    StubConnectionPolicyProviderById pY(QStringLiteral("policy.y"));
    StubConnectionPolicyProviderById pZ(QStringLiteral("policy.z"));

    ExtensionContractRegistry reg(registryV1().coreApiVersion());
    QVERIFY(reg.registerConnectionPolicyProvider(&pX));
    QVERIFY(reg.registerConnectionPolicyProvider(&pY));
    QVERIFY(reg.registerConnectionPolicyProvider(&pZ));

    const QList<const IConnectionPolicyProvider *> providers = reg.connectionPolicyProviders();
    QCOMPARE(providers.size(), 3);
    QCOMPARE(providers.at(0)->providerId(), QStringLiteral("policy.x"));
    QCOMPARE(providers.at(1)->providerId(), QStringLiteral("policy.y"));
    QCOMPARE(providers.at(2)->providerId(), QStringLiteral("policy.z"));
}

QTEST_MAIN(ExtensionContractRegistryTests)
#include "tst_ExtensionContractRegistry.moc"
