#include "ExtensionContractRegistry.h"

ExtensionContractRegistry::ExtensionContractRegistry(const ExtensionApiVersion &coreApiVersion)
    : m_coreApiVersion(coreApiVersion)
{
}

ExtensionApiVersion ExtensionContractRegistry::coreApiVersion() const
{
    return m_coreApiVersion;
}

bool ExtensionContractRegistry::registerManifest(const ExtensionManifest &manifest, QString *error)
{
    QString validationError;
    if (!manifest.isValid(&validationError)) {
        if (error) {
            *error = validationError;
        }
        return false;
    }

    const ExtensionCompatibilityReport report = evaluateCompatibility(
        m_coreApiVersion, manifest.minCoreApi, manifest.maxCoreApi);
    if (!report.compatible()) {
        if (error) {
            *error = report.message;
        }
        return false;
    }

    if (m_manifests.contains(manifest.extensionId)) {
        if (error) {
            *error = QStringLiteral("Duplicate manifest extensionId: %1").arg(manifest.extensionId);
        }
        return false;
    }

    m_manifests.insert(manifest.extensionId, manifest);
    return true;
}

bool ExtensionContractRegistry::registerComponentTypeProvider(const IComponentTypeProvider *provider, QString *error)
{
    return registerProviderInternal(provider, &m_componentTypeProviders,
                                    QStringLiteral("component type"), error);
}

bool ExtensionContractRegistry::registerConnectionPolicyProvider(const IConnectionPolicyProvider *provider, QString *error)
{
    return registerProviderInternal(provider, &m_connectionPolicyProviders,
                                    QStringLiteral("connection policy"), error);
}

bool ExtensionContractRegistry::registerPropertySchemaProvider(const IPropertySchemaProvider *provider, QString *error)
{
    return registerProviderInternal(provider, &m_propertySchemaProviders,
                                    QStringLiteral("property schema"), error);
}

bool ExtensionContractRegistry::registerValidationProvider(const IValidationProvider *provider, QString *error)
{
    return registerProviderInternal(provider, &m_validationProviders,
                                    QStringLiteral("validation"), error);
}

bool ExtensionContractRegistry::registerActionProvider(const IActionProvider *provider, QString *error)
{
    return registerProviderInternal(provider, &m_actionProviders,
                                    QStringLiteral("action"), error);
}

bool ExtensionContractRegistry::hasManifest(const QString &extensionId) const
{
    return m_manifests.contains(extensionId);
}

ExtensionManifest ExtensionContractRegistry::manifest(const QString &extensionId) const
{
    return m_manifests.value(extensionId);
}

QList<const IComponentTypeProvider *> ExtensionContractRegistry::componentTypeProviders() const
{
    return m_componentTypeProviders.values();
}

QList<const IConnectionPolicyProvider *> ExtensionContractRegistry::connectionPolicyProviders() const
{
    return m_connectionPolicyProviders.values();
}

QList<const IPropertySchemaProvider *> ExtensionContractRegistry::propertySchemaProviders() const
{
    return m_propertySchemaProviders.values();
}
