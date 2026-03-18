#include "SampleExtensionPack.h"

SampleExtensionPack::SampleExtensionPack()
{
    m_manifest.extensionId      = QString::fromLatin1(ExtensionId);
    m_manifest.displayName      = QString::fromLatin1(DisplayName);
    m_manifest.extensionVersion = QString::fromLatin1(ExtensionVersion);
    m_manifest.minCoreApi       = { MinApiMajor, MinApiMinor, MinApiPatch };
    m_manifest.maxCoreApi       = { MaxApiMajor, MaxApiMinor, MaxApiPatch };
    m_manifest.capabilities     = { QStringLiteral("nodeTypes"),
                                    QStringLiteral("connectionPolicy"),
                                    QStringLiteral("propertySchema"),
                                    QStringLiteral("validation"),
                                    QStringLiteral("actions") };
}

const ExtensionManifest &SampleExtensionPack::manifest() const
{
    return m_manifest;
}

bool SampleExtensionPack::registerAll(ExtensionContractRegistry &registry, QString *error)
{
    if (!registry.registerManifest(m_manifest, error))
        return false;
    return registerProviders(registry, error);
}

bool SampleExtensionPack::registerProviders(ExtensionContractRegistry &registry, QString *error)
{
    if (!registry.registerNodeTypeProvider(&m_nodeTypeProvider, error))
        return false;
    if (!registry.registerConnectionPolicyProvider(&m_connectionPolicyProvider, error))
        return false;
    if (!registry.registerPropertySchemaProvider(&m_propertySchemaProvider, error))
        return false;
    if (!registry.registerValidationProvider(&m_validationProvider, error))
        return false;
    if (!registry.registerActionProvider(&m_actionProvider, error))
        return false;
    return true;
}
