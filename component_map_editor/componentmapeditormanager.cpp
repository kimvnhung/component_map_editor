#include "componentmapeditormanager.h"


#include <base_log.h>
#include "extensions/contracts/ExtensionContractRegistry.h"

#define DEFAULT_EXTENSION_MANIFEST_DIR ""
#define DEFAULT_EXTENSION_RULE_FILE ""

ComponentMapEditorManager::ComponentMapEditorManager(QObject *parent)
    : QObject(parent)
    , m_extensionContracts(new ExtensionContractRegistry({0, 0, 1}))
, m_extensionStartupLoader(new ExtensionStartupLoader())
, m_componentTypeRegistry(new TypeRegistry())
, m_propertySchemaRegistry(new PropertySchemaRegistry())
, m_executionSandbox(new GraphExecutionSandbox())
, m_validationService(new ValidationService())
{
}

ComponentMapEditorManager::~ComponentMapEditorManager()
{
    delete m_extensionContracts;
    delete m_extensionStartupLoader;
    delete m_componentTypeRegistry;
    delete m_propertySchemaRegistry;
    delete m_executionSandbox;
    delete m_validationService;

    m_extensionContracts = nullptr;
    m_extensionStartupLoader = nullptr;
    m_componentTypeRegistry = nullptr;
    m_propertySchemaRegistry = nullptr;
    m_executionSandbox = nullptr;
    m_validationService = nullptr;
}

void ComponentMapEditorManager::reloadComponentTypes()
{
    LOGD("");
    m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
}

void ComponentMapEditorManager::setExtensionContractRegistry(ExtensionContractRegistry *registry)
{
    if (!registry)
    {
        LOGW("Attempted to set a null extension contract registry; ignoring.");
        return;
    }

    delete m_extensionContracts;
    m_extensionContracts = registry;

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
    m_validationService->rebuildValidationFromRegistry(*m_extensionContracts);
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


void ComponentMapEditorManager::setManifestDirectory(const QString &dirPath)
{
    if (dirPath.isEmpty())
    {
        LOGW("Attempted to set an empty manifest directory path; ignoring.");
        return;
    }

    m_manifestDir = dirPath;

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
    m_validationService->rebuildValidationFromRegistry(*m_extensionContracts);
}

ComponentMapEditorManagerBuilder &ComponentMapEditorManagerBuilder::withExtensionContractRegistry(
    ExtensionContractRegistry *registry)
{
    m_extensionContracts = registry;
    return *this;
}

ComponentMapEditorManager *ComponentMapEditorManagerBuilder::build()
{
    // Verify components
    std::string err;

    if (m_extensionContracts == nullptr)
    {
        throw new ExtensionContractRegistryMissingException();
    }

    if (m_ruleFilePath.isEmpty())
    {
        m_ruleFilePath = DEFAULT_EXTENSION_RULE_FILE;
        LOGW("No rule file path provided; using default: {}", m_ruleFilePath.toStdString());
    }

    if (m_manifestDir.isEmpty())
    {
        m_manifestDir = DEFAULT_EXTENSION_MANIFEST_DIR;
        LOGW("No manifest directory provided; using default: {}", m_manifestDir.toStdString());
    }

    if (m_manager != nullptr)
    {
        delete m_manager;
    }

    m_manager = new ComponentMapEditorManager();
    m_manager->setExtensionContractRegistry(m_extensionContracts);
    return m_manager;
}
