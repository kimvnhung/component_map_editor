#include <QtTest/QtTest>

#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/ExtensionManifest.h"
#include "extensions/sample_pack/SampleExtensionPack.h"
#include "extensions/sample_pack/SampleNodeTypeProvider.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Build a minimal valid ExtensionManifest.
ExtensionManifest validManifest(const QString &id = QStringLiteral("test.pack"))
{
    ExtensionManifest m;
    m.extensionId      = id;
    m.displayName      = QStringLiteral("Test Pack");
    m.extensionVersion = QStringLiteral("1.0.0");
    m.minCoreApi       = { 1, 0, 0 };
    m.maxCoreApi       = { 1, 99, 99 };
    return m;
}

// Registry wired with core API 1.0.0, matching the sample pack's range.
ExtensionContractRegistry registryV1()
{
    return ExtensionContractRegistry({ 1, 0, 0 });
}

// Build a minimal valid graphSnapshot with required start + end nodes.
QVariantMap minimalValidSnapshot()
{
    QVariantMap startNode;
    startNode[QStringLiteral("id")]   = QStringLiteral("n1");
    startNode[QStringLiteral("type")] = QStringLiteral("start");

    QVariantMap taskNode;
    taskNode[QStringLiteral("id")]   = QStringLiteral("n2");
    taskNode[QStringLiteral("type")] = QStringLiteral("task");

    QVariantMap endNode;
    endNode[QStringLiteral("id")]   = QStringLiteral("n3");
    endNode[QStringLiteral("type")] = QStringLiteral("end");

    QVariantMap conn1;
    conn1[QStringLiteral("id")]       = QStringLiteral("e1");
    conn1[QStringLiteral("sourceId")] = QStringLiteral("n1");
    conn1[QStringLiteral("targetId")] = QStringLiteral("n2");

    QVariantMap conn2;
    conn2[QStringLiteral("id")]       = QStringLiteral("e2");
    conn2[QStringLiteral("sourceId")] = QStringLiteral("n2");
    conn2[QStringLiteral("targetId")] = QStringLiteral("n3");

    QVariantMap snapshot;
    snapshot[QStringLiteral("components")]  = QVariantList{ startNode, taskNode, endNode };
    snapshot[QStringLiteral("connections")] = QVariantList{ conn1, conn2 };
    return snapshot;
}

bool hasIssueWithCode(const QVariantList &issues, const QString &code)
{
    for (const QVariant &v : issues) {
        if (v.toMap().value(QStringLiteral("code")).toString() == code)
            return true;
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class ExtensionContractRegistryTests : public QObject
{
    Q_OBJECT

private slots:
    // --- Manifest and compatibility ---
    void manifestValidationRejectsEmptyId();
    void manifestValidationRejectsEmptyDisplayName();
    void manifestValidationRejectsEmptyVersion();
    void manifestValidationRejectsInvalidApiRange();
    void compatibilityRejectsCoreTooOld();
    void compatibilityRejectsCoreMajorTooNew();
    void compatibilityAcceptsCoreBelowMaxMinor();
    void duplicateManifestRejected();

    // --- End-to-end sample pack registration ---
    void samplePackRegistersSuccessfully();
    void samplePackManifestIsPresent();
    void samplePackSecondRegisterFails();

    // --- INodeTypeProvider ---
    void nodeTypeProviderReturnsFourTypes();
    void nodeTypeDescriptorContainsRequiredKeys();
    void nodeTypeDefaultsForTask();
    void nodeTypeDefaultsForDecisionHasCondition();
    void nodeTypeDefaultsForStartIsEmpty();

    // --- IConnectionPolicyProvider ---
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

    // --- IPropertySchemaProvider ---
    void propertySchemaTargetsCoverAllNodeTypes();
    void propertySchemaForTaskHasThreeEntries();
    void propertySchemaEntriesContainRequiredKeys();
    void propertySchemaForDecisionHasConditionEntry();
    void propertySchemaForFlowConnectionHasLabelEntry();
    void propertySchemaForUnknownTargetIsEmpty();

    // --- IValidationProvider ---
    void validationAcceptsMinimalValidGraph();
    void validationDetectsMissingStartNode();
    void validationDetectsMultipleStartNodes();
    void validationDetectsMissingEndNode();
    void validationDetectsDanglingConnectionSource();
    void validationDetectsDanglingConnectionTarget();
    void validationWarnsAboutIsolatedNode();

    // --- IActionProvider ---
    void actionProviderExposesSetTaskPriorityId();
    void actionDescriptorContainsRequiredKeys();
    void actionInvokeProducesCorrectCommandRequest();
    void actionInvokeFailsWithMissingComponentId();
    void actionInvokeFailsWithInvalidPriority();
    void actionInvokeFailsWithUnknownActionId();
};

// ===========================================================================
// Manifest and compatibility
// ===========================================================================

void ExtensionContractRegistryTests::manifestValidationRejectsEmptyId()
{
    ExtensionManifest m = validManifest();
    m.extensionId = QString();
    QString error;
    QVERIFY(!m.isValid(&error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::manifestValidationRejectsEmptyDisplayName()
{
    ExtensionManifest m = validManifest();
    m.displayName = QString();
    QVERIFY(!m.isValid());
}

void ExtensionContractRegistryTests::manifestValidationRejectsEmptyVersion()
{
    ExtensionManifest m = validManifest();
    m.extensionVersion = QString();
    QVERIFY(!m.isValid());
}

void ExtensionContractRegistryTests::manifestValidationRejectsInvalidApiRange()
{
    ExtensionManifest m = validManifest();
    m.minCoreApi = { 2, 0, 0 };
    m.maxCoreApi = { 1, 0, 0 }; // max < min
    QVERIFY(!m.isValid());
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
    ExtensionContractRegistry registry({ 2, 0, 0 }); // major 2 > max major 1
    QString error;
    QVERIFY(!registry.registerManifest(validManifest(), &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::compatibilityAcceptsCoreBelowMaxMinor()
{
    // Core 1.5.0 is within [1.0.0, 1.99.99].
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

// ===========================================================================
// End-to-end sample pack
// ===========================================================================

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
    const ExtensionManifest m = registry.manifest(QString::fromLatin1(SampleExtensionPack::ExtensionId));
    QCOMPARE(m.extensionId, QString::fromLatin1(SampleExtensionPack::ExtensionId));
    QCOMPARE(m.displayName, QString::fromLatin1(SampleExtensionPack::DisplayName));
}

void ExtensionContractRegistryTests::samplePackSecondRegisterFails()
{
    auto registry = registryV1();
    SampleExtensionPack pack;
    QVERIFY(pack.registerAll(registry));
    // A second call must fail because the manifest is already registered.
    QString error;
    QVERIFY(!pack.registerAll(registry, &error));
    QVERIFY(!error.isEmpty());
}

// ===========================================================================
// INodeTypeProvider
// ===========================================================================

void ExtensionContractRegistryTests::nodeTypeProviderReturnsFourTypes()
{
    SampleNodeTypeProvider p;
    QCOMPARE(p.nodeTypeIds().size(), 4);
    QVERIFY(p.nodeTypeIds().contains(QString::fromLatin1(SampleNodeTypeProvider::TypeStart)));
    QVERIFY(p.nodeTypeIds().contains(QString::fromLatin1(SampleNodeTypeProvider::TypeTask)));
    QVERIFY(p.nodeTypeIds().contains(QString::fromLatin1(SampleNodeTypeProvider::TypeDecision)));
    QVERIFY(p.nodeTypeIds().contains(QString::fromLatin1(SampleNodeTypeProvider::TypeEnd)));
}

void ExtensionContractRegistryTests::nodeTypeDescriptorContainsRequiredKeys()
{
    SampleNodeTypeProvider p;
    for (const QString &typeId : p.nodeTypeIds()) {
        const QVariantMap desc = p.nodeTypeDescriptor(typeId);
        QVERIFY2(!desc.isEmpty(), qPrintable(typeId));
        QVERIFY2(desc.contains(QStringLiteral("id")),            qPrintable(typeId));
        QVERIFY2(desc.contains(QStringLiteral("title")),         qPrintable(typeId));
        QVERIFY2(desc.contains(QStringLiteral("defaultWidth")),  qPrintable(typeId));
        QVERIFY2(desc.contains(QStringLiteral("defaultHeight")), qPrintable(typeId));
        QVERIFY2(desc.contains(QStringLiteral("defaultColor")),  qPrintable(typeId));
        QVERIFY2(desc.contains(QStringLiteral("allowIncoming")), qPrintable(typeId));
        QVERIFY2(desc.contains(QStringLiteral("allowOutgoing")), qPrintable(typeId));
    }
}

void ExtensionContractRegistryTests::nodeTypeDefaultsForTask()
{
    SampleNodeTypeProvider p;
    const QVariantMap defaults = p.defaultNodeProperties(
        QString::fromLatin1(SampleNodeTypeProvider::TypeTask));
    QVERIFY(defaults.contains(QStringLiteral("priority")));
    QCOMPARE(defaults.value(QStringLiteral("priority")).toString(), QStringLiteral("normal"));
    QVERIFY(defaults.contains(QStringLiteral("description")));
}

void ExtensionContractRegistryTests::nodeTypeDefaultsForDecisionHasCondition()
{
    SampleNodeTypeProvider p;
    const QVariantMap defaults = p.defaultNodeProperties(
        QString::fromLatin1(SampleNodeTypeProvider::TypeDecision));
    QVERIFY(defaults.contains(QStringLiteral("condition")));
}

void ExtensionContractRegistryTests::nodeTypeDefaultsForStartIsEmpty()
{
    SampleNodeTypeProvider p;
    QVERIFY(p.defaultNodeProperties(QString::fromLatin1(SampleNodeTypeProvider::TypeStart)).isEmpty());
    QVERIFY(p.defaultNodeProperties(QString::fromLatin1(SampleNodeTypeProvider::TypeEnd)).isEmpty());
}

// ===========================================================================
// IConnectionPolicyProvider
// ===========================================================================

void ExtensionContractRegistryTests::connectionPolicyAllowsStartToTask()
{
    SampleConnectionPolicyProvider p;
    QVERIFY(p.canConnect(QStringLiteral("start"), QStringLiteral("task"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsTaskToTask()
{
    SampleConnectionPolicyProvider p;
    QVERIFY(p.canConnect(QStringLiteral("task"), QStringLiteral("task"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsTaskToDecision()
{
    SampleConnectionPolicyProvider p;
    QVERIFY(p.canConnect(QStringLiteral("task"), QStringLiteral("decision"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsDecisionToTask()
{
    SampleConnectionPolicyProvider p;
    QVERIFY(p.canConnect(QStringLiteral("decision"), QStringLiteral("task"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyAllowsDecisionToEnd()
{
    SampleConnectionPolicyProvider p;
    QVERIFY(p.canConnect(QStringLiteral("decision"), QStringLiteral("end"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartAsTarget()
{
    SampleConnectionPolicyProvider p;
    QString reason;
    QVERIFY(!p.canConnect(QStringLiteral("task"), QStringLiteral("start"), {}, &reason));
    QVERIFY(!reason.isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyDeniesEndAsSource()
{
    SampleConnectionPolicyProvider p;
    QString reason;
    QVERIFY(!p.canConnect(QStringLiteral("end"), QStringLiteral("task"), {}, &reason));
    QVERIFY(!reason.isEmpty());
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartToDecision()
{
    SampleConnectionPolicyProvider p;
    QVERIFY(!p.canConnect(QStringLiteral("start"), QStringLiteral("decision"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyDeniesStartToEnd()
{
    SampleConnectionPolicyProvider p;
    QVERIFY(!p.canConnect(QStringLiteral("start"), QStringLiteral("end"), {}, nullptr));
}

void ExtensionContractRegistryTests::connectionPolicyNormalizeAddsFlowType()
{
    SampleConnectionPolicyProvider p;
    const QVariantMap result = p.normalizeConnectionProperties(
        QStringLiteral("task"), QStringLiteral("task"), {});
    QCOMPARE(result.value(QStringLiteral("type")).toString(), QStringLiteral("flow"));
}

void ExtensionContractRegistryTests::connectionPolicyNormalizePreservesExistingType()
{
    SampleConnectionPolicyProvider p;
    QVariantMap raw;
    raw[QStringLiteral("type")] = QStringLiteral("dependency");
    const QVariantMap result = p.normalizeConnectionProperties(
        QStringLiteral("task"), QStringLiteral("task"), raw);
    QCOMPARE(result.value(QStringLiteral("type")).toString(), QStringLiteral("dependency"));
}

// ===========================================================================
// IPropertySchemaProvider
// ===========================================================================

void ExtensionContractRegistryTests::propertySchemaTargetsCoverAllNodeTypes()
{
    SamplePropertySchemaProvider p;
    const QStringList targets = p.schemaTargets();
    QVERIFY(targets.contains(QStringLiteral("node/start")));
    QVERIFY(targets.contains(QStringLiteral("node/task")));
    QVERIFY(targets.contains(QStringLiteral("node/decision")));
    QVERIFY(targets.contains(QStringLiteral("node/end")));
    QVERIFY(targets.contains(QStringLiteral("connection/flow")));
}

void ExtensionContractRegistryTests::propertySchemaForTaskHasThreeEntries()
{
    SamplePropertySchemaProvider p;
    QCOMPARE(p.propertySchema(QStringLiteral("node/task")).size(), 3);
}

void ExtensionContractRegistryTests::propertySchemaEntriesContainRequiredKeys()
{
    SamplePropertySchemaProvider p;
    for (const QString &target : p.schemaTargets()) {
        const QVariantList schema = p.propertySchema(target);
        for (const QVariant &v : schema) {
            const QVariantMap entry = v.toMap();
            QVERIFY2(entry.contains(QStringLiteral("key")),          qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("type")),         qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("title")),        qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("required")),     qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("defaultValue")), qPrintable(target));
            QVERIFY2(entry.contains(QStringLiteral("editor")),       qPrintable(target));
        }
    }
}

void ExtensionContractRegistryTests::propertySchemaForDecisionHasConditionEntry()
{
    SamplePropertySchemaProvider p;
    const QVariantList schema = p.propertySchema(QStringLiteral("node/decision"));
    bool found = false;
    for (const QVariant &v : schema) {
        if (v.toMap().value(QStringLiteral("key")).toString() == QStringLiteral("condition"))
            found = true;
    }
    QVERIFY(found);
}

void ExtensionContractRegistryTests::propertySchemaForFlowConnectionHasLabelEntry()
{
    SamplePropertySchemaProvider p;
    const QVariantList schema = p.propertySchema(QStringLiteral("connection/flow"));
    QVERIFY(!schema.isEmpty());
    QCOMPARE(schema.first().toMap().value(QStringLiteral("key")).toString(),
             QStringLiteral("label"));
}

void ExtensionContractRegistryTests::propertySchemaForUnknownTargetIsEmpty()
{
    SamplePropertySchemaProvider p;
    QVERIFY(p.propertySchema(QStringLiteral("node/unknown")).isEmpty());
}

// ===========================================================================
// IValidationProvider
// ===========================================================================

void ExtensionContractRegistryTests::validationAcceptsMinimalValidGraph()
{
    SampleValidationProvider p;
    const QVariantList issues = p.validateGraph(minimalValidSnapshot());
    // No W001 or W002 errors in a valid graph
    QVERIFY(!hasIssueWithCode(issues, QStringLiteral("W001")));
    QVERIFY(!hasIssueWithCode(issues, QStringLiteral("W002")));
    QVERIFY(!hasIssueWithCode(issues, QStringLiteral("W003")));
}

void ExtensionContractRegistryTests::validationDetectsMissingStartNode()
{
    SampleValidationProvider p;
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")]  = QVariantList{};
    snapshot[QStringLiteral("connections")] = QVariantList{};
    const QVariantList issues = p.validateGraph(snapshot);
    QVERIFY(hasIssueWithCode(issues, QStringLiteral("W001")));
}

void ExtensionContractRegistryTests::validationDetectsMultipleStartNodes()
{
    SampleValidationProvider p;
    QVariantMap s1;
    s1[QStringLiteral("id")]   = QStringLiteral("s1");
    s1[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap s2;
    s2[QStringLiteral("id")]   = QStringLiteral("s2");
    s2[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap e1;
    e1[QStringLiteral("id")]   = QStringLiteral("e1");
    e1[QStringLiteral("type")] = QStringLiteral("end");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")]  = QVariantList{ s1, s2, e1 };
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(p.validateGraph(snapshot), QStringLiteral("W001")));
}

void ExtensionContractRegistryTests::validationDetectsMissingEndNode()
{
    SampleValidationProvider p;
    QVariantMap startNode;
    startNode[QStringLiteral("id")]   = QStringLiteral("n1");
    startNode[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")]  = QVariantList{ startNode };
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(p.validateGraph(snapshot), QStringLiteral("W002")));
}

void ExtensionContractRegistryTests::validationDetectsDanglingConnectionSource()
{
    SampleValidationProvider p;
    QVariantMap snapshot = minimalValidSnapshot();
    QVariantMap extraConn;
    extraConn[QStringLiteral("id")]       = QStringLiteral("bad");
    extraConn[QStringLiteral("sourceId")] = QStringLiteral("nonexistent");
    extraConn[QStringLiteral("targetId")] = QStringLiteral("n1");
    QVariantList conns = snapshot[QStringLiteral("connections")].toList();
    conns.append(extraConn);
    snapshot[QStringLiteral("connections")] = conns;
    QVERIFY(hasIssueWithCode(p.validateGraph(snapshot), QStringLiteral("W003")));
}

void ExtensionContractRegistryTests::validationDetectsDanglingConnectionTarget()
{
    SampleValidationProvider p;
    QVariantMap snapshot = minimalValidSnapshot();
    QVariantMap extraConn;
    extraConn[QStringLiteral("id")]       = QStringLiteral("bad2");
    extraConn[QStringLiteral("sourceId")] = QStringLiteral("n1");
    extraConn[QStringLiteral("targetId")] = QStringLiteral("ghost");
    QVariantList conns = snapshot[QStringLiteral("connections")].toList();
    conns.append(extraConn);
    snapshot[QStringLiteral("connections")] = conns;
    QVERIFY(hasIssueWithCode(p.validateGraph(snapshot), QStringLiteral("W003")));
}

void ExtensionContractRegistryTests::validationWarnsAboutIsolatedNode()
{
    SampleValidationProvider p;
    // Graph with a start, end, and a task with no connections.
    QVariantMap startNode;
    startNode[QStringLiteral("id")]   = QStringLiteral("n1");
    startNode[QStringLiteral("type")] = QStringLiteral("start");
    QVariantMap endNode;
    endNode[QStringLiteral("id")]   = QStringLiteral("n3");
    endNode[QStringLiteral("type")] = QStringLiteral("end");
    QVariantMap isolated;
    isolated[QStringLiteral("id")]   = QStringLiteral("iso");
    isolated[QStringLiteral("type")] = QStringLiteral("task");
    QVariantMap snapshot;
    snapshot[QStringLiteral("components")]  = QVariantList{ startNode, endNode, isolated };
    snapshot[QStringLiteral("connections")] = QVariantList{};
    QVERIFY(hasIssueWithCode(p.validateGraph(snapshot), QStringLiteral("W004")));
}

// ===========================================================================
// IActionProvider
// ===========================================================================

void ExtensionContractRegistryTests::actionProviderExposesSetTaskPriorityId()
{
    SampleActionProvider p;
    QVERIFY(p.actionIds().contains(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority)));
}

void ExtensionContractRegistryTests::actionDescriptorContainsRequiredKeys()
{
    SampleActionProvider p;
    const QVariantMap desc = p.actionDescriptor(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority));
    QVERIFY(desc.contains(QStringLiteral("id")));
    QVERIFY(desc.contains(QStringLiteral("title")));
    QVERIFY(desc.contains(QStringLiteral("category")));
    QVERIFY(desc.contains(QStringLiteral("icon")));
    QVERIFY(desc.contains(QStringLiteral("enabled")));
}

void ExtensionContractRegistryTests::actionInvokeProducesCorrectCommandRequest()
{
    SampleActionProvider p;
    QVariantMap context;
    context[QStringLiteral("componentId")] = QStringLiteral("task-42");
    context[QStringLiteral("newPriority")] = QStringLiteral("high");
    QVariantMap commandRequest;
    QString error;
    QVERIFY2(p.invokeAction(
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
    SampleActionProvider p;
    QVariantMap context;
    context[QStringLiteral("newPriority")] = QStringLiteral("low");
    QString error;
    QVERIFY(!p.invokeAction(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority),
        context, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::actionInvokeFailsWithInvalidPriority()
{
    SampleActionProvider p;
    QVariantMap context;
    context[QStringLiteral("componentId")] = QStringLiteral("task-1");
    context[QStringLiteral("newPriority")] = QStringLiteral("critical"); // not in allowed set
    QString error;
    QVERIFY(!p.invokeAction(
        QString::fromLatin1(SampleActionProvider::ActionSetTaskPriority),
        context, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void ExtensionContractRegistryTests::actionInvokeFailsWithUnknownActionId()
{
    SampleActionProvider p;
    QString error;
    QVERIFY(!p.invokeAction(QStringLiteral("nonexistent"), {}, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(ExtensionContractRegistryTests)
#include "tst_ExtensionContractRegistry.moc"
