#ifndef INVARIANTCHECKER_H
#define INVARIANTCHECKER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

class GraphModel;

// ── InvariantEntry ────────────────────────────────────────────────────────────
// A named predicate evaluated against a read-only GraphModel snapshot.
// The check function must be pure (no side effects, no graph mutations).
// If the invariant is violated, it should set *violation to a diagnostic
// message and return false.
struct InvariantEntry {
    QString name;
    std::function<bool(const GraphModel *, QString *violation)> check;
};

// ── InvariantChecker ──────────────────────────────────────────────────────────
// Maintains a registry of named graph invariants and evaluates them on demand.
//
// The CommandGateway calls checkAll() before and after every command push:
//   • Pre-check failure → command is rejected before it is ever executed.
//   • Post-check failure → CommandGateway issues undo() to roll back the
//     command that was already pushed and emits invariantViolated().
//
// Thread-affinity: main thread only.
class InvariantChecker : public QObject
{
    Q_OBJECT

public:
    explicit InvariantChecker(QObject *parent = nullptr);

    // Register a named invariant.  Duplicate names are silently overwritten so
    // tests can replace built-in invariants without noisy error paths.
    void registerInvariant(const QString &name,
                           std::function<bool(const GraphModel *, QString *)> check);

    void unregisterInvariant(const QString &name);
    void clearInvariants();

    int invariantCount() const;

    // Returns true only if every registered invariant passes.
    // On the first failure *firstViolation is set to
    //   "<invariant-name>: <diagnostic>" and the remaining invariants
    //   are skipped (fail-fast).  Pass nullptr to suppress the output.
    bool checkAll(const GraphModel *graph, QString *firstViolation = nullptr) const;

    // Returns a message string for every failing invariant (exhaustive scan).
    QStringList allViolations(const GraphModel *graph) const;

signals:
    // Emitted by checkAll() the first time a violation is discovered.
    void invariantViolated(const QString &invariantName, const QString &details);

private:
    QList<InvariantEntry> m_invariants;
};

#endif // INVARIANTCHECKER_H
