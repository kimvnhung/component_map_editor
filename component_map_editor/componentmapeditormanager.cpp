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

ComponentMapEditorManager::ComponentMapEditorManager(
    const ExtensionApiVersion &coreApiVersion,
    PackFactory extensionPackFactory,
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
    , m_propertySchemaRegistry(new PropertySchemaRegistry())
    , m_executionSandbox(new GraphExecutionSandbox())
    , m_manifestDir(manifestDir)
{
    // 1. Set up the extension contract registry and register any built-in providers.
    QString providerError;

    if (!m_extensionContracts->registerConnectionPolicyProvider(m_connectionPolicyProvider, &providerError))
    {
        LOGWF("[Rules][ERROR] Failed to register compiled connection policy provider: {}",
              providerError.toStdString());
    }

    if (!m_extensionContracts->registerValidationProvider(m_validationProvider, &providerError))
    {
        LOGWF("[Rules][ERROR] Failed to register compiled validation provider: {}",
              providerError.toStdString());
    }

    // 2. Start watching the rule file – changes are picked up without restart.
    if (!m_ruleHotReloadService->startWatchingFile(ruleFilePath))
    {
        const QVector<RuleDiagnostic> diagnostics = m_ruleHotReloadService->lastDiagnostics();

        if (!diagnostics.isEmpty())
        {
            const RuleDiagnostic first = diagnostics.first();
            LOGWF("[Rules][ERROR] {} file={} jsonPath={} line={} column={}",
                  first.message.toStdString(),
                  first.location.filePath.toStdString(),
                  first.location.jsonPath.toStdString(),
                  first.location.line,
                  first.location.column);
        }
    }

    // 3. Discover and load extension packs from the manifest directory.
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
    reloadExtensionPacks();
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
    m_extensionStartupLoader->loadFromDirectory(m_manifestDir, *m_extensionContracts);

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
    if (m_manager)
    {
        delete m_manager;
    }

    m_manager = new ComponentMapEditorManager(m_coreVersion,
        m_packFactory ? m_packFactory :
        (m_extensionPackBuilder ? m_extensionPackBuilder->build() : nullptr),
        m_ruleFilePath, m_manifestDir);

    return m_manager;
}
