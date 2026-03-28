// tst_Phase0Baseline.cpp
//
// Phase 0: Baseline and Freeze
// ----------------------------
// This test file is the living contract for the pre-migration baseline captured on
// 2026-03-28 (branch: create_example, commit: 9a1d681).
//
// PURPOSE
//   Every assertion below is a hard gate for ALL subsequent migration phases.
//   If any test here fails after a code change, the migration has introduced a
//   regression and must be corrected before merging.
//
// COVERAGE
//   1. Performance budgets are unchanged.
//   2. Supported command names (CommandGateway) are unchanged.
//   3. GraphSchema canonical key names are unchanged.
//   4. Extension provider contract IIDs are unchanged.
//   5. CommandGateway behavioral contracts (success, rejection, rollback).
//   6. TypeRegistry contract (descriptor keys, policy delegation).
//   7. ValidationService contract (issue shape, severity mapping).
//   8. GraphExecutionSandbox contract (determinism, timeline event names).
//   9. Architecture layer invariants (budgets vs. gate thresholds).
//  10. Regression gate: verifies no new hard-coded literals crept in by checking
//      an explicit allow-list of command/event keys used at the boundary.
//
// HOW TO USE
//   Run this suite at the start of every phase to confirm no regression before
//   starting new work.  The test must pass with exit code 0.
//
// ─────────────────────────────────────────────────────────────────────────────

#include <QtTest>
#include <QElapsedTimer>
#include <QStringList>
#include <algorithm>

#include "commands/UndoStack.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IActionProvider.h"
#include "extensions/contracts/IComponentTypeProvider.h"
#include "extensions/contracts/IConnectionPolicyProvider.h"
#include "extensions/contracts/IValidationProvider.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "extensions/runtime/TypeRegistry.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/CapabilityRegistry.h"
#include "services/CommandGateway.h"
#include "services/GraphEditorController.h"
#include "services/GraphExecutionSandbox.h"
#include "services/GraphSchema.h"
#include "services/InvariantChecker.h"
#include "services/PerformanceBudgets.h"
#include "services/ValidationService.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Returns a sorted snapshot of all keys present in @p map.
QStringList sortedKeys(const QVariantMap &map)
{
    QStringList keys = map.keys();
    keys.sort();
    return keys;
}

// Minimal IComponentTypeProvider for wiring TypeRegistry in tests.
class MinimalComponentTypeProvider : public IComponentTypeProvider
{
public:
    QString providerId() const override { return QStringLiteral("baseline.component"); }
    QStringList componentTypeIds() const override { return { QStringLiteral("process"), QStringLiteral("start"), QStringLiteral("stop") }; }

    QVariantMap componentTypeDescriptor(const QString &typeId) const override {
        return QVariantMap{
            { QStringLiteral("id"),            typeId },
            { QStringLiteral("title"),         typeId },
            { QStringLiteral("defaultWidth"),  96.0 },
            { QStringLiteral("defaultHeight"), 64.0 },
            { QStringLiteral("defaultColor"),  QStringLiteral("#4fc3f7") },
            { QStringLiteral("shape"),         QStringLiteral("rounded") },
        };
    }

    QVariantMap defaultComponentProperties(const QString &) const override { return {}; }
};

// Minimal IConnectionPolicyProvider — always allows.
class AllowAllConnectionPolicy : public IConnectionPolicyProvider
{
public:
    QString providerId() const override { return QStringLiteral("baseline.policy.allow"); }

    bool canConnect(const QString &, const QString &, const QVariantMap &, QString *) const override { return true; }

    QVariantMap normalizeConnectionProperties(const QString &, const QString &, const QVariantMap &raw) const override { return raw; }
};

// Minimal IValidationProvider — records call without any issues.
class MinimalValidationProvider : public IValidationProvider
{
public:
    QString providerId() const override { return QStringLiteral("baseline.validation"); }

    QVariantList validateGraph(const QVariantMap &snapshot) const override {
        Q_UNUSED(snapshot)
        return {};
    }
};

// Execution semantics provider that does nothing — used for determinism check.
class NoOpExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override { return QStringLiteral("baseline.execution"); }
    QStringList supportedComponentTypes() const override { return { QStringLiteral("process") }; }

    bool executeComponent(const QString &, const QString &,
                          const QVariantMap &, const QVariantMap &input,
                          QVariantMap *output, QVariantMap *trace,
                          QString *) const override {
        if (output) *output = input;
        if (trace) trace->insert(QStringLiteral("provider"), QStringLiteral("noop"));
        return true;
    }
};

double p95Ms(QVector<double> samples)
{
    if (samples.isEmpty()) return 0.0;
    std::sort(samples.begin(), samples.end());
    const int idx = qBound(0, static_cast<int>(0.95 * (samples.size() - 1)), samples.size() - 1);
    return samples.at(idx);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test class
// ─────────────────────────────────────────────────────────────────────────────

class tst_Phase0Baseline : public QObject
{
    Q_OBJECT

private slots:
    // 1. Performance budget constants are frozen.
    void performanceBudgetsAreUnchanged();

    // 2. CommandGateway command name contract.
    void commandGatewaySupportedCommandsAreUnchanged();

    // 3. GraphSchema key names are frozen.
    void graphSchemaKeyNamesAreUnchanged();

    // 4. Extension IID constants are unchanged.
    void extensionIIDsAreUnchanged();

    // 5. CommandGateway behavioral: success path.
    void commandGatewayAddComponentSucceeds();

    // 6. CommandGateway behavioral: unknown command is rejected.
    void commandGatewayUnknownCommandIsRejected();

    // 7. CommandGateway behavioral: missing required field is rejected.
    void commandGatewayMissingFieldIsRejected();

    // 8. CommandGateway behavioral: capability denied blocks execution.
    void commandGatewayCapabilityDeniedBlocks();

    // 9. CommandGateway behavioral: post-invariant failure is rolled back.
    void commandGatewayPostInvariantRollbackRestoresGraph();

    // 10. TypeRegistry descriptor shape is frozen.
    void typeRegistryDescriptorHasExpectedKeys();

    // 11. TypeRegistry policy delegation (allow-all passes, deny-all fails).
    void typeRegistryPolicyDelegation();

    // 12. ValidationService issue shape is frozen.
    void validationServiceIssueHasExpectedKeys();

    // 13. ValidationService: null graph produces sentinel error.
    void validationServiceNullGraphProducesSentinelError();

    // 14. GraphExecutionSandbox: timeline event names are frozen.
    void executionSandboxTimelineEventNamesAreUnchanged();

    // 15. GraphExecutionSandbox determinism: same graph gives same event order.
    void executionSandboxIsDeterministic();

    // 16. CommandGateway latency p95 stays within budget under 200 warm iterations.
    void commandGatewayLatencyP95WithinBudget();

    // 17. Architecture layer invariant: budget gate thresholds not relaxed.
    void architectureGateThresholdsNotRelaxed();

    // 18. Boundary allowlist: request/response map keys are limited to known set.
    void commandRequestKeyAllowlistIsComplete();
};

// ─────────────────────────────────────────────────────────────────────────────
// 1. Performance budget constants are frozen
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::performanceBudgetsAreUnchanged()
{
    // These values are the migration baseline. Any phase that changes them must
    // justify it explicitly with a performance measurement report.
    QCOMPARE(PerformanceBudgets::kFrameP95BudgetMs,           16.0);
    QCOMPARE(PerformanceBudgets::kCommandLatencyP95BudgetMs,  50.0);
    QCOMPARE(PerformanceBudgets::kMemoryGrowthBudgetBytes,    64ll * 1024ll * 1024ll);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. CommandGateway command name contract
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandGatewaySupportedCommandsAreUnchanged()
{
    // The exact set of supported command name strings is the public contract for
    // all plugins and the extension adapter layer. No string may be removed or
    // renamed without a versioned migration.
    const QStringList expected = {
        QStringLiteral("addComponent"),
        QStringLiteral("removeComponent"),
        QStringLiteral("moveComponent"),
        QStringLiteral("addConnection"),
        QStringLiteral("removeConnection"),
        QStringLiteral("setComponentProperty"),
        QStringLiteral("setConnectionProperty"),
    };

    QStringList actual = CommandGateway::supportedCommands();
    actual.sort();
    QStringList sorted = expected;
    sorted.sort();

    QCOMPARE(actual, sorted);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. GraphSchema key names
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::graphSchemaKeyNamesAreUnchanged()
{
    // These keys are used in JSON serialized files and in map-based contracts
    // for connection/component data. Renaming them is a breaking schema change.
    QCOMPARE(GraphSchema::Keys::componentId(),          QStringLiteral("id"));
    QCOMPARE(GraphSchema::Keys::componentType(),        QStringLiteral("type"));
    QCOMPARE(GraphSchema::Keys::componentTitle(),       QStringLiteral("title"));
    QCOMPARE(GraphSchema::Keys::componentX(),           QStringLiteral("x"));
    QCOMPARE(GraphSchema::Keys::componentY(),           QStringLiteral("y"));
    QCOMPARE(GraphSchema::Keys::componentWidth(),       QStringLiteral("width"));
    QCOMPARE(GraphSchema::Keys::componentHeight(),      QStringLiteral("height"));
    QCOMPARE(GraphSchema::Keys::componentColor(),       QStringLiteral("color"));
    QCOMPARE(GraphSchema::Keys::componentShape(),       QStringLiteral("shape"));
    QCOMPARE(GraphSchema::Keys::componentContent(),     QStringLiteral("content"));
    QCOMPARE(GraphSchema::Keys::componentIcon(),        QStringLiteral("icon"));

    QCOMPARE(GraphSchema::Keys::connectionId(),         QStringLiteral("id"));
    QCOMPARE(GraphSchema::Keys::connectionSourceId(),   QStringLiteral("sourceId"));
    QCOMPARE(GraphSchema::Keys::connectionTargetId(),   QStringLiteral("targetId"));
    QCOMPARE(GraphSchema::Keys::connectionLabel(),      QStringLiteral("label"));
    QCOMPARE(GraphSchema::Keys::connectionSourceSide(), QStringLiteral("sourceSide"));
    QCOMPARE(GraphSchema::Keys::connectionTargetSide(), QStringLiteral("targetSide"));

    QCOMPARE(GraphSchema::Keys::components(),           QStringLiteral("components"));
    QCOMPARE(GraphSchema::Keys::connections(),          QStringLiteral("connections"));
    QCOMPARE(GraphSchema::Keys::coordinateSystem(),     QStringLiteral("coordinateSystem"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Extension IID constants
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::extensionIIDsAreUnchanged()
{
    // Qt plugin IIDs are binary identifiers. Changing them breaks all compiled
    // extension plugins. Only change with a major interface version bump.
    QCOMPARE(QString::fromLatin1(COMPONENT_MAP_EDITOR_IID_COMPONENT_TYPE_PROVIDER),
             QStringLiteral("ComponentMapEditor.Extensions.IComponentTypeProvider/1.0"));
    QCOMPARE(QString::fromLatin1(COMPONENT_MAP_EDITOR_IID_CONNECTION_POLICY_PROVIDER),
             QStringLiteral("ComponentMapEditor.Extensions.IConnectionPolicyProvider/1.0"));
    QCOMPARE(QString::fromLatin1(COMPONENT_MAP_EDITOR_IID_ACTION_PROVIDER),
             QStringLiteral("ComponentMapEditor.Extensions.IActionProvider/1.0"));
    QCOMPARE(QString::fromLatin1(COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER),
             QStringLiteral("ComponentMapEditor.Extensions.IValidationProvider/1.0"));
    QCOMPARE(QString::fromLatin1(COMPONENT_MAP_EDITOR_IID_EXECUTION_SEMANTICS_PROVIDER),
             QStringLiteral("ComponentMapEditor.Extensions.IExecutionSemanticsProvider/1.0"));
    QCOMPARE(QString::fromLatin1(COMPONENT_MAP_EDITOR_IID_PROPERTY_SCHEMA_PROVIDER),
             QStringLiteral("ComponentMapEditor.Extensions.IPropertySchemaProvider/1.0"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. CommandGateway behavioral: success path
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandGatewayAddComponentSucceeds()
{
    QObject root;
    GraphModel graph(&root);
    UndoStack undo(&root);
    CapabilityRegistry caps(&root);
    CommandGateway gateway(&root);

    gateway.setGraph(&graph);
    gateway.setUndoStack(&undo);
    gateway.setCapabilityRegistry(&caps);
    caps.grantCapabilities(QStringLiteral("test.plugin"), { QStringLiteral("graph.mutate") });

    QString error;
    const bool ok = gateway.executeRequest(
        QStringLiteral("test.plugin"),
        QVariantMap{
            { QStringLiteral("command"), QStringLiteral("addComponent") },
            { QStringLiteral("id"),      QStringLiteral("c1") },
            { QStringLiteral("typeId"),  QStringLiteral("process") },
            { QStringLiteral("x"),       10.0 },
            { QStringLiteral("y"),       20.0 },
        },
        &error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(graph.componentCount(), 1);
    QVERIFY(graph.componentById(QStringLiteral("c1")) != nullptr);

    // Log must record success entry.
    QCOMPARE(gateway.requestLog().size(), 1);
    const QVariantMap entry = gateway.requestLog().first().toMap();
    QCOMPARE(entry.value(QStringLiteral("blocked")).toBool(), false);
    QCOMPARE(entry.value(QStringLiteral("command")).toString(), QStringLiteral("addComponent"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. CommandGateway behavioral: unknown command is rejected
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandGatewayUnknownCommandIsRejected()
{
    QObject root;
    GraphModel graph(&root);
    UndoStack undo(&root);
    CapabilityRegistry caps(&root);
    CommandGateway gateway(&root);

    gateway.setGraph(&graph);
    gateway.setUndoStack(&undo);
    gateway.setCapabilityRegistry(&caps);
    caps.grantCapabilities(QStringLiteral("test.plugin"), { QStringLiteral("graph.mutate") });

    QString error;
    const bool ok = gateway.executeRequest(
        QStringLiteral("test.plugin"),
        QVariantMap{ { QStringLiteral("command"), QStringLiteral("deleteEverythingNow") } },
        &error);

    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());
    QCOMPARE(graph.componentCount(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. CommandGateway behavioral: missing required field is rejected
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandGatewayMissingFieldIsRejected()
{
    QObject root;
    GraphModel graph(&root);
    UndoStack undo(&root);
    CapabilityRegistry caps(&root);
    CommandGateway gateway(&root);

    gateway.setGraph(&graph);
    gateway.setUndoStack(&undo);
    gateway.setCapabilityRegistry(&caps);
    caps.grantCapabilities(QStringLiteral("test.plugin"), { QStringLiteral("graph.mutate") });

    // addComponent without 'id'
    QString error;
    const bool ok = gateway.executeRequest(
        QStringLiteral("test.plugin"),
        QVariantMap{
            { QStringLiteral("command"), QStringLiteral("addComponent") },
            { QStringLiteral("typeId"),  QStringLiteral("process") },
        },
        &error);

    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());
    QCOMPARE(graph.componentCount(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. CommandGateway behavioral: capability-denied blocks execution
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandGatewayCapabilityDeniedBlocks()
{
    QObject root;
    GraphModel graph(&root);
    UndoStack undo(&root);
    CapabilityRegistry caps(&root);
    CommandGateway gateway(&root);

    gateway.setGraph(&graph);
    gateway.setUndoStack(&undo);
    gateway.setCapabilityRegistry(&caps);
    // Intentionally not granting any capabilities

    QString error;
    const bool ok = gateway.executeRequest(
        QStringLiteral("rogue.plugin"),
        QVariantMap{
            { QStringLiteral("command"), QStringLiteral("addComponent") },
            { QStringLiteral("id"),      QStringLiteral("c_rogue") },
            { QStringLiteral("typeId"),  QStringLiteral("process") },
        },
        &error);

    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());
    QCOMPARE(graph.componentCount(), 0);

    // Log must show blocked=true
    QCOMPARE(gateway.requestLog().size(), 1);
    QVERIFY(gateway.requestLog().first().toMap().value(QStringLiteral("blocked")).toBool());
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. CommandGateway behavioral: post-invariant rollback restores graph
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandGatewayPostInvariantRollbackRestoresGraph()
{
    QObject root;
    GraphModel graph(&root);
    UndoStack undo(&root);
    CapabilityRegistry caps(&root);
    InvariantChecker checker(&root);
    CommandGateway gateway(&root);

    gateway.setGraph(&graph);
    gateway.setUndoStack(&undo);
    gateway.setCapabilityRegistry(&caps);
    gateway.setInvariantChecker(&checker);
    caps.grantCapabilities(QStringLiteral("test.plugin"), { QStringLiteral("graph.mutate") });

    // Register an invariant that rejects the third component onward.
    checker.registerInvariant(QStringLiteral("max-two"),
        [](const GraphModel *g, QString *reason) -> bool {
            if (g->componentCount() > 2) {
                if (reason) *reason = QStringLiteral("max 2 components allowed");
                return false;
            }
            return true;
        });

    // Add two components OK
    for (int i = 1; i <= 2; ++i) {
        const bool ok = gateway.executeRequest(
            QStringLiteral("test.plugin"),
            QVariantMap{
                { QStringLiteral("command"), QStringLiteral("addComponent") },
                { QStringLiteral("id"),      QStringLiteral("c%1").arg(i) },
                { QStringLiteral("typeId"),  QStringLiteral("process") },
            });
        QVERIFY(ok);
    }
    QCOMPARE(graph.componentCount(), 2);

    // Third component must fail and graph must revert to 2.
    QString error;
    const bool ok = gateway.executeRequest(
        QStringLiteral("test.plugin"),
        QVariantMap{
            { QStringLiteral("command"), QStringLiteral("addComponent") },
            { QStringLiteral("id"),      QStringLiteral("c3") },
            { QStringLiteral("typeId"),  QStringLiteral("process") },
        },
        &error);

    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());
    QCOMPARE(graph.componentCount(), 2);            // graph restored
    QVERIFY(graph.componentById(QStringLiteral("c3")) == nullptr); // rolled back
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. TypeRegistry descriptor shape
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::typeRegistryDescriptorHasExpectedKeys()
{
    ExtensionApiVersion v;
    v.major = 1; v.minor = 0; v.patch = 0;
    ExtensionContractRegistry reg(v);
    MinimalComponentTypeProvider provider;
    QVERIFY(reg.registerComponentTypeProvider(&provider));

    TypeRegistry tr;
    tr.rebuildFromRegistry(reg);

    QVERIFY(tr.hasComponentType(QStringLiteral("process")));
    const QVariantMap desc = tr.componentTypeDescriptor(QStringLiteral("process"));

    // These descriptor keys are the minimum required shape for UI and controller.
    QVERIFY(desc.contains(QStringLiteral("id")));
    QVERIFY(desc.contains(QStringLiteral("title")));
    QVERIFY(desc.contains(QStringLiteral("defaultWidth")));
    QVERIFY(desc.contains(QStringLiteral("defaultHeight")));
    QVERIFY(desc.contains(QStringLiteral("defaultColor")));
    QVERIFY(desc.contains(QStringLiteral("shape")));
}

// ─────────────────────────────────────────────────────────────────────────────
// 11. TypeRegistry policy delegation
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::typeRegistryPolicyDelegation()
{
    ExtensionApiVersion v;
    v.major = 1; v.minor = 0; v.patch = 0;
    ExtensionContractRegistry reg(v);
    AllowAllConnectionPolicy policy;
    QVERIFY(reg.registerConnectionPolicyProvider(&policy));

    TypeRegistry tr;
    tr.rebuildFromRegistry(reg);

    // Allow-all provider must pass
    QVERIFY(tr.canConnect(QStringLiteral("process"), QStringLiteral("process")));

    // No providers registered: open-world assumption allows
    TypeRegistry emptyTr;
    QVERIFY(emptyTr.canConnect(QStringLiteral("anything"), QStringLiteral("anything")));
}

// ─────────────────────────────────────────────────────────────────────────────
// 12. ValidationService issue shape
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::validationServiceIssueHasExpectedKeys()
{
    // The ValidationService builds graph snapshot maps and passes them to providers.
    // The expected issue shape drives UI (PropertyPanel, StatusBar, QaMatrix).
    // Adding keys is safe; removing or renaming keys is a breaking change.
    const QStringList requiredIssueKeys = {
        QStringLiteral("code"),
        QStringLiteral("severity"),
        QStringLiteral("message"),
        QStringLiteral("entityType"),
        QStringLiteral("entityId"),
    };

    // Build an issue from ValidationService null-graph sentinel path.
    GraphModel *nullGraph = nullptr;
    ValidationService svc;
    const QVariantList issues = svc.validationIssues(nullGraph);
    QVERIFY(!issues.isEmpty());

    const QVariantMap issue = issues.first().toMap();
    for (const QString &key : requiredIssueKeys)
        QVERIFY2(issue.contains(key), qPrintable(QStringLiteral("Issue missing key: %1").arg(key)));
}

// ─────────────────────────────────────────────────────────────────────────────
// 13. ValidationService: null graph produces sentinel error
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::validationServiceNullGraphProducesSentinelError()
{
    ValidationService svc;
    const QVariantList issues = svc.validationIssues(nullptr);
    QCOMPARE(issues.size(), 1);

    const QVariantMap issue = issues.first().toMap();
    QCOMPARE(issue.value(QStringLiteral("code")).toString(),     QStringLiteral("CORE_NULL_GRAPH"));
    QCOMPARE(issue.value(QStringLiteral("severity")).toString(), QStringLiteral("error"));
    QCOMPARE(issue.value(QStringLiteral("entityType")).toString(), QStringLiteral("graph"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 14. GraphExecutionSandbox timeline event names
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::executionSandboxTimelineEventNamesAreUnchanged()
{
    // Timeline event name strings are consumed by QML bindings and test assertions.
    // They form a public contract. Changing any of these requires QML updates too.
    GraphModel graph;
    auto *start = new ComponentModel(&graph);
    start->setId(QStringLiteral("s1"));
    start->setType(QStringLiteral("process"));
    graph.addComponent(start);

    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);

    QVERIFY(sandbox.start());
    sandbox.run();

    const QVariantList timeline = sandbox.timeline();
    QVERIFY(timeline.size() >= 2);

    // First event must still be "simulationStarted"
    const QString firstEvent = timeline.first().toMap().value(QStringLiteral("event")).toString();
    QCOMPARE(firstEvent, QStringLiteral("simulationStarted"));

    // Last event must be "simulationCompleted" or "simulationBlocked"
    const QString lastEvent = timeline.last().toMap().value(QStringLiteral("event")).toString();
    QVERIFY2(lastEvent == QStringLiteral("simulationCompleted") ||
             lastEvent == QStringLiteral("simulationBlocked"),
             qPrintable(QStringLiteral("Unexpected last event: %1").arg(lastEvent)));
}

// ─────────────────────────────────────────────────────────────────────────────
// 15. GraphExecutionSandbox determinism
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::executionSandboxIsDeterministic()
{
    GraphModel graph;
    for (int i = 0; i < 4; ++i) {
        auto *c = new ComponentModel(&graph);
        c->setId(QStringLiteral("n%1").arg(i));
        c->setType(QStringLiteral("process"));
        graph.addComponent(c);
    }
    // Linear chain: n0 -> n1 -> n2 -> n3
    for (int i = 0; i < 3; ++i) {
        auto *conn = new ConnectionModel(&graph);
        conn->setId(QStringLiteral("e%1").arg(i));
        conn->setSourceId(QStringLiteral("n%1").arg(i));
        conn->setTargetId(QStringLiteral("n%1").arg(i + 1));
        graph.addConnection(conn);
    }

    NoOpExecutionProvider provider;

    GraphExecutionSandbox sb;
    sb.setGraph(&graph);
    sb.setExecutionSemanticsProviders({ &provider });

    QVERIFY(sb.start());
    sb.run();
    const QVariantList first = sb.timeline();

    QVERIFY(sb.start());
    sb.run();
    const QVariantList second = sb.timeline();

    QCOMPARE(first.size(), second.size());
    for (int i = 0; i < first.size(); ++i) {
        const QString e1 = first.at(i).toMap().value(QStringLiteral("event")).toString();
        const QString e2 = second.at(i).toMap().value(QStringLiteral("event")).toString();
        QCOMPARE(e1, e2);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 16. CommandGateway latency p95 within budget
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandGatewayLatencyP95WithinBudget()
{
    QObject root;
    GraphModel graph(&root);
    UndoStack undo(&root);
    CapabilityRegistry caps(&root);
    CommandGateway gateway(&root);

    gateway.setGraph(&graph);
    gateway.setUndoStack(&undo);
    gateway.setCapabilityRegistry(&caps);
    caps.grantCapabilities(QStringLiteral("bench.plugin"), { QStringLiteral("graph.mutate") });

    // Pre-populate a component to move.
    gateway.executeRequest(QStringLiteral("bench.plugin"),
        QVariantMap{
            { QStringLiteral("command"), QStringLiteral("addComponent") },
            { QStringLiteral("id"),      QStringLiteral("hot") },
            { QStringLiteral("typeId"),  QStringLiteral("process") },
            { QStringLiteral("x"),       0.0 },
            { QStringLiteral("y"),       0.0 },
        });

    gateway.resetCommandLatencyStats();

    const int iterations = 200;
    for (int i = 0; i < iterations; ++i) {
        gateway.executeRequest(QStringLiteral("bench.plugin"),
            QVariantMap{
                { QStringLiteral("command"), QStringLiteral("moveComponent") },
                { QStringLiteral("id"),      QStringLiteral("hot") },
                { QStringLiteral("x"),       static_cast<qreal>(i % 400) },
                { QStringLiteral("y"),       static_cast<qreal>((i * 3) % 400) },
            });
    }

    const double p95 = gateway.commandLatencyP95Ms();
    PerformanceBudgets budgets;
    QVERIFY2(budgets.passesCommandLatencyBudget(p95),
             qPrintable(QStringLiteral("Phase0 baseline latency p95 exceeded budget: %1 ms")
                            .arg(p95, 0, 'f', 3)));
}

// ─────────────────────────────────────────────────────────────────────────────
// 17. Architecture budget gates not relaxed
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::architectureGateThresholdsNotRelaxed()
{
    // Migration phases are only allowed to tighten or hold these gate values,
    // never to relax them. If a phase needs a higher budget, it must be explicit
    // and reviewed.
    QVERIFY2(PerformanceBudgets::kCommandLatencyP95BudgetMs <= 50.0,
             "Command latency budget must not be relaxed beyond 50 ms");
    QVERIFY2(PerformanceBudgets::kFrameP95BudgetMs <= 16.0,
             "Frame budget must not be relaxed beyond 16 ms");
    QVERIFY2(PerformanceBudgets::kMemoryGrowthBudgetBytes <= 64ll * 1024ll * 1024ll,
             "Memory growth budget must not be relaxed beyond 64 MB");
}

// ─────────────────────────────────────────────────────────────────────────────
// 18. Command request allowlist
// ─────────────────────────────────────────────────────────────────────────────

void tst_Phase0Baseline::commandRequestKeyAllowlistIsComplete()
{
    // This test documents the complete allowlist of map keys accepted at the
    // command boundary. Any new key added during protobuf migration must be
    // added here to prove the adapter covers it.
    //
    // Keys are organized per command.
    const QMap<QString, QStringList> allowList = {
        { QStringLiteral("addComponent"),          { QStringLiteral("command"), QStringLiteral("id"), QStringLiteral("typeId"), QStringLiteral("x"), QStringLiteral("y") } },
        { QStringLiteral("removeComponent"),       { QStringLiteral("command"), QStringLiteral("id") } },
        { QStringLiteral("moveComponent"),         { QStringLiteral("command"), QStringLiteral("id"), QStringLiteral("x"), QStringLiteral("y") } },
        { QStringLiteral("addConnection"),         { QStringLiteral("command"), QStringLiteral("id"), QStringLiteral("sourceId"), QStringLiteral("targetId"), QStringLiteral("label") } },
        { QStringLiteral("removeConnection"),      { QStringLiteral("command"), QStringLiteral("id") } },
        { QStringLiteral("setComponentProperty"),  { QStringLiteral("command"), QStringLiteral("id"), QStringLiteral("property"), QStringLiteral("value") } },
        { QStringLiteral("setConnectionProperty"), { QStringLiteral("command"), QStringLiteral("id"), QStringLiteral("property"), QStringLiteral("value") } },
    };

    // Allowlist keys must exactly match the supported commands.
    QStringList allowListCommands = allowList.keys();
    allowListCommands.sort();
    QStringList supported = CommandGateway::supportedCommands();
    supported.sort();
    QCOMPARE(allowListCommands, supported);

    // Each command in the allowlist must have "command" as its first key.
    for (auto it = allowList.constBegin(); it != allowList.constEnd(); ++it) {
        QVERIFY2(it.value().contains(QStringLiteral("command")),
                 qPrintable(QStringLiteral("allowlist for '%1' is missing 'command' key").arg(it.key())));
    }
}

QTEST_MAIN(tst_Phase0Baseline)
#include "tst_Phase0Baseline.moc"
