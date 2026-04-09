#ifndef CAPABILITYREGISTRY_H
#define CAPABILITYREGISTRY_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

// ── Capability name constants ─────────────────────────────────────────────────
// Capabilities are declared in an extension's manifest (ExtensionManifest::capabilities).
// The CapabilityRegistry grants exactly those capabilities at registration time.
// Extensions must hold the matching capability before the CommandGateway will
// execute a mutation on their behalf.
namespace Capability {
    // Allows reading files from the host file system through approved APIs.
    constexpr char kFileRead[]          = "file.read";
    // Allows writing files to the host file system through approved APIs.
    constexpr char kFileWrite[]         = "file.write";
    // Allows initiating outbound network requests through approved APIs.
    constexpr char kNetworkAccess[]     = "network.access";
    // Allows pushing graph mutation commands through the CommandGateway.
    // Without this capability, executeRequest() is always rejected.
    constexpr char kGraphMutate[]       = "graph.mutate";
    // Allows use of advanced execution sandbox APIs beyond basic step/run.
    constexpr char kAdvancedExecution[] = "advanced.execution";
} // namespace Capability

// ── Audit entry ───────────────────────────────────────────────────────────────
struct CapabilityAuditEntry {
    enum class Event { Granted, Blocked, Revoked };

    Event   event;
    QString extensionId;
    QString capability;
    QString context;    // caller-supplied free-form context string
};

// ── CapabilityRegistry ────────────────────────────────────────────────────────
// Thread-affinity: main thread only (follows QObject rules).
class CapabilityRegistry : public QObject
{
    Q_OBJECT

public:
    explicit CapabilityRegistry(QObject *parent = nullptr);

    // Register capabilities declared in an extension manifest.
    // Only strings that appear in knownCapabilities() are stored; unknown
    // strings are silently ignored to prevent typo-based capability escalation.
    void grantCapabilities(const QString &extensionId, const QStringList &capabilities);

    // Remove all capabilities granted to an extension (e.g. on unload).
    void revokeCapabilities(const QString &extensionId);

    // Query without logging.
    bool hasCapability(const QString &extensionId, const QString &capability) const;

    // Query and automatically append to the audit log.
    // Returns true if the capability is held; false if blocked.
    // Emits capabilityGranted or capabilityBlocked accordingly.
    bool checkAndAudit(const QString &extensionId,
                       const QString &capability,
                       const QString &context = {});

    QStringList grantedCapabilities(const QString &extensionId) const;

    QList<CapabilityAuditEntry> auditLog() const;
    void clearAuditLog();

    // All capability strings the registry recognises.
    static QStringList knownCapabilities();

signals:
    void capabilityGranted(const QString &extensionId, const QString &capability);
    void capabilityBlocked(const QString &extensionId,
                           const QString &capability,
                           const QString &context);
    void capabilitiesRevoked(const QString &extensionId);

private:
    QHash<QString, QSet<QString>> m_grants;   // extensionId → capability set
    QList<CapabilityAuditEntry>   m_auditLog;
};

#endif // CAPABILITYREGISTRY_H
