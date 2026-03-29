#include "ConnectionPolicyProviderV1ToV2Adapter.h"

#include "adapters/PolicyAdapter.h"

ConnectionPolicyProviderV1ToV2Adapter::ConnectionPolicyProviderV1ToV2Adapter(
    const IConnectionPolicyProvider *legacyProvider)
    : m_legacyProvider(legacyProvider)
{
}

QString ConnectionPolicyProviderV1ToV2Adapter::providerId() const
{
    return m_legacyProvider ? m_legacyProvider->providerId() : QString();
}

bool ConnectionPolicyProviderV1ToV2Adapter::canConnect(const cme::ConnectionPolicyContext &context,
                                                       QString *reason) const
{
    if (!m_legacyProvider) {
        if (reason)
            *reason = QStringLiteral("Legacy connection policy provider is null");
        return false;
    }

    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());
    const QVariantMap contextMap = cme::adapter::connectionPolicyContextToVariantMap(context);
    return m_legacyProvider->canConnect(sourceTypeId, targetTypeId, contextMap, reason);
}

QVariantMap ConnectionPolicyProviderV1ToV2Adapter::normalizeConnectionProperties(
    const cme::ConnectionPolicyContext &context,
    const QVariantMap &rawProperties) const
{
    if (!m_legacyProvider)
        return rawProperties;

    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());
    return m_legacyProvider->normalizeConnectionProperties(sourceTypeId, targetTypeId, rawProperties);
}
