#include <QtTest>

#include "commands/UndoStack.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/CapabilityRegistry.h"
#include "services/CommandGateway.h"
#include "services/GraphEditorController.h"
#include "services/InvariantChecker.h"

// ── Test helpers ──────────────────────────────────────────────────────────────

namespace {

// Assemble a gateway wired with a fresh graph, undo stack, capability
// registry, and invariant checker.  All objects are parented to *parent so
// ownership is automatic.
struct GatewayFixture {
    GraphModel         *graph;
    UndoStack          *undo;
    CapabilityRegistry *caps;
    InvariantChecker   *invariants;
    CommandGateway     *gateway;

    explicit GatewayFixture(QObject *parent = nullptr)
    {
        graph      = new GraphModel(parent);
        undo       = new UndoStack(parent);
        caps       = new CapabilityRegistry(parent);
        invariants = new InvariantChecker(parent);
        gateway    = new CommandGateway(parent);

        gateway->setGraph(graph);
        gateway->setUndoStack(undo);
        gateway->setCapabilityRegistry(caps);
        gateway->setInvariantChecker(invariants);
    }
};

QVariantMap addComponentRequest(const QString &id,
                                const QString &typeId = QStringLiteral("process"),
                                qreal x = 0.0,
                                qreal y = 0.0)
{
    QVariantMap req;
    req.insert(QStringLiteral("command"), QStringLiteral("addComponent"));
    req.insert(QStringLiteral("id"),      id);
    req.insert(QStringLiteral("typeId"),  typeId);
    req.insert(QStringLiteral("x"),       x);
    req.insert(QStringLiteral("y"),       y);
    return req;
}

QVariantMap removeComponentRequest(const QString &id)
{
    QVariantMap req;
    req.insert(QStringLiteral("command"), QStringLiteral("removeComponent"));
    req.insert(QStringLiteral("id"),      id);
    return req;
}

QVariantMap addConnectionRequest(const QString &id,
                                 const QString &src,
                                 const QString &tgt,
                                 const QString &label = {})
{
    QVariantMap req;
    req.insert(QStringLiteral("command"),  QStringLiteral("addConnection"));
    req.insert(QStringLiteral("id"),       id);
    req.insert(QStringLiteral("sourceId"), src);
    req.insert(QStringLiteral("targetId"), tgt);
    if (!label.isEmpty())
        req.insert(QStringLiteral("label"), label);
    return req;
}

} // namespace

// ── Test class ────────────────────────────────────────────────────────────────

class tst_SecurityGuardrails : public QObject
{
    Q_OBJECT

private slots:
    // ── Capability enforcement ────────────────────────────────────────────
    void unauthorizedMutationIsBlocked();
    void authorizedMutationSucceeds();
    void capabilitiesRevokedAtRuntimeBlockSubsequentRequests();

    // ── Invariant checker ─────────────────────────────────────────────────
    void invariantViolationPostCommandRollsBack();
    void invariantViolationPreCommandPreventsExecution();
    void invariantCheckerAllViolationsIsExhaustive();

    // ── CommandGateway log and protocol ──────────────────────────────────
    void auditLogRecordsBlockedAndAllowedRequests();
    void unknownCommandTypeIsBlocked();
    void missingRequiredFieldIsBlocked();
    void systemCommandBypassesCapsButHonorsInvariants();

    // ── Mutation isolation ────────────────────────────────────────────────
    void blockedRequestLeavesGraphAndUndoStackUnchanged();
    void successfulCommandIsUndoable();
    void rollbackAfterInvariantViolationRestoresExactState();

    // ── UI-actor strict mode (PR5) ────────────────────────────────────────
    void uiActorStrictModeBlocksPreCheckViolation();
    void uiActorStrictModeRollsBackPostViolation();
    void uiActorStrictModeAllowsValidMutation();
    void uiActorNonStrictModeBypassesInvariantChecker();
    void uiActorStrictModeEmitsRollbackSignalOnViolation();
};

// ─────────────────────────────────────────────────────────────────────────────
//  Capability enforcement
// ─────────────────────────────────────────────────────────────────────────────

void tst_SecurityGuardrails::unauthorizedMutationIsBlocked()
{
    QObject root;
    GatewayFixture f(&root);

    // "rogue_plugin" has NOT been granted graph.mutate.
    QString error;
    bool ok = f.gateway->executeRequest(
        QStringLiteral("rogue_plugin"), addComponentRequest(QStringLiteral("c1")), &error);

    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());
    QCOMPARE(f.graph->componentCount(), 0);

    // Audit log must contain a BLOCKED entry for the capability check.
    bool foundBlocked = false;
    for (const CapabilityAuditEntry &entry : f.caps->auditLog()) {
        if (entry.event == CapabilityAuditEntry::Event::Blocked
            && entry.extensionId == QStringLiteral("rogue_plugin")
            && entry.capability  == QStringLiteral("graph.mutate")) {
            foundBlocked = true;
            break;
        }
    }
    QVERIFY2(foundBlocked,
             "CapabilityRegistry audit log must contain a Blocked entry for graph.mutate");

    // Gateway request log must also reflect the blocked mutation.
    bool foundLog = false;
    for (const QVariant &v : f.gateway->requestLog()) {
        const QVariantMap entry = v.toMap();
        if (entry.value(QStringLiteral("actor")).toString() == QStringLiteral("rogue_plugin")
            && entry.value(QStringLiteral("blocked")).toBool()) {
            foundLog = true;
            break;
        }
    }
    QVERIFY2(foundLog, "Gateway request log must record the blocked mutation");
}

void tst_SecurityGuardrails::authorizedMutationSucceeds()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("trusted_plugin"),
                               {QStringLiteral("graph.mutate")});

    QString error;
    bool ok = f.gateway->executeRequest(
        QStringLiteral("trusted_plugin"), addComponentRequest(QStringLiteral("c1")), &error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(f.graph->componentCount(), 1);
    QVERIFY(f.graph->componentById(QStringLiteral("c1")) != nullptr);
}

void tst_SecurityGuardrails::capabilitiesRevokedAtRuntimeBlockSubsequentRequests()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin_x"),
                               {QStringLiteral("graph.mutate")});

    // First mutation — should succeed.
    QVERIFY(f.gateway->executeRequest(
        QStringLiteral("plugin_x"), addComponentRequest(QStringLiteral("c1"))));

    // Revoke capabilities mid-session.
    f.caps->revokeCapabilities(QStringLiteral("plugin_x"));

    // Second mutation — must now be blocked.
    QString error;
    bool ok = f.gateway->executeRequest(
        QStringLiteral("plugin_x"), addComponentRequest(QStringLiteral("c2")), &error);

    QVERIFY(!ok);
    QCOMPARE(f.graph->componentCount(), 1); // only c1 was added
}

// ─────────────────────────────────────────────────────────────────────────────
//  Invariant checker
// ─────────────────────────────────────────────────────────────────────────────

void tst_SecurityGuardrails::invariantViolationPostCommandRollsBack()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin"),
                               {QStringLiteral("graph.mutate")});

    // Invariant: no component may have typeId == "forbidden".
    f.invariants->registerInvariant(
        QStringLiteral("no-forbidden-type"),
        [](const GraphModel *graph, QString *msg) -> bool {
            for (ComponentModel *comp : graph->componentList()) {
                if (comp->type() == QStringLiteral("forbidden")) {
                    if (msg)
                        *msg = QStringLiteral("Component '%1' has forbidden type").arg(comp->id());
                    return false;
                }
            }
            return true;
        });

    // Attempt to add a "forbidden"-typed component.
    QString error;
    bool ok = f.gateway->executeRequest(
        QStringLiteral("plugin"),
        addComponentRequest(QStringLiteral("bad"), QStringLiteral("forbidden")),
        &error);

    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());

    // The graph must be empty — the command was rolled back.
    QCOMPARE(f.graph->componentCount(), 0);

    // The undo stack must be empty after rollback.
    QCOMPARE(f.undo->count(), 0);
    QVERIFY(!f.undo->canRedo());
}

void tst_SecurityGuardrails::invariantViolationPreCommandPreventsExecution()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin"),
                               {QStringLiteral("graph.mutate")});

    // Put the graph into a pre-violation state by direct system command
    // flagging it so the invariant checker immediately fires.
    // We do this by registering an invariant that always fails.
    f.invariants->registerInvariant(
        QStringLiteral("always-fail"),
        [](const GraphModel *, QString *msg) -> bool {
            if (msg) *msg = QStringLiteral("graph is locked (always-fail invariant)");
            return false;
        });

    QString error;
    bool ok = f.gateway->executeRequest(
        QStringLiteral("plugin"),
        addComponentRequest(QStringLiteral("c1")),
        &error);

    QVERIFY(!ok);
    // The pre-check rejects the request before redo() is ever called.
    QCOMPARE(f.graph->componentCount(), 0);
    QCOMPARE(f.undo->count(), 0);
}

void tst_SecurityGuardrails::invariantCheckerAllViolationsIsExhaustive()
{
    QObject root;
    InvariantChecker checker(&root);
    GraphModel       graph(&root);

    // Add two always-failing invariants.
    checker.registerInvariant(QStringLiteral("fail-a"),
        [](const GraphModel *, QString *m) { if (m) *m = QStringLiteral("A"); return false; });
    checker.registerInvariant(QStringLiteral("fail-b"),
        [](const GraphModel *, QString *m) { if (m) *m = QStringLiteral("B"); return false; });
    checker.registerInvariant(QStringLiteral("pass-c"),
        [](const GraphModel *, QString *)  { return true; });

    const QStringList violations = checker.allViolations(&graph);
    QCOMPARE(violations.size(), 2);
    QVERIFY(violations.at(0).contains(QStringLiteral("fail-a")));
    QVERIFY(violations.at(1).contains(QStringLiteral("fail-b")));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Gateway log and protocol
// ─────────────────────────────────────────────────────────────────────────────

void tst_SecurityGuardrails::auditLogRecordsBlockedAndAllowedRequests()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin"),
                               {QStringLiteral("graph.mutate")});

    // One blocked (no capability at all for "attacker").
    f.gateway->executeRequest(QStringLiteral("attacker"),
                              addComponentRequest(QStringLiteral("x")));

    // One successful.
    f.gateway->executeRequest(QStringLiteral("plugin"),
                              addComponentRequest(QStringLiteral("c1")));

    const QVariantList log = f.gateway->requestLog();
    QCOMPARE(log.size(), 2);

    const QVariantMap blockedEntry  = log.at(0).toMap();
    const QVariantMap allowedEntry  = log.at(1).toMap();

    QVERIFY(blockedEntry.value(QStringLiteral("blocked")).toBool());
    QCOMPARE(blockedEntry.value(QStringLiteral("actor")).toString(),
             QStringLiteral("attacker"));

    QVERIFY(!allowedEntry.value(QStringLiteral("blocked")).toBool());
    QCOMPARE(allowedEntry.value(QStringLiteral("actor")).toString(),
             QStringLiteral("plugin"));
}

void tst_SecurityGuardrails::unknownCommandTypeIsBlocked()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin"),
                               {QStringLiteral("graph.mutate")});

    QVariantMap req;
    req.insert(QStringLiteral("command"), QStringLiteral("nukeDatabaseForFun"));

    QString error;
    bool ok = f.gateway->executeRequest(QStringLiteral("plugin"), req, &error);

    QVERIFY(!ok);
    QVERIFY(error.contains(QStringLiteral("nukeDatabaseForFun")));
    QCOMPARE(f.graph->componentCount(), 0);
}

void tst_SecurityGuardrails::missingRequiredFieldIsBlocked()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin"),
                               {QStringLiteral("graph.mutate")});

    // addComponent without 'id'.
    QVariantMap req;
    req.insert(QStringLiteral("command"), QStringLiteral("addComponent"));
    req.insert(QStringLiteral("typeId"),  QStringLiteral("process"));

    QString error;
    bool ok = f.gateway->executeRequest(QStringLiteral("plugin"), req, &error);

    QVERIFY(!ok);
    QVERIFY(error.contains(QStringLiteral("id")));
    QCOMPARE(f.graph->componentCount(), 0);
}

void tst_SecurityGuardrails::systemCommandBypassesCapsButHonorsInvariants()
{
    QObject root;
    GatewayFixture f(&root);

    // No capabilities granted — system commands should still succeed.
    QString error;
    bool ok = f.gateway->executeSystemCommand(
        addComponentRequest(QStringLiteral("sys1")), &error);
    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(f.graph->componentCount(), 1);

    // Now register a blocking invariant.
    f.invariants->registerInvariant(
        QStringLiteral("no-sys2-type"),
        [](const GraphModel *graph, QString *msg) -> bool {
            for (ComponentModel *comp : graph->componentList()) {
                if (comp->id() == QStringLiteral("sys2")) {
                    if (msg) *msg = QStringLiteral("sys2 is not allowed");
                    return false;
                }
            }
            return true;
        });

    bool ok2 = f.gateway->executeSystemCommand(
        addComponentRequest(QStringLiteral("sys2")), &error);
    QVERIFY(!ok2);
    // Rollback should have kept the graph to only sys1.
    QCOMPARE(f.graph->componentCount(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mutation isolation
// ─────────────────────────────────────────────────────────────────────────────

void tst_SecurityGuardrails::blockedRequestLeavesGraphAndUndoStackUnchanged()
{
    QObject root;
    GatewayFixture f(&root);

    // Pre-populate the graph via system command.
    f.gateway->executeSystemCommand(addComponentRequest(QStringLiteral("base")));
    const int baseCount = f.graph->componentCount();
    const int baseUndoCount = f.undo->count();

    // Attempt a blocked extension request.
    f.gateway->executeRequest(QStringLiteral("rogue"),
                              addComponentRequest(QStringLiteral("extra")));

    QCOMPARE(f.graph->componentCount(), baseCount);
    QCOMPARE(f.undo->count(), baseUndoCount);
}

void tst_SecurityGuardrails::successfulCommandIsUndoable()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin"),
                               {QStringLiteral("graph.mutate")});

    f.gateway->executeRequest(QStringLiteral("plugin"),
                              addComponentRequest(QStringLiteral("c1")));
    QCOMPARE(f.graph->componentCount(), 1);
    QVERIFY(f.undo->canUndo());

    f.undo->undo();
    QCOMPARE(f.graph->componentCount(), 0);
}

void tst_SecurityGuardrails::rollbackAfterInvariantViolationRestoresExactState()
{
    QObject root;
    GatewayFixture f(&root);

    f.caps->grantCapabilities(QStringLiteral("plugin"),
                               {QStringLiteral("graph.mutate")});

    // Add two components as baseline state.
    f.gateway->executeRequest(QStringLiteral("plugin"),
                              addComponentRequest(QStringLiteral("a")));
    f.gateway->executeRequest(QStringLiteral("plugin"),
                              addComponentRequest(QStringLiteral("b")));
    QCOMPARE(f.graph->componentCount(), 2);

    const int undoCountBefore = f.undo->count();

    // Register invariant that forbids a component named "evil".
    f.invariants->registerInvariant(
        QStringLiteral("no-evil-id"),
        [](const GraphModel *graph, QString *msg) -> bool {
            if (graph->componentById(QStringLiteral("evil"))) {
                if (msg) *msg = QStringLiteral("'evil' component is not permitted");
                return false;
            }
            return true;
        });

    bool ok = f.gateway->executeRequest(QStringLiteral("plugin"),
                                        addComponentRequest(QStringLiteral("evil")));
    QVERIFY(!ok);

    // Graph must be exactly as before — a, b, no evil.
    QCOMPARE(f.graph->componentCount(), 2);
    QVERIFY(f.graph->componentById(QStringLiteral("a"))    != nullptr);
    QVERIFY(f.graph->componentById(QStringLiteral("b"))    != nullptr);
    QVERIFY(f.graph->componentById(QStringLiteral("evil")) == nullptr);

    // Undo stack must be back to exactly what it was before the failed attempt.
    QCOMPARE(f.undo->count(), undoCountBefore);
    QVERIFY(!f.undo->canRedo());
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI-actor strict mode  (PR5)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Minimal fixture for GraphEditorController strict-mode tests.
// No CapabilityRegistry needed — the UI actor owns the graph unconditionally.
struct ControllerFixture {
    GraphModel            *graph;
    UndoStack             *undo;
    InvariantChecker      *invariants;
    GraphEditorController *controller;

    explicit ControllerFixture(QObject *parent = nullptr)
    {
        graph      = new GraphModel(parent);
        undo       = new UndoStack(parent);
        invariants = new InvariantChecker(parent);
        controller = new GraphEditorController(parent);

        controller->setGraph(graph);
        controller->setUndoStack(undo);
        controller->setInvariantChecker(invariants);
    }
};

} // namespace

// Pre-check fires → mutation is never executed; undo stack unchanged.
void tst_SecurityGuardrails::uiActorStrictModeBlocksPreCheckViolation()
{
    QObject root;
    ControllerFixture f(&root);

    // Invariant that already fails before any mutation.
    f.invariants->registerInvariant(
        QStringLiteral("always-fail"),
        [](const GraphModel *, QString *msg) -> bool {
            if (msg) *msg = QStringLiteral("pre-existing violation");
            return false;
        });

    f.controller->setStrictMode(true);

    const int undoBefore = f.undo->count();
    const QString id = f.controller->createPaletteComponent(
        QStringLiteral("process"), QStringLiteral("Node"), {}, {}, 0.0, 0.0, 96.0, 96.0);

    QVERIFY2(id.isEmpty(), "Pre-check violation must block the mutation");
    QCOMPARE(f.graph->componentCount(), 0);
    QCOMPARE(f.undo->count(), undoBefore);
}

// Post-check fires → command is rolled back; graph restored to pre-command state.
void tst_SecurityGuardrails::uiActorStrictModeRollsBackPostViolation()
{
    QObject root;
    ControllerFixture f(&root);

    // Invariant: no component may have typeId == "forbidden".
    f.invariants->registerInvariant(
        QStringLiteral("no-forbidden-type"),
        [](const GraphModel *graph, QString *msg) -> bool {
            for (ComponentModel *comp : graph->componentList()) {
                if (comp->type() == QStringLiteral("forbidden")) {
                    if (msg)
                        *msg = QStringLiteral("component '%1' has forbidden type").arg(comp->id());
                    return false;
                }
            }
            return true;
        });

    f.controller->setStrictMode(true);

    const int undoBefore = f.undo->count();
    const QString id = f.controller->createPaletteComponent(
        QStringLiteral("forbidden"), QStringLiteral("Bad"), {}, {}, 0.0, 0.0, 96.0, 96.0);

    QVERIFY2(id.isEmpty(), "Post-check violation must roll back and return empty");
    QCOMPARE(f.graph->componentCount(), 0);
    // Undo stack must be clean — the rolled-back command is discarded.
    QCOMPARE(f.undo->count(), undoBefore);
    QVERIFY(!f.undo->canRedo());
}

// Valid mutation in strict mode — no invariant violations — succeeds normally.
void tst_SecurityGuardrails::uiActorStrictModeAllowsValidMutation()
{
    QObject root;
    ControllerFixture f(&root);

    // Register a passing invariant to confirm the check itself runs.
    f.invariants->registerInvariant(
        QStringLiteral("max-10-components"),
        [](const GraphModel *graph, QString *msg) -> bool {
            if (graph->componentCount() > 10) {
                if (msg) *msg = QStringLiteral("too many components");
                return false;
            }
            return true;
        });

    f.controller->setStrictMode(true);

    const QString id = f.controller->createPaletteComponent(
        QStringLiteral("process"), QStringLiteral("Node"), {}, {}, 10.0, 20.0, 96.0, 96.0);

    QVERIFY2(!id.isEmpty(), "Valid mutation must succeed in strict mode");
    QCOMPARE(f.graph->componentCount(), 1);
    QVERIFY(f.undo->canUndo());
    // Undo must remove the component.
    f.undo->undo();
    QCOMPARE(f.graph->componentCount(), 0);
}

// Non-strict mode (default): invariant checker is present but ignored.
void tst_SecurityGuardrails::uiActorNonStrictModeBypassesInvariantChecker()
{
    QObject root;
    ControllerFixture f(&root);

    // Always-failing invariant — would block every mutation in strict mode.
    f.invariants->registerInvariant(
        QStringLiteral("always-fail"),
        [](const GraphModel *, QString *msg) -> bool {
            if (msg) *msg = QStringLiteral("always fails");
            return false;
        });

    // strictMode is false by default — mutations must go through unchecked.
    QVERIFY(!f.controller->strictMode());

    const QString id = f.controller->createPaletteComponent(
        QStringLiteral("process"), QStringLiteral("Node"), {}, {}, 0.0, 0.0, 96.0, 96.0);

    QVERIFY2(!id.isEmpty(), "Non-strict mode must bypass the invariant checker");
    QCOMPARE(f.graph->componentCount(), 1);
}

// invariantViolationRolledBack signal is emitted on post-check rollback.
void tst_SecurityGuardrails::uiActorStrictModeEmitsRollbackSignalOnViolation()
{
    QObject root;
    ControllerFixture f(&root);

    f.invariants->registerInvariant(
        QStringLiteral("no-forbidden-type"),
        [](const GraphModel *graph, QString *msg) -> bool {
            for (ComponentModel *comp : graph->componentList()) {
                if (comp->type() == QStringLiteral("forbidden")) {
                    if (msg) *msg = QStringLiteral("forbidden type detected");
                    return false;
                }
            }
            return true;
        });

    f.controller->setStrictMode(true);

    QString capturedOpType;
    QString capturedViolation;
    QObject::connect(f.controller, &GraphEditorController::invariantViolationRolledBack,
                     &root, [&](const QString &opType, const QString &violation) {
                         capturedOpType   = opType;
                         capturedViolation = violation;
                     });

    bool mutationBlockedEmitted = false;
    QObject::connect(f.controller, &GraphEditorController::mutationBlocked,
                     &root, [&](const QString &, const QString &) {
                         mutationBlockedEmitted = true;
                     });

    f.controller->createPaletteComponent(
        QStringLiteral("forbidden"), QStringLiteral("Bad"), {}, {}, 0.0, 0.0, 96.0, 96.0);

    QVERIFY2(!capturedOpType.isEmpty(),   "invariantViolationRolledBack must be emitted");
    QCOMPARE(capturedOpType, QStringLiteral("createPaletteComponent"));
    QVERIFY2(!capturedViolation.isEmpty(), "Violation message must be non-empty");
    QVERIFY2(mutationBlockedEmitted,       "mutationBlocked must also be emitted");
}

// ─────────────────────────────────────────────────────────────────────────────

QTEST_MAIN(tst_SecurityGuardrails)
#include "tst_SecurityGuardrails.moc"
