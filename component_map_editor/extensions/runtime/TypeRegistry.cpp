#include "TypeRegistry.h"

#include "adapters/PolicyAdapter.h"

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
    cme::ConnectionPolicyContext typedContext;
    const cme::adapter::ConversionError conversionErr =
        cme::adapter::variantMapToConnectionPolicyContext(context, typedContext);
    if (conversionErr.has_error) {
        if (reason)
            *reason = conversionErr.error_message;
        return false;
    }

    if (typedContext.source_type_id().empty())
        typedContext.set_source_type_id(srcTypeId.toStdString());
    if (typedContext.target_type_id().empty())
        typedContext.set_target_type_id(tgtTypeId.toStdString());

    return canConnect(typedContext, reason);
}

bool TypeRegistry::canConnect(const cme::ConnectionPolicyContext &context,
                              QString *reason) const
{
    for (const IConnectionPolicyProvider *provider : m_connectionPolicies) {
        if (!provider)
            continue;
        QString providerReason;
        if (!provider->canConnect(context, &providerReason)) {
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
    cme::ConnectionPolicyContext context;
    context.set_source_type_id(srcTypeId.toStdString());
    context.set_target_type_id(tgtTypeId.toStdString());
    return normalizeConnectionProperties(context, rawProps);
}

QVariantMap TypeRegistry::normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                                        const QVariantMap &rawProps) const
{
    QVariantMap result = rawProps;
    for (const IConnectionPolicyProvider *provider : m_connectionPolicies) {
        if (!provider)
            continue;
        const QVariantMap normalized = provider->normalizeConnectionProperties(context, result);
        // Merge: later providers overwrite keys from earlier providers.
        for (auto it = normalized.constBegin(); it != normalized.constEnd(); ++it)
            result.insert(it.key(), it.value());
    }
    return result;
}
