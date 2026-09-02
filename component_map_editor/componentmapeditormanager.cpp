#include "componentmapeditormanager.h"


#include <QDir>
#include <base_log.h>
#include "extensions/contracts/ExtensionContractRegistry.h"

#define DEFAULT_EXTENSION_MANIFEST_DIR ""
#define DEFAULT_EXTENSION_RULE_FILE ""

ComponentMapEditorManager::ComponentMapEditorManager(QObject *parent)
    : QObject(parent)
    , m_extensionStartupLoader(new ExtensionStartupLoader())
    , m_componentTypeRegistry(new TypeRegistry())
    , m_propertySchemaRegistry(new PropertySchemaRegistry())
    , m_executionSandbox(new GraphExecutionSandbox())
{
}

ComponentMapEditorManager::~ComponentMapEditorManager()
{
    delete m_ruleHotReloadService;
    m_ruleHotReloadService = nullptr;

    delete m_extensionStartupLoader;
    delete m_componentTypeRegistry;
    delete m_propertySchemaRegistry;
    delete m_executionSandbox;

    m_extensionStartupLoader = nullptr;
    m_componentTypeRegistry = nullptr;
    m_propertySchemaRegistry = nullptr;
    m_executionSandbox = nullptr;
}

void ComponentMapEditorManager::reloadComponentTypes()
{
    LOGD("");
    m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
}

void ComponentMapEditorManager::setExtensionContractRegistry(std::unique_ptr<ExtensionContractRegistry> registry)
{
    if (!registry)
    {
        LOGW("Attempted to set a null extension contract registry; ignoring.");
        return;
    }

    m_extensionContracts = std::move(registry);

    // With ruleruntime
    if (!m_ruleHotReloadService)
    {
        m_ruleHotReloadService = new RuleHotReloadService(&ruleRegistry);
    }

    QString providerError;

    if (!m_extensionContracts->registerConnectionPolicyProvider(&compiledConnectionPolicy, &providerError))
    {
        LOGWF("[Rules][ERROR] Failed to register compiled connection policy provider: {}",
              providerError.toStdString());
    }

    if (!m_extensionContracts->registerValidationProvider(&compiledValidation, &providerError))
    {
        LOGWF("[Rules][ERROR] Failed to register compiled validation provider: {}",
              providerError.toStdString());
    }

    if (!m_ruleHotReloadService->startWatchingFile(m_ruleFilePath))
    {
        const QVector<RuleDiagnostic> diagnostics = m_ruleHotReloadService->lastDiagnostics();

        if (!diagnostics.isEmpty())
        {
            const RuleDiagnostic first = diagnostics.first();
            WARNF("[Rules][ERROR] {} file={} jsonPath={} line={} column={}",
                  first.message.toStdString(),
                  first.location.filePath.toStdString(),
                  first.location.jsonPath.toStdString(),
                  first.location.line,
                  first.location.column);
        }
    }


    // Load extensions from the manifest directory
    if (!m_extensionStartupLoader)
    {
        LOGW("Extension startup loader is not initialized; cannot load extensions.");
        return;
    }

    if (m_extensionStartupLoader->registeredFactoryCount() == 0)
    {
        LOGI("No extension pack factories are registered; no extension types will be loaded.");
        return;
    }

    const ExtensionLoadResult result = m_extensionStartupLoader->loadFromDirectory(m_manifestDir, *m_extensionContracts);

    for (const ExtensionLoadDiagnostic &diag : result.diagnostics)
    {
        if (diag.severity == ExtensionLoadDiagnostic::Severity::Error)
        {
            LOGWF("[ExtensionStartupLoader][ERROR] {} extensionId={} manifest={}",
                  diag.message.toStdString(),
                  diag.extensionId.toStdString(),
                  diag.manifestPath.toStdString());
        }
        else
        {
            LOGIF("[ExtensionStartupLoader] {} extensionId={} manifest={}",
                  diag.message.toStdString(),
                  diag.extensionId.toStdString(),
                  diag.manifestPath.toStdString());
        }
    }

    // Rebuild registries to reflect the new contract registry.
    m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
    m_propertySchemaRegistry->rebuildFromRegistry(*m_extensionContracts);
    m_executionSandbox->rebuildSemanticsFromRegistry(*m_extensionContracts);


}

void ComponentMapEditorManager::setRuleFilePath(const QString &filePath)
{
    if (filePath.isEmpty())
    {
        LOGW("Attempted to set an empty rule file path; ignoring.");
        return;
    }

    m_ruleFilePath = filePath;

    if (!m_ruleHotReloadService)
    {
        m_ruleHotReloadService = new RuleHotReloadService(&ruleRegistry);
    }

    if (!m_ruleHotReloadService->startWatchingFile(m_ruleFilePath))
    {
        const QVector<RuleDiagnostic> diagnostics = m_ruleHotReloadService->lastDiagnostics();

        if (!diagnostics.isEmpty())
        {
            const RuleDiagnostic first = diagnostics.first();
            WARNF("[Rules][ERROR] {} file={} jsonPath={} line={} column={}",
                  first.message.toStdString(),
                  first.location.filePath.toStdString(),
                  first.location.jsonPath.toStdString(),
                  first.location.line,
                  first.location.column);
        }

    }
}

void ComponentMapEditorManager::registerPackFactoryEntry(PackFactoryEntry entry)
{
    if (!m_extensionStartupLoader)
    {
        LOGW("Attempted to register a pack factory entry, but the extension startup loader is not initialized; ignoring.");
        return;
    }

    m_extensionStartupLoader->registerFactory(entry);
}

void ComponentMapEditorManager::setManifestDirectory(const QString &dirPath)
{
    if (dirPath.isEmpty())
    {
        LOGW("Attempted to set an empty manifest directory path; ignoring.");
        return;
    }

    m_manifestDir = dirPath;

    if (m_extensionContracts)
    {
        const ExtensionLoadResult result = m_extensionStartupLoader->loadFromDirectory(m_manifestDir, *m_extensionContracts);

        for (const ExtensionLoadDiagnostic &diag : result.diagnostics)
        {
            if (diag.severity == ExtensionLoadDiagnostic::Severity::Error)
            {
                LOGWF("[ExtensionStartupLoader][ERROR] {} extensionId={} manifest={}",
                      diag.message.toStdString(),
                      diag.extensionId.toStdString(),
                      diag.manifestPath.toStdString());
            }
            else
            {
                LOGIF("[ExtensionStartupLoader] {} extensionId={} manifest={}",
                      diag.message.toStdString(),
                      diag.extensionId.toStdString(),
                      diag.manifestPath.toStdString());
            }
        }

        // Rebuild registries to reflect the new contract registry.
        m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
        m_propertySchemaRegistry->rebuildFromRegistry(*m_extensionContracts);
        m_executionSandbox->rebuildSemanticsFromRegistry(*m_extensionContracts);
    }
    else
    {
        LOGI("Manifest directory set, but extension contract registry is not yet set. Extension types will not be loaded until the registry is provided.");
    }
}

ComponentMapEditorManagerBuilder::ComponentMapEditorManagerBuilder()
    : m_manager(std::make_unique<ComponentMapEditorManager>())
{
}

ComponentMapEditorManagerBuilder::~ComponentMapEditorManagerBuilder() = default;

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withExtensionContractRegistry(
    std::unique_ptr<ExtensionContractRegistry> registry)
{
    if (m_extensionContracts != nullptr)
    {
        throw std::runtime_error("Extension contract registry has already been set; cannot set it again.");
    }

    m_extensionContracts = std::move(registry);
    return *this;
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withRuleFilePath(const QString &filePath)
{
    if (!m_ruleFilePath.isEmpty())
    {
        throw std::runtime_error("Rule file path has already been set; cannot set it again.");
    }

    m_ruleFilePath = filePath;
    return *this;
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withManifestDirectory(const QString &dirPath)
{
    if (!m_manifestDir.isEmpty())
    {
        throw std::runtime_error("Manifest directory has already been set; cannot set it again.");
    }

    m_manifestDir = dirPath;
    return *this;
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withPackFactoryEntry(
    std::unique_ptr<PackFactoryEntry> entry)
{
    m_packFactoryEntries.push_back(std::move(entry));
    return *this;
}

std::unique_ptr<ComponentMapEditorManager> ComponentMapEditorManagerBuilder::build()
{
    // Verify components
    std::string err;

    if (!m_extensionContracts)
    {
        throw ExtensionContractRegistryMissingException();
    }

    if (m_ruleFilePath.isEmpty())
    {
        // Get system path from DEFAULT_EXTENSION_RULE_FILE
        m_ruleFilePath = QDir::currentPath() + "/" + DEFAULT_EXTENSION_RULE_FILE;
        LOGI("No rule file path provided; using default: {}", m_ruleFilePath.toStdString());
    }

    if (m_manifestDir.isEmpty())
    {
        // Get system path from DEFAULT_EXTENSION_MANIFEST_DIR
        m_manifestDir = QDir::currentPath() + "/" + DEFAULT_EXTENSION_MANIFEST_DIR;
        LOGI("No manifest directory provided; using default: {}", m_manifestDir.toStdString());
    }

    m_manager->setRuleFilePath(m_ruleFilePath);
    m_manager->setManifestDirectory(m_manifestDir);

    if (!m_packFactoryEntries.empty())
    {
        for (const auto &entry : m_packFactoryEntries)
        {
            m_manager->registerPackFactoryEntry(*entry);
        }
    }

    m_manager->setExtensionContractRegistry(std::move(m_extensionContracts));



    return std::move(m_manager);
}
