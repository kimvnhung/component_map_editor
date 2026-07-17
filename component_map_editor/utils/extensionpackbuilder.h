#ifndef EXTENSIONPACKBUILDER_H
#define EXTENSIONPACKBUILDER_H

#include "utils/common.h"

#define DEFAULT_EXTENSION_ID QStringLiteral("built.extension.pack")
#define DEFAULT_EXTENSION_NAME QStringLiteral("Built Extension Pack")
#define DEFAULT_EXTENSION_VERSION QStringLiteral("1.0.0")
#define DEFAULT_MIN_CORE_API_MAJOR 1
#define DEFAULT_MIN_CORE_API_MINOR 0
#define DEFAULT_MIN_CORE_API_PATCH 0
#define DEFAULT_MAX_CORE_API_MAJOR 1
#define DEFAULT_MAX_CORE_API_MINOR 99
#define DEFAULT_MAX_CORE_API_PATCH 99
#define DEFAULT_CAPABILITIES extensions::Capability_All

// Builder that produces an ExtensionStartupLoader::PackFactory which
// constructs an IExtensionPack that registers the provided provider instances.
class ExtensionPackBuilder
{
public:
    ExtensionPackBuilder();
    ~ExtensionPackBuilder();

    ExtensionPackBuilder &withComponentProviderFactory(ComponentFactory f);
    ExtensionPackBuilder &withPropertySchemaProviderFactory(PropertySchemaFactory f);
    ExtensionPackBuilder &withExecutionSemanticsFactory(ExecutionSemanticsFactory f);
    ExtensionPackBuilder &withConnectionPolicyProviderFactory(ConnectionPolicyFactory f);
    ExtensionPackBuilder &withValidationProviderFactory(ValidationFactory f);
    ExtensionPackBuilder &withActionProviderFactory(ActionFactory f);

    // Setup manifest, this must match with the manifest in the extension's manifest.json file
    ExtensionPackBuilder &withManifest(const ExtensionManifest &manifest);
    /**
     * @brief withExtensionId
     * @param id : this should match the extensionId in the extension's manifest.json file
     * @return
     */
    ExtensionPackBuilder &withExtensionId(const QString &id = DEFAULT_EXTENSION_ID);
    ExtensionPackBuilder &withDisplayName(const QString &name = DEFAULT_EXTENSION_NAME);
    ExtensionPackBuilder &withExtensionVersion(const QString &version = DEFAULT_EXTENSION_VERSION);
    ExtensionPackBuilder &withMinCoreApi(int major = DEFAULT_MIN_CORE_API_MAJOR, int minor = DEFAULT_MIN_CORE_API_MINOR,
                                         int patch = DEFAULT_MIN_CORE_API_PATCH);
    ExtensionPackBuilder &withMaxCoreApi(int major = DEFAULT_MAX_CORE_API_MAJOR, int minor = DEFAULT_MAX_CORE_API_MINOR,
                                         int patch = DEFAULT_MAX_CORE_API_PATCH);
    ExtensionPackBuilder &withCapabilities(const extensions::Capability &capabilities = DEFAULT_CAPABILITIES);

    // Returns a PackFactory compatible with ExtensionStartupLoader::registerFactory
    std::unique_ptr<PackFactoryEntry> build() const;

private:
    class BuiltExtensionPack;
    std::unique_ptr<BuiltExtensionPack> m_builtPack{nullptr};
};

#endif // EXTENSIONPACKBUILDER_H
