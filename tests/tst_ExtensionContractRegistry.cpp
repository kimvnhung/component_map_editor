#include <QtTest/QtTest>

#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/ExtensionManifest.h"
#include "extensions/sample_pack/SampleComponentTypeProvider.h"
#include "extensions/sample_pack/SampleExtensionPack.h"

namespace {

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

ExtensionContractRegistry registryV1()
{
    return ExtensionContractRegistry({ 1, 0, 0 });
}

QVariantMap minimalValidSnapshot()
{
    QVariantMap startComponent;
    startComponent[QStringLiteral("id")] = QStringLiteral("n1");
    startComponent[QStringLiteral("type")] = QStringLiteral("start");

    QVariantMap taskComponent;
    taskComponent[QStringLiteral("id")] = QStringLiteral("n2");
    taskComponent[QStringLiteral("type")] = QStringLiteral("task");

    QVariantMap endComponent;
    endComponent[QStringLiteral("id")] = QStringLiteral("n3");
    endComponent[QStringLiteral("type")] = QStringLiteral("end");

    QVariantMap conn1;
    conn1[QStringLiteral("id")] = QStringLiteral("e1");
    conn1[QStringLiteral("sourceId")] = QStringLiteral("n1");
    conn1[QStringLiteral("targetId")] = QStringLiteral("n2");

    QVariantMap conn2;
    conn2[QStringLiteral("id")] = QStringLiteral("e2");
    conn2[QStringLiteral("sourceId")] = QStringLiteral("n2");
    conn2[QStringLiteral("targetId")] = QStringLiteral("n3");

    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{ startComponent, taskComponent, endComponent };
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

    void componentTypeProviderReturnsFourTypes();
    void componentTypeDescriptorContainsRequiredKeys();
    void componentTypeDefaultsForTask();
    void componentTypeDefaultsForDecisionHasCondition();
    void componentTypeDefaultsForStartIsEmpty();

    void connectionPolicyAllowsStartToTask();
    void connectionPolicyAllowsTaskToTask();
    void connectionPolicyAllowsTaskToDecision();
    void connectionPolicyAllowsDecisionToTask();
    void connectionPolicyAllowsDecisionToEnd();
    void connectionPolicyDeniesStartAsTarget();
    void connectionPolicyDeniesEndAsSource();
    void connectionPolicyDeniesStartToDecision();
    void connectionPolicyDeniesStartToEnd();
    void connectionPolicyNormalizeAddsFlowType();
    void connectionPolicyNormalizePreservesExistingType();

    void propertySchemaTargetsCoverAllComponentTypes();
    void propertySchemaForTaskHasThreeEntries();
    void propertySchemaEntriesContainRequiredKeys();
    void propertySchemaForDecisionHasConditionEntry();
    void propertySchemaForFlowConnectionHasLabelEntry();
    void propertySchemaForUnknownTargetIsEmpty();

    void validationAcceptsMinimalValidGraph();
    void validationDetectsMissingStartComponent();
    void validationDetectsMultipleStartComponents();
    void validationDetectsMissingEndComponent();
    void validationDetectsDanglingConnectionSource();
    void validationDetectsDanglingConnectionTarget();
    void validationWarnsAboutIsolatedComponent();

    void actionProviderExposesSetTaskPriorityId();
    void actionDescriptorContainsRequiredKeys();
    void actionInvokeProducesCorrectCommandRequest();
    void actionInvokeFailsWithMissingComponentId();
    void actionInvokeFailsWithInvalidPriority();
    void actionInvokeFailsWithUnknownActionId();
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

void ExtensionContractRegistryTests::componentTypeProviderReturnsFourTypes()
{
    SampleComponentTypeProvider provider;
    QCOMPARE(provider.componentTypeIds().size(), 4);
    QVERIFY(provider.componentTypeIds().contains(QString::fromLatin1(SampleComponentTypeProvider::TypeStart)));
    QVERIFY(provider.componentTypeIds().contains(QString::fromLatin1(SampleComponentTypeProvider::TypeTask)));
    QVERIFY(provider.componentTypeIds().contains(QString::fromLatin1(SampleComponentTypeProvider::TypeDecision)));
    QVERIFY(provider.componentTypeIds().contains(QString::fromLatin1(SampleComponentTypeProvider::TypeEnd)));
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

void ExtensionContractRegistryTests::componentTypeDefaultsForTask()
{
    SampleComponentTypeProvider provider;
    const QVariantMap defaults = provider.defaultComponentProperties(
        QString::fromLatin1(SampleComponentTypeProvider::TypeTask));
    QVERIFY(defaults.contains(QStringLiteral("priority")));
    QCOMPARE(defaults.value(QStringLiteral("priority")).toString(), QStringLiteral("normal"));
    QVERIFY(defaults.contains(QStringLiteral("description")));
}

void ExtensionContractRegistryTests::componentTypeDefaultsForDecisionHasCondition()
{
    SampleComponentTypeProvider provider;
    const QVariantMap defaults = provider.defaultComponentProperties(
        QString::fromLatin1(SampleComponentTypeProvider::TypeDecision));
    QVERIFY(defaults.contains(QStringLiteral("condition")));
}

void ExtensionContractRegistryTests::componentTypeDefaultsForStartIsEmpty()
{
    SampleComponentTypeProvider provider;
    QVERIFY(provider.defaultComponentProperties(QString::fromLatin1(SampleComponentTypeProvider::TypeStart)).isEmpty());
    QVERIFY(provider.defaultComponentProperties(QString::fromLatin1(SampleComponentTypeProvider::TypeEnd)).isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyAllowsStartToTask()
{
    SampleConnectionPolicyProvider provider;
    QVERIFY(provider.canConnect(QStringLiteral("start"), QStringLiteral("task"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsTaskToTask()
{
    SampleConnectionPolicyProvider provider;
    QVERIFY(provider.canConnect(QStringLiteral("task"), QStringLiteral("task"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsTaskToDecision()
{
    SampleConnectionPolicyProvider provider;
    QVERIFY(provider.canConnect(QStringLiteral("task"), QStringLiteral("decision"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsDecisionToTask()
{
    SampleConnectionPolicyProvider provider;
    QVERIFY(provider.canConnect(QStringLiteral("decision"), QStringLiteral("task"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsDecisionToEnd()
{
    SampleConnectionPolicyProvider provider;
    QVERIFY(provider.canConnect(QStringLiteral("decision"), QStringLiteral("end"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartAsTarget()
{
    SampleConnectionPolicyProvider provider;
    QString reason;
    QVERIFY(!provider.canConnect(QStringLiteral("task"), QStringLiteral("start"), {}, &reason));
    QVERIFY(!reason.isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyDeniesEndAsSource()
{
    SampleConnectionPolicyProvider provider;
    QString reason;
    QVERIFY(!provider.canConnect(QStringLiteral("end"), QStringLiteral("task"), {}, &reason));
    QVERIFY(!reason.isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartToDecision()
{
    SampleConnectionPolicyProvider provider;
    QVERIFY(!provider.canConnect(QStringLiteral("start"), QStringLiteral("decision"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartToEnd()
{
    SampleConnectionPolicyProvider provider;
    QVERIFY(!provider.canConnect(QStringLiteral("start"), QStringLiteral("end"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyNormalizeAddsFlowType()
{
    SampleConnectionPolicyProvider provider;
    const QVariantMap result = provider.normalizeConnectionProperties(
        QStringLiteral("task"), QStringLiteral("task"), {});
    QCOMPARE(result.value(QStringLiteral("type")).toString(), QStringLiteral("flow"));
}

void ExtensionContractRegistryTests::connectionPolicyNormalizePreservesExistingType()
{
    SampleConnectionPolicyProvider provider;
    QVariantMap raw;
    raw[QStringLiteral("type")] = QStringLiteral("dependency");
    const QVariantMap result = provider.normalizeConnectionProperties(
        QStringLiteral("task"), QStringLiteral("task"), raw);
    QCOMPARE(result.value(QStringLiteral("type")).toString(), QStringLiteral("dependency"));
}

void ExtensionContractRegistryTests::propertySchemaTargetsCoverAllComponentTypes()
{
    SamplePropertySchemaProvider provider;
    const QStringList targets = provider.schemaTargets();
    QVERIFY(targets.contains(QStringLiteral("component/start")));
    QVERIFY(targets.contains(QStringLiteral("component/task")));
    QVERIFY(targets.contains(QStringLiteral("component/decision")));
    QVERIFY(targets.contains(QStringLiteral("component/end")));
    QVERIFY(targets.contains(QStringLiteral("connection/flow")));
}

void ExtensionContractRegistryTests::propertySchemaForTaskHasThreeEntries()
{
    SamplePropertySchemaProvider provider;
    QCOMPARE(provider.propertySchema(QStringLiteral("component/task")).size(), 3);
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

void ExtensionContractRegistryTests::propertySchemaForDecisionHasConditionEntry()
{
    SamplePropertySchemaProvider provider;
    const QVariantList schema = provider.propertySchema(QStringLiteral("component/decision"));
    bool found = false;
    for (const QVariant &value : schema) {
        if (value.toMap().value(QStringLiteral("key")).toString() == QStringLiteral("condition"))
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
    QVariantMap endComponent;
    endComponent[QStringLiteral("id")] = QStringLiteral("e1");
    endComponent[QStringLiteral("type")] = QStringLiteral("end");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{ c1, c2, endComponent };
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(provider.validateGraph(snapshot), QStringLiteral("W001")));
}

void ExtensionContractRegistryTests::validationDetectsMissingEndComponent()
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
    QVariantMap endComponent;
    endComponent[QStringLiteral("id")] = QStringLiteral("n3");
    endComponent[QStringLiteral("type")] = QStringLiteral("end");
    QVariantMap isolatedComponent;
    isolatedComponent[QStringLiteral("id")] = QStringLiteral("iso");
    isolatedComponent[QStringLiteral("type")] = QStringLiteral("task");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")] = QVariantList{ startComponent, endComponent, isolatedComponent };
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
    context[QStringLiteral("componentId")] = QStringLiteral("task-42");
    context[QStringLiteral("newPriority")] = QStringLiteral("high");
    QVariantMap commandRequest;
    QString error;
    QVERIFY2(provider.invokeAction(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority),
        context, &commandRequest, &error), qPrintable(error));
    QCOMPARE(commandRequest.value(QStringLiteral("commandType")).toString(),
             QStringLiteral("setComponentProperty"));
    QCOMPARE(commandRequest.value(QStringLiteral("componentId")).toString(),
             QStringLiteral("task-42"));
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
    context[QStringLiteral("componentId")] = QStringLiteral("task-1");
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

QTEST_MAIN(ExtensionContractRegistryTests)
#include "tst_ExtensionContractRegistry.moc"
