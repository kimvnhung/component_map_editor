#ifndef EXTENSIONCONTRACTREGISTRY_H
#define EXTENSIONCONTRACTREGISTRY_H

#include <QHash>
#include <QList>
#include <QString>
#include <memory>
#include <vector>

#include "ExtensionApiVersion.h"
#include "ConnectionPolicyProviderV1ToV2Adapter.h"
#include "ExecutionSemanticsV0Adapter.h"
#include "ExtensionManifest.h"
#include "IActionProvider.h"
#include "IComponentTypeProvider.h"
#include "IConnectionPolicyProvider.h"
#include "IConnectionPolicyProviderV2.h"
#include "IExecutionSemanticsProvider.h"
#include "IExecutionSemanticsProviderV0.h"
#include "IPropertySchemaProvider.h"
#include "IValidationProvider.h"

// Holds providers in insertion order while also providing O(1) duplicate-ID checks.
template <typename T>
struct ProviderRegistry {
    QHash<QString, const T *> index; // keyed by provider ID for O(1) duplicate-ID checks during registration
    QList<const T *>          order; // preserves registration order for deterministic iteration
};

class ExtensionContractRegistry
{
public:
    explicit ExtensionContractRegistry(const ExtensionApiVersion &coreApiVersion);

    ExtensionApiVersion coreApiVersion() const;

    bool registerManifest(const ExtensionManifest &manifest, QString *error = nullptr);
    bool registerComponentTypeProvider(const IComponentTypeProvider *provider, QString *error = nullptr);
    bool registerConnectionPolicyProvider(const IConnectionPolicyProvider *provider, QString *error = nullptr);
    bool registerConnectionPolicyProvider(const IConnectionPolicyProviderV2 *provider, QString *error = nullptr);
    bool registerPropertySchemaProvider(const IPropertySchemaProvider *provider, QString *error = nullptr);
    bool registerValidationProvider(const IValidationProvider *provider, QString *error = nullptr);
    bool registerActionProvider(const IActionProvider *provider, QString *error = nullptr);
    bool registerExecutionSemanticsProvider(const IExecutionSemanticsProvider *provider,
                                            QString *error = nullptr);
    bool registerExecutionSemanticsProvider(const IExecutionSemanticsProviderV0 *provider,
                                            QString *error = nullptr);

    bool hasManifest(const QString &extensionId) const;
    ExtensionManifest manifest(const QString &extensionId) const;

    // Provider accessors used by TypeRegistry to build its O(1) cache.
    // Providers are returned in registration order.
    QList<const IComponentTypeProvider *> componentTypeProviders() const;
    QList<const IConnectionPolicyProvider *> connectionPolicyProviders() const;
    QList<const IConnectionPolicyProviderV2 *> connectionPolicyProvidersV2() const;
    QList<const IPropertySchemaProvider *> propertySchemaProviders() const;
    QList<const IValidationProvider *> validationProviders() const;
    QList<const IExecutionSemanticsProvider *> executionSemanticsProviders() const;

private:
    // Registers a provider into both a hash index (for O(1) duplicate detection) and
    // an ordered list (to preserve registration order for iteration).
    template <typename ProviderT>
    bool registerProviderInternal(const ProviderT *provider,
                                  ProviderRegistry<ProviderT> *target,
                                  const QString &providerType,
                                  QString *error)
    {
        if (!provider) {
            if (error) {
                *error = QStringLiteral("%1 provider pointer is null.").arg(providerType);
            }
            return false;
        }

        const QString id = provider->providerId().trimmed();
        if (id.isEmpty()) {
            if (error) {
                *error = QStringLiteral("%1 provider id is empty.").arg(providerType);
            }
            return false;
        }

        if (target->index.contains(id)) {
            if (error) {
                *error = QStringLiteral("Duplicate %1 provider id: %2").arg(providerType, id);
            }
            return false;
        }

        target->index.insert(id, provider);
        target->order.append(provider);
        return true;
    }

    ExtensionApiVersion m_coreApiVersion;
    QHash<QString, ExtensionManifest> m_manifests;
    ProviderRegistry<IComponentTypeProvider>     m_componentTypeProviders;
    ProviderRegistry<IConnectionPolicyProvider>  m_connectionPolicyProviders;
    ProviderRegistry<IConnectionPolicyProviderV2> m_connectionPolicyProvidersV2;
    ProviderRegistry<IPropertySchemaProvider>    m_propertySchemaProviders;
    ProviderRegistry<IValidationProvider>        m_validationProviders;
    ProviderRegistry<IActionProvider>            m_actionProviders;
    ProviderRegistry<IExecutionSemanticsProvider> m_executionSemanticsProviders;
    std::vector<std::unique_ptr<ConnectionPolicyProviderV1ToV2Adapter>> m_connectionPolicyV1Adapters;
    std::vector<std::unique_ptr<ExecutionSemanticsV0Adapter>> m_executionSemanticsV0Adapters;
};

#endif // EXTENSIONCONTRACTREGISTRY_H
