#include "customizeextensionpack.h"

#include <extensions/contracts/ExtensionContractRegistry.h>

CustomizeExtensionPack::CustomizeExtensionPack()
{
    m_manifest.extensionId      = QString::fromLatin1(ExtensionId);
    m_manifest.displayName      = QString::fromLatin1(DisplayName);
    m_manifest.extensionVersion = QString::fromLatin1(ExtensionVersion);
    m_manifest.minCoreApi       = { MinApiMajor, MinApiMinor, MinApiPatch };
    m_manifest.maxCoreApi       = { MaxApiMajor, MaxApiMinor, MaxApiPatch };
    m_manifest.capabilities     = { QStringLiteral("componentTypes"),
                               QStringLiteral("connectionPolicy"),
                               QStringLiteral("propertySchema"),
                               QStringLiteral("validation"),
                               QStringLiteral("actions"),
                               QStringLiteral("executionSemantics") };
}

const ExtensionManifest &CustomizeExtensionPack::manifest() const
{
    return m_manifest;
}

bool CustomizeExtensionPack::registerAll(ExtensionContractRegistry &registry, QString *error)
{
    if (!registry.registerManifest(m_manifest, error))
        return false;
    return registerProviders(registry, error);
}

bool CustomizeExtensionPack::registerProviders(ExtensionContractRegistry &registry, QString *error)
{
    if (!registry.registerComponentTypeProvider(&m_componentTypeProvider, error))
        return false;
    if (!registry.registerConnectionPolicyProvider(&m_connectionPolicyProvider, error))
        return false;
    if (!registry.registerPropertySchemaProvider(&m_propertySchemaProvider, error))
        return false;
    if (!registry.registerValidationProvider(&m_workflowValidationProvider, error))
        return false;
    // if (!registry.registerActionProvider(&m_actionProvider, error))
    //     return false;
    // if (!registry.registerExecutionSemanticsProvider(&m_executionSemanticsProvider, error))
        // return false;
    return true;
}
