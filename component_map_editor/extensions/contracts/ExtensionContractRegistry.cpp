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
    if (!registerProviderInternal(provider, &m_validationProviders,
                                  QStringLiteral("validation"), error)) {
        return false;
    }
    return true;
}

bool ExtensionContractRegistry::registerActionProvider(const IActionProvider *provider, QString *error)
{
    return registerProviderInternal(provider, &m_actionProviders,
                                    QStringLiteral("action"), error);
}

bool ExtensionContractRegistry::registerExecutionSemanticsProvider(const IExecutionSemanticsProvider *provider,
                                                                   QString *error)
{
    return registerProviderInternal(provider, &m_executionSemanticsProviders,
                                    QStringLiteral("execution semantics"), error);
}

bool ExtensionContractRegistry::registerExecutionSemanticsProvider(const IExecutionSemanticsProviderV0 *provider,
                                                                   QString *error)
{
    if (!provider) {
        if (error) {
            *error = QStringLiteral("execution semantics provider pointer is null.");
        }
        return false;
    }

    std::unique_ptr<ExecutionSemanticsV0Adapter> adapter(
        new ExecutionSemanticsV0Adapter(provider));
    const QString id = adapter->providerId().trimmed();
    if (id.isEmpty()) {
        if (error) {
            *error = QStringLiteral("execution semantics provider id is empty.");
        }
        return false;
    }

    if (!registerProviderInternal(static_cast<const IExecutionSemanticsProvider *>(adapter.get()),
                                  &m_executionSemanticsProviders,
                                  QStringLiteral("execution semantics"),
                                  error)) {
        return false;
    }

    m_executionSemanticsV0Adapters.push_back(std::move(adapter));
    return true;
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
    return m_componentTypeProviders.order;
}

QList<const IConnectionPolicyProvider *> ExtensionContractRegistry::connectionPolicyProviders() const
{
    return m_connectionPolicyProviders.order;
}

QList<const IPropertySchemaProvider *> ExtensionContractRegistry::propertySchemaProviders() const
{
    return m_propertySchemaProviders.order;
}

QList<const IValidationProvider *> ExtensionContractRegistry::validationProviders() const
{
    return m_validationProviders.order;
}

QList<const IActionProvider *> ExtensionContractRegistry::actionProviders() const
{
    return m_actionProviders.order;
}

QList<const IExecutionSemanticsProvider *> ExtensionContractRegistry::executionSemanticsProviders() const
{
    return m_executionSemanticsProviders.order;
}
