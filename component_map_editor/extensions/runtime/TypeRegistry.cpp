#include "TypeRegistry.h"

TypeRegistry::TypeRegistry(QObject *parent)
    : QObject(parent)
{}

void TypeRegistry::rebuildFromRegistry(const ExtensionContractRegistry &registry)
{
    const QStringList prevIds = m_orderedTypeIds;

    m_descriptors.clear();
    m_defaults.clear();
    m_orderedTypeIds.clear();
    m_connectionPolicies.clear();

    // --- Component-type cache ----------------------------------------------
    const QList<const IComponentTypeProvider *> componentProviders = registry.componentTypeProviders();
    for (const IComponentTypeProvider *provider : componentProviders) {
        if (!provider)
            continue;
        const QStringList typeIds = provider->componentTypeIds();
        for (const QString &typeId : typeIds) {
            if (typeId.isEmpty() || m_descriptors.contains(typeId))
                continue; // skip empty IDs and first-registration wins
            m_descriptors.insert(typeId, provider->componentTypeDescriptor(typeId));
            m_defaults.insert(typeId,    provider->defaultComponentProperties(typeId));
            m_orderedTypeIds.append(typeId);
        }
    }

    // --- Connection-policy list --------------------------------------------
    m_connectionPolicies = registry.connectionPolicyProviders();

    if (m_orderedTypeIds != prevIds)
        emit componentTypesChanged();
}

// ---------------------------------------------------------------------------
// Component-type lookups
// ---------------------------------------------------------------------------

bool TypeRegistry::hasComponentType(const QString &typeId) const
{
    return m_descriptors.contains(typeId);
}

QVariantMap TypeRegistry::componentTypeDescriptor(const QString &typeId) const
{
    return m_descriptors.value(typeId);
}

QVariantMap TypeRegistry::defaultComponentProperties(const QString &typeId) const
{
    return m_defaults.value(typeId);
}

QStringList TypeRegistry::componentTypeIds() const
{
    return m_orderedTypeIds;
}

QVariantList TypeRegistry::componentTypeDescriptors() const
{
    QVariantList out;
    out.reserve(m_orderedTypeIds.size());
    for (const QString &id : m_orderedTypeIds)
        out.append(m_descriptors.value(id));
    return out;
}

// ---------------------------------------------------------------------------
// Connection-policy queries
// ---------------------------------------------------------------------------

bool TypeRegistry::canConnect(const QString &srcTypeId,
                              const QString &tgtTypeId,
                              const QVariantMap &context,
                              QString *reason) const
{
    for (const IConnectionPolicyProvider *provider : m_connectionPolicies) {
        if (!provider)
            continue;
        QString providerReason;
        if (!provider->canConnect(srcTypeId, tgtTypeId, context, &providerReason)) {
            if (reason)
                *reason = providerReason;
            return false;
        }
    }
    return true;
}

QVariantMap TypeRegistry::normalizeConnectionProperties(const QString &srcTypeId,
                                                        const QString &tgtTypeId,
                                                        const QVariantMap &rawProps) const
{
    QVariantMap result = rawProps;
    for (const IConnectionPolicyProvider *provider : m_connectionPolicies) {
        if (!provider)
            continue;
        const QVariantMap normalized =
            provider->normalizeConnectionProperties(srcTypeId, tgtTypeId, result);
        // Merge: later providers overwrite keys from earlier providers.
        for (auto it = normalized.constBegin(); it != normalized.constEnd(); ++it)
            result.insert(it.key(), it.value());
    }
    return result;
}
