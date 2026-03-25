#include "CapabilityRegistry.h"

CapabilityRegistry::CapabilityRegistry(QObject *parent)
    : QObject(parent)
{}

void CapabilityRegistry::grantCapabilities(const QString &extensionId,
                                           const QStringList &capabilities)
{
    if (extensionId.isEmpty())
        return;

    const QStringList known = knownCapabilities();
    QSet<QString> &grants = m_grants[extensionId];

    for (const QString &cap : capabilities) {
        if (!known.contains(cap))
            continue; // silently ignore unrecognised capability strings

        if (!grants.contains(cap)) {
            grants.insert(cap);
            m_auditLog.append({CapabilityAuditEntry::Event::Granted, extensionId, cap, {}});
            emit capabilityGranted(extensionId, cap);
        }
    }
}

void CapabilityRegistry::revokeCapabilities(const QString &extensionId)
{
    if (!m_grants.contains(extensionId))
        return;

    const QSet<QString> caps = m_grants.take(extensionId);
    for (const QString &cap : caps)
        m_auditLog.append({CapabilityAuditEntry::Event::Revoked, extensionId, cap, {}});

    emit capabilitiesRevoked(extensionId);
}

bool CapabilityRegistry::hasCapability(const QString &extensionId,
                                       const QString &capability) const
{
    const auto it = m_grants.constFind(extensionId);
    if (it == m_grants.constEnd())
        return false;
    return it->contains(capability);
}

bool CapabilityRegistry::checkAndAudit(const QString &extensionId,
                                        const QString &capability,
                                        const QString &context)
{
    const bool granted = hasCapability(extensionId, capability);

    if (granted) {
        m_auditLog.append({CapabilityAuditEntry::Event::Granted, extensionId, capability, context});
        emit capabilityGranted(extensionId, capability);
    } else {
        m_auditLog.append({CapabilityAuditEntry::Event::Blocked, extensionId, capability, context});
        emit capabilityBlocked(extensionId, capability, context);
    }

    return granted;
}

QStringList CapabilityRegistry::grantedCapabilities(const QString &extensionId) const
{
    const auto it = m_grants.constFind(extensionId);
    if (it == m_grants.constEnd())
        return {};
    return QStringList(it->begin(), it->end());
}

QList<CapabilityAuditEntry> CapabilityRegistry::auditLog() const
{
    return m_auditLog;
}

void CapabilityRegistry::clearAuditLog()
{
    m_auditLog.clear();
}

QStringList CapabilityRegistry::knownCapabilities()
{
    return {
        QStringLiteral("file.read"),
        QStringLiteral("file.write"),
        QStringLiteral("network.access"),
        QStringLiteral("graph.mutate"),
        QStringLiteral("advanced.execution"),
    };
}
