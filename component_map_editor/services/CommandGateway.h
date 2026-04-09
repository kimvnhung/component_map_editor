#ifndef COMMANDGATEWAY_H
#define COMMANDGATEWAY_H

#include <QObject>
#include <QPointer>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "command.pb.h"
#include "adapters/CommandAdapterRegistry.h"

class GraphModel;
class UndoStack;
class CapabilityRegistry;
class InvariantChecker;

// ── CommandGateway ────────────────────────────────────────────────────────────
// Single authoritative entry point for all plugin-initiated graph mutations.
//
// The enforcement contract:
//   1. Preferred external boundary is executeTypedRequest() with
//      cme::GraphCommandRequest.
//   2. Legacy executeRequest() (QVariantMap) remains as a compatibility wrapper.
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

    // ── Plugin/extension entry point (legacy wrapper) ─────────────────────
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

    // Typed external entrypoint. Preferred for integrations outside the library.
    bool executeTypedRequest(const QString &extensionId,
                             const cme::GraphCommandRequest &commandRequest,
                             QString *error = nullptr);

    // ── System/internal entry point (legacy wrapper) ──────────────────────
    // Same command format but capability check is skipped.
    // InvariantChecker still runs.
    bool executeSystemCommand(const QVariantMap &commandRequest, QString *error = nullptr);

    // Typed system/internal entrypoint.
    bool executeTypedSystemCommand(const cme::GraphCommandRequest &commandRequest,
                                   QString *error = nullptr);

    // ── Audit ─────────────────────────────────────────────────────────────
    QVariantList requestLog() const;
    void clearRequestLog();

    int maxLatencySamples() const;
    void setMaxLatencySamples(int value);
    double commandLatencyP50Ms() const;
    double commandLatencyP95Ms() const;
    double lastCommandLatencyMs() const;
    int commandLatencySampleCount() const;
    void resetCommandLatencyStats();

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
    void recordLatencySampleNs(qint64 elapsedNs);
    static double percentile(const QVector<double> &samples, double p);

    bool dispatchCommand(const QString &actor,
                         const QVariantMap &commandRequest,
                         bool requireCapability,
                         QString *error);

    // ── Phase 3: Typed command execution using protobuf oneof/enum dispatch ──
    // Executes a typed GraphCommandRequest (already converted from legacy map).
    // Returns true on success, false on failure. Sets *error on failure.
    // Responsibility: command validation (missing fields, not found, duplicate)
    //                 and delegation to UndoStack. Pre/post checks and logging
    //                 are handled by dispatchCommand().
    bool executeTypedCommand(const QString &actor,
                             const cme::GraphCommandRequest &protoCmd,
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

    QVector<double>               m_latencySamplesMs;
    int                           m_maxLatencySamples = 4096;
    double                        m_lastCommandLatencyMs = 0.0;

    // ── Adapter pattern for extensible command handling ──────────────────
    std::unique_ptr<CommandAdapterRegistry> m_adapterRegistry;
};

#endif // COMMANDGATEWAY_H
