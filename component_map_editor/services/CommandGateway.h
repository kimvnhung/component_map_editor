#ifndef COMMANDGATEWAY_H
#define COMMANDGATEWAY_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class GraphModel;
class UndoStack;
class CapabilityRegistry;
class InvariantChecker;

// ── CommandGateway ────────────────────────────────────────────────────────────
// Single authoritative entry point for all plugin-initiated graph mutations.
//
// The enforcement contract:
//   1. Extensions submit mutations as data (QVariantMap commandRequest) through
//      executeRequest().  They never receive a writable GraphModel pointer.
//   2. executeRequest() verifies the caller holds Capability::kGraphMutate
//      before doing anything else.
//   3. An optional InvariantChecker runs pre-command and post-command:
//        • Pre-failure  → request rejected; graph unchanged.
//        • Post-failure → committed command is undone; invariantViolationRolledBack
//                         is emitted; graph state is restored to pre-command.
//   4. Every request (successful or blocked) is appended to requestLog().
//
// System-initiated mutations use executeSystemCommand() which skips the
// capability check but still honours the InvariantChecker.
//
// Thread-affinity: main thread only.
class CommandGateway : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GraphModel *graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(UndoStack  *undoStack READ undoStack WRITE setUndoStack NOTIFY undoStackChanged FINAL)

public:
    explicit CommandGateway(QObject *parent = nullptr);

    GraphModel *graph() const;
    void setGraph(GraphModel *graph);

    UndoStack *undoStack() const;
    void setUndoStack(UndoStack *stack);

    CapabilityRegistry *capabilityRegistry() const;
    void setCapabilityRegistry(CapabilityRegistry *registry);

    InvariantChecker *invariantChecker() const;
    void setInvariantChecker(InvariantChecker *checker);

    // ── Plugin/extension entry point ──────────────────────────────────────
    // extensionId must hold Capability::kGraphMutate in the CapabilityRegistry.
    //
    // commandRequest keys (all commands):
    //   "command"   QString  — one of supportedCommands()
    //
    // Per-command extra keys:
    //   addComponent        : id(str), typeId(str), x(real), y(real)
    //   removeComponent     : id(str)
    //   moveComponent       : id(str), x(real), y(real)
    //   addConnection       : id(str), sourceId(str), targetId(str), label(str,opt)
    //   removeConnection    : id(str)
    //   setComponentProperty: id(str), property(str), value(variant)
    //   setConnectionProperty: id(str), property(str), value(variant)
    //
    // Returns false on any failure (capability denied, pre/post invariant
    // violation, or unknown command).  *error is set when non-null.
    Q_INVOKABLE bool executeRequest(const QString &extensionId,
                                    const QVariantMap &commandRequest,
                                    QString *error = nullptr);

    // ── System/internal entry point ───────────────────────────────────────
    // Same command format but capability check is skipped.
    // InvariantChecker still runs.
    bool executeSystemCommand(const QVariantMap &commandRequest, QString *error = nullptr);

    // ── Audit ─────────────────────────────────────────────────────────────
    QVariantList requestLog() const;
    void clearRequestLog();

    static QStringList supportedCommands();

signals:
    void graphChanged();
    void undoStackChanged();

    // Emitted when a mutation attempt was blocked (capability or invariant).
    void mutationBlocked(const QString &extensionId, const QString &reason);

    // Emitted after a post-command invariant failure.  The command has been
    // rolled back before this signal fires.
    void invariantViolationRolledBack(const QString &commandType,
                                      const QString &violation);

    // Emitted after a successful mutation.
    void commandExecuted(const QString &extensionId, const QString &commandType);

private:
    bool dispatchCommand(const QString &actor,
                         const QVariantMap &commandRequest,
                         bool requireCapability,
                         QString *error);

    bool runPreChecks(const QString &commandType, QString *error) const;
    bool runPostChecks(const QString &commandType, QString *error);

    void appendLog(const QString &actor,
                   const QString &commandType,
                   bool blocked,
                   const QString &reason);

    QPointer<GraphModel>          m_graph;
    QPointer<UndoStack>           m_undoStack;
    QPointer<CapabilityRegistry>  m_capabilityRegistry;
    QPointer<InvariantChecker>    m_invariantChecker;
    QVariantList                  m_requestLog;
};

#endif // COMMANDGATEWAY_H
