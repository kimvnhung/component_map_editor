#ifndef EXTENSIONCONTRACTREGISTRY_H
#define EXTENSIONCONTRACTREGISTRY_H

#include <QHash>
#include <QString>

#include "ExtensionApiVersion.h"
#include "ExtensionManifest.h"
#include "IActionProvider.h"
#include "IConnectionPolicyProvider.h"
#include "INodeTypeProvider.h"
#include "IPropertySchemaProvider.h"
#include "IValidationProvider.h"

class ExtensionContractRegistry
{
public:
    explicit ExtensionContractRegistry(const ExtensionApiVersion &coreApiVersion);

    ExtensionApiVersion coreApiVersion() const;

    bool registerManifest(const ExtensionManifest &manifest, QString *error = nullptr);
    bool registerNodeTypeProvider(const INodeTypeProvider *provider, QString *error = nullptr);
    bool registerConnectionPolicyProvider(const IConnectionPolicyProvider *provider, QString *error = nullptr);
    bool registerPropertySchemaProvider(const IPropertySchemaProvider *provider, QString *error = nullptr);
    bool registerValidationProvider(const IValidationProvider *provider, QString *error = nullptr);
    bool registerActionProvider(const IActionProvider *provider, QString *error = nullptr);

    bool hasManifest(const QString &extensionId) const;
    ExtensionManifest manifest(const QString &extensionId) const;

private:
    template <typename ProviderT>
    bool registerProviderInternal(const ProviderT *provider,
                                  QHash<QString, const ProviderT *> *target,
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

        if (target->contains(id)) {
            if (error) {
                *error = QStringLiteral("Duplicate %1 provider id: %2").arg(providerType, id);
            }
            return false;
        }

        target->insert(id, provider);
        return true;
    }

    ExtensionApiVersion m_coreApiVersion;
    QHash<QString, ExtensionManifest> m_manifests;
    QHash<QString, const INodeTypeProvider *> m_nodeTypeProviders;
    QHash<QString, const IConnectionPolicyProvider *> m_connectionPolicyProviders;
    QHash<QString, const IPropertySchemaProvider *> m_propertySchemaProviders;
    QHash<QString, const IValidationProvider *> m_validationProviders;
    QHash<QString, const IActionProvider *> m_actionProviders;
};

#endif // EXTENSIONCONTRACTREGISTRY_H
