#include "componentmapeditormanager.h"

#ifndef EXAMPLE_EXTENSION_RULE_FILE
    #define EXAMPLE_EXTENSION_RULE_FILE ""
#endif

#ifndef EXAMPLE_EXTENSION_MANIFEST_DIR
    #define EXAMPLE_EXTENSION_MANIFEST_DIR ""
#endif

ComponentMapEditorManager::ComponentMapEditorManager(ExtensionStartupLoader::PackFactory extensionPackFactory)
    : QObject{nullptr}
    , m_extensionContracts(new ExtensionContractRegistry({1, 0, 0}))
, m_extensionStartupLoader(new ExtensionStartupLoader())
, m_ruleRegistry(new RuleRuntimeRegistry())
, m_connectionPolicyProvider(new RuleBackedConnectionPolicyProvider(m_ruleRegistry))
, m_validationProvider(new RuleBackedValidationProvider(m_ruleRegistry))
, m_ruleHotReloadService(new RuleHotReloadService(m_ruleRegistry))
, m_componentTypeRegistry(new TypeRegistry())
, m_propertySchemaRegistry(new PropertySchemaRegistry())
, m_executionSandbox(new GraphExecutionSandbox())
{
    // 1. Set up the extension contract registry and register any built-in providers.
    QString providerError;
    m_extensionContracts->registerConnectionPolicyProvider(m_connectionPolicyProvider, &providerError);
    m_extensionContracts->registerValidationProvider(m_validationProvider, &providerError);

    // 2. Start watching the rule file – changes are picked up without restart.
    const QString ruleFilePath = QString::fromUtf8(EXAMPLE_EXTENSION_RULE_FILE);
    m_ruleHotReloadService->startWatchingFile(ruleFilePath);

    // 3. Discover and load extension packs from the manifest directory.
    const QString manifestDir = QString::fromUtf8(EXAMPLE_EXTENSION_MANIFEST_DIR);
    m_extensionStartupLoader->registerFactory("customize.workflow", extensionPackFactory);
    m_extensionStartupLoader->loadFromDirectory(manifestDir, *m_extensionContracts);
}

std::unique_ptr<ComponentMapEditorManager> ComponentMapEditorBuilder::build(const QString& extensionId,
        ExtensionStartupLoader::PackFactory extensionPackFactory)
{
    auto manager = std::make_unique<ComponentMapEditorManager>(extensionPackFactory);

    Q_UNUSED(extensionId);
    return manager;
}