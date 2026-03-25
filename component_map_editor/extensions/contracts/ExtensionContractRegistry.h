#ifndef EXTENSIONCONTRACTREGISTRY_H
#define EXTENSIONCONTRACTREGISTRY_H

#include <QHash>
#include <QList>
#include <QString>
#include <memory>
#include <vector>

#include "ExtensionApiVersion.h"
#include "ExecutionSemanticsV0Adapter.h"
#include "ExtensionManifest.h"
#include "IActionProvider.h"
#include "IComponentTypeProvider.h"
#include "IConnectionPolicyProvider.h"
#include "IExecutionSemanticsProvider.h"
#include "IExecutionSemanticsProviderV0.h"
#include "IPropertySchemaProvider.h"
#include "IValidationProvider.h"

class ExtensionContractRegistry
{
public:
    explicit ExtensionContractRegistry(const ExtensionApiVersion &coreApiVersion);

    ExtensionApiVersion coreApiVersion() const;

    bool registerManifest(const ExtensionManifest &manifest, QString *error = nullptr);
    bool registerComponentTypeProvider(const IComponentTypeProvider *provider, QString *error = nullptr);
    bool registerConnectionPolicyProvider(const IConnectionPolicyProvider *provider, QString *error = nullptr);
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
    QList<const IPropertySchemaProvider *> propertySchemaProviders() const;
    QList<const IExecutionSemanticsProvider *> executionSemanticsProviders() const;

private:
    // Registers a provider into both a hash index (for O(1) duplicate detection) and
    // an ordered list (to preserve registration order for iteration).
    template <typename ProviderT>
    bool registerProviderInternal(const ProviderT *provider,
                                  QHash<QString, const ProviderT *> *index,
                                  QList<const ProviderT *> *ordered,
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

        if (index->contains(id)) {
            if (error) {
                *error = QStringLiteral("Duplicate %1 provider id: %2").arg(providerType, id);
            }
            return false;
        }

        index->insert(id, provider);
        ordered->append(provider);
        return true;
    }

    ExtensionApiVersion m_coreApiVersion;
    QHash<QString, ExtensionManifest> m_manifests;
    QHash<QString, const IComponentTypeProvider *> m_componentTypeProviders;
    QList<const IComponentTypeProvider *> m_orderedComponentTypeProviders;
    QHash<QString, const IConnectionPolicyProvider *> m_connectionPolicyProviders;
    QList<const IConnectionPolicyProvider *> m_orderedConnectionPolicyProviders;
    QHash<QString, const IPropertySchemaProvider *> m_propertySchemaProviders;
    QList<const IPropertySchemaProvider *> m_orderedPropertySchemaProviders;
    QHash<QString, const IValidationProvider *> m_validationProviders;
    QList<const IValidationProvider *> m_orderedValidationProviders;
    QHash<QString, const IActionProvider *> m_actionProviders;
    QList<const IActionProvider *> m_orderedActionProviders;
    QHash<QString, const IExecutionSemanticsProvider *> m_executionSemanticsProviders;
    QList<const IExecutionSemanticsProvider *> m_orderedExecutionSemanticsProviders;
    std::vector<std::unique_ptr<ExecutionSemanticsV0Adapter>> m_executionSemanticsV0Adapters;
};

#endif // EXTENSIONCONTRACTREGISTRY_H
