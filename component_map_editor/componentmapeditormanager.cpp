#include "componentmapeditormanager.h"

#include "extensionpackbuilder.h"

#include <base_log.h>
#include "extensions/sample_pack/SampleExtensionPack.h"

#ifndef EXAMPLE_EXTENSION_RULE_FILE
    #define EXAMPLE_EXTENSION_RULE_FILE ""
#endif

#ifndef EXAMPLE_EXTENSION_MANIFEST_DIR
    #define EXAMPLE_EXTENSION_MANIFEST_DIR ""
#endif

ComponentMapEditorManager::ComponentMapEditorManager(PackFactory extensionPackFactory)
    : ComponentMapEditorManager(extensionPackFactory, ExtensionApiVersion{1, 0, 0}, nullptr, nullptr, nullptr,
                                QString::fromUtf8(EXAMPLE_EXTENSION_RULE_FILE), QString::fromUtf8(EXAMPLE_EXTENSION_MANIFEST_DIR))
{
    reloadExtensionPacks();
}

ComponentMapEditorManager::ComponentMapEditorManager(PackFactory extensionPackFactory,
        const ExtensionApiVersion &coreApiVersion,
        const IComponentTypeProvider *customComponentProvider,
        PropertySchemaRegistry *customPropertySchemaRegistry,
        GraphExecutionSandbox *customExecutionSandbox,
        const QString &ruleFilePath,
        const QString &manifestDir)
    : QObject{nullptr}
    , m_extensionContracts(new ExtensionContractRegistry(coreApiVersion))
    , m_extensionStartupLoader(new ExtensionStartupLoader())
    , m_ruleRegistry(new RuleRuntimeRegistry())
    , m_connectionPolicyProvider(new RuleBackedConnectionPolicyProvider(m_ruleRegistry))
    , m_validationProvider(new RuleBackedValidationProvider(m_ruleRegistry))
    , m_ruleHotReloadService(new RuleHotReloadService(m_ruleRegistry))
    , m_componentTypeRegistry(new TypeRegistry())
    , m_propertySchemaRegistry(customPropertySchemaRegistry ? customPropertySchemaRegistry : new PropertySchemaRegistry())
    , m_executionSandbox(customExecutionSandbox ? customExecutionSandbox : new GraphExecutionSandbox())
{
    // 1. Set up the extension contract registry and register any built-in providers.
    QString providerError;
    m_extensionContracts->registerConnectionPolicyProvider(m_connectionPolicyProvider, &providerError);
    m_extensionContracts->registerValidationProvider(m_validationProvider, &providerError);

    // If a custom component provider is supplied, register it so TypeRegistry picks it up.
    if (customComponentProvider)
    {
        m_extensionContracts->registerComponentTypeProvider(customComponentProvider, &providerError);
    }

    // 2. Start watching the rule file – changes are picked up without restart.
    m_ruleHotReloadService->startWatchingFile(ruleFilePath);

    // 3. Discover and load extension packs from the manifest directory.
    m_manifestDir = manifestDir;

    // If caller didn't provide a factory, fall back to the built-in sample pack and warn.
    if (!extensionPackFactory)
    {
        LOGW("No extension pack factory provided, falling back to SampleExtensionPack");
        extensionPackFactory = []() -> std::unique_ptr<IExtensionPack>
        {
            return std::make_unique<SampleExtensionPack>();
        };
    }

    m_extensionStartupLoader->registerFactory("customize.workflow", extensionPackFactory);
    const ExtensionLoadResult loadResult = m_extensionStartupLoader->loadFromDirectory(manifestDir, *m_extensionContracts);

    // If the loader returned auxiliary registries produced by builder factories,
    // adopt them (first non-null wins).
    if (loadResult.propertySchemaRegistry) {
        if (m_propertySchemaRegistry)
            delete m_propertySchemaRegistry;
        m_propertySchemaRegistry = loadResult.propertySchemaRegistry.release();
    }
    if (loadResult.executionSandbox) {
        if (m_executionSandbox)
            delete m_executionSandbox;
        m_executionSandbox = loadResult.executionSandbox.release();
    }

    // Rebuild registries to reflect newly-registered providers.
    m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
    m_propertySchemaRegistry->rebuildFromRegistry(*m_extensionContracts);
    m_executionSandbox->rebuildSemanticsFromRegistry(*m_extensionContracts);
}

void ComponentMapEditorManager::reloadComponentTypes()
{
    LOGD("");
    m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
}

void ComponentMapEditorManager::setExtensionPackFactory(PackFactory factory)
{
    if (!m_extensionStartupLoader)
    {
        return;
    }

    if (!factory)
    {
        LOGW("Attempted to set a null extension pack factory; ignoring.");
        return;
    }

    m_extensionStartupLoader->clearFactories();
    m_extensionStartupLoader->registerFactory("customize.workflow", factory);
    reloadExtensionPacks();
}

void ComponentMapEditorManager::reloadExtensionPacks()
{
    if (!m_extensionStartupLoader || !m_extensionContracts)
    {
        LOGW("Cannot reload extension packs because the startup loader or contract registry is not initialized.");
        return;
    }

    // Re-scan manifests using the same manifest directory used at construction.
    const ExtensionLoadResult loadResult = m_extensionStartupLoader->loadFromDirectory(m_manifestDir, *m_extensionContracts);

    // Adopt any auxiliary registries provided by the packs (if any).
    if (loadResult.propertySchemaRegistry) {
        if (m_propertySchemaRegistry)
            delete m_propertySchemaRegistry;
        m_propertySchemaRegistry = loadResult.propertySchemaRegistry.release();
    }
    if (loadResult.executionSandbox) {
        if (m_executionSandbox)
            delete m_executionSandbox;
        m_executionSandbox = loadResult.executionSandbox.release();
    }

    // Rebuild registries to reflect newly-registered providers.
    m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
    m_propertySchemaRegistry->rebuildFromRegistry(*m_extensionContracts);
    m_executionSandbox->rebuildSemanticsFromRegistry(*m_extensionContracts);
}

void ComponentMapEditorManager::setExtensionContractVersion(const ExtensionApiVersion &v)
{
    if (!m_extensionContracts)
    {
        return;
    }

    // Recreate the registry with the new version and reload manifests.
    m_extensionContracts = new ExtensionContractRegistry(v);
    reloadExtensionPacks();
}

void ComponentMapEditorManager::setRuleFilePath(const QString &path)
{
    if (m_ruleHotReloadService)
    {
        m_ruleHotReloadService->startWatchingFile(path);
    }
}

void ComponentMapEditorManager::setManifestDir(const QString &dir)
{
    m_manifestDir = dir;

    // Immediately reload packs from the new directory.
    reloadExtensionPacks();
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withExtensionPackFactory(
    PackFactory factory)
{
    m_packFactory = factory;
    return *this;
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withExtensionPackBuilder(
    const ExtensionPackBuilder &builder)
{
    // store a heap copy so builder lifetime extends until build()
    if (m_extensionPackBuilder)
    {
        delete m_extensionPackBuilder;
    }

    m_extensionPackBuilder = new ExtensionPackBuilder(builder);
    m_hasExtensionPackBuilder = true;
    return *this;
}

// (Custom providers are constructed via ExtensionPackBuilder; no-op here.)

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withExtensionContractVersion(
    const ExtensionApiVersion &v)
{
    m_coreVersion = v;
    return *this;
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withRuleFilePath(const QString &path)
{
    m_ruleFilePath = path;
    return *this;
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withManifestDir(const QString &dir)
{
    m_manifestDir = dir;
    return *this;
}

ComponentMapEditorManager *ComponentMapEditorManagerBuilder::build()
{
    if (!m_manager)
    {
        // If an ExtensionPackBuilder was provided, use it to create the pack factory
        // and produce runtime registries/sandbox to hand to the manager.
        PropertySchemaRegistry *propRegistry = nullptr;
        GraphExecutionSandbox *execSandbox = nullptr;

        if (m_hasExtensionPackBuilder && m_extensionPackBuilder)
        {
            // adopt the pack factory produced by the extension pack builder
            m_packFactory = m_extensionPackBuilder->build();

            // create instances using the builder and release ownership to manager
            auto propUp = m_extensionPackBuilder->createPropertySchemaRegistry();

            if (propUp) { propRegistry = propUp.release(); }

            auto execUp = m_extensionPackBuilder->createExecutionSandbox();

            if (execUp) { execSandbox = execUp.release(); }
        }

        m_manager = new ComponentMapEditorManager(m_packFactory,
            m_coreVersion,
            nullptr,
            propRegistry,
            execSandbox,
            m_ruleFilePath,
            m_manifestDir);
    }

    return m_manager;
}
