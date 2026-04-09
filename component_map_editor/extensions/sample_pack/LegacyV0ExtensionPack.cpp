#include "LegacyV0ExtensionPack.h"

LegacyV0ExtensionPack::LegacyV0ExtensionPack()
{
    m_manifest.extensionId = QString::fromLatin1(ExtensionId);
    m_manifest.displayName = QStringLiteral("Sample Workflow Pack (Legacy v0)");
    m_manifest.extensionVersion = QStringLiteral("0.9.0");
    m_manifest.minCoreApi = {1, 0, 0};
    m_manifest.maxCoreApi = {1, 99, 99};
    m_manifest.capabilities = { QStringLiteral("executionSemantics") };

    QVariantMap contractVersions;
    contractVersions.insert(QStringLiteral("executionSemantics"), 0);
    m_manifest.metadata.insert(QStringLiteral("contractVersions"), contractVersions);
}

const ExtensionManifest &LegacyV0ExtensionPack::manifest() const
{
    return m_manifest;
}

bool LegacyV0ExtensionPack::registerAll(ExtensionContractRegistry &registry, QString *error)
{
    if (!registry.registerManifest(m_manifest, error))
        return false;
    return registerProviders(registry, error);
}

bool LegacyV0ExtensionPack::registerProviders(ExtensionContractRegistry &registry, QString *error)
{
    // This is intentionally unchanged legacy call style: the provider type is v0,
    // and the registry overload injects an adapter transparently.
    return registry.registerExecutionSemanticsProvider(&m_executionProvider, error);
}
