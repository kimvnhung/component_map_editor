#include <QtTest>

#include "commands/UndoStack.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/CapabilityRegistry.h"
#include "services/CommandGateway.h"
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

QTEST_MAIN(tst_SecurityGuardrails)
#include "tst_SecurityGuardrails.moc"
