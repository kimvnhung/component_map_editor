#include "componentmapeditormanager.h"

#include "services/DeploymentGraphExporter.h"
#include <QDir>
#include <QTimer>
#include <base_log.h>
#include "extensions/contracts/ExtensionContractRegistry.h"

#define DEFAULT_EXTENSION_MANIFEST_DIR ""
#define DEFAULT_EXTENSION_RULE_FILE ""

ComponentMapEditorManager::ComponentMapEditorManager(QObject *parent)
    : QObject(parent)
    , m_extensionStartupLoader(new ExtensionStartupLoader())
    , m_componentTypeRegistry(new TypeRegistry())
    , m_propertySchemaRegistry(new PropertySchemaRegistry())
    , m_dynamicOptionsRegistry(new DynamicOptionsRegistry())
    , m_runtimeClient(std::make_unique<cme::editor::RuntimeClient>())
    , m_runtimePollTimer(new QTimer(this))
{
    m_runtimePollTimer->setInterval(m_runtimePollIntervalMs);
    connect(m_runtimePollTimer, &QTimer::timeout, this, [this] {
        refreshRuntimeStatus();
    });
    m_runtimePollTimer->start();
}

ComponentMapEditorManager::~ComponentMapEditorManager()
{
    delete m_ruleHotReloadService;
    m_ruleHotReloadService = nullptr;

    delete m_extensionStartupLoader;
    delete m_componentTypeRegistry;
    delete m_propertySchemaRegistry;
    delete m_dynamicOptionsRegistry;
    m_runtimeClient.reset();

    m_extensionStartupLoader = nullptr;
    m_componentTypeRegistry = nullptr;
    m_propertySchemaRegistry = nullptr;
    m_dynamicOptionsRegistry = nullptr;
}

void ComponentMapEditorManager::reloadComponentTypes()
{
    if (!m_extensionContracts) {
        LOGW("Cannot reload component types: extension contract registry is not set.");
        return;
    }

    m_componentTypeRegistry->rebuildFromRegistry(*m_extensionContracts);
}

bool ComponentMapEditorManager::refreshRuntimeCatalog()
{
    if (!m_runtimeClient || !m_componentTypeRegistry)
    {
        m_runtimeLastError = QStringLiteral("Runtime catalog is not configured.");
        emit runtimeStatusChanged();
        return false;
    }

    QVariantList descriptors;
    QString error;
    if (!m_runtimeClient->componentCatalog(QString(), QString(), QString(), &descriptors, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }

    m_runtimeCatalog = descriptors;
    m_componentTypeRegistry->replaceFromRuntimeCatalog(m_runtimeCatalog);
    if (m_propertySchemaRegistry)
        m_propertySchemaRegistry->replaceFromRuntimeCatalog(m_runtimeCatalog);
    m_runtimeLastError.clear();
    emit runtimeStatusChanged();
    return true;
}

QString ComponentMapEditorManager::runtimeAddress() const
{
    return m_runtimeClient ? m_runtimeClient->address() : QString();
}

QString ComponentMapEditorManager::runtimeChannelState() const
{
    return m_runtimeClient
        ? cme::editor::runtimeChannelStateName(m_runtimeClient->channelState())
        : QStringLiteral("unknown");
}

bool ComponentMapEditorManager::runtimeConnected() const
{
    return m_runtimeClient
        && m_runtimeClient->channelState() == cme::editor::RuntimeChannelState::Ready;
}

void ComponentMapEditorManager::setRuntimePollIntervalMs(int intervalMs)
{
    const int normalizedIntervalMs = qMax(250, intervalMs);
    if (m_runtimePollIntervalMs == normalizedIntervalMs)
        return;

    m_runtimePollIntervalMs = normalizedIntervalMs;
    if (m_runtimePollTimer)
        m_runtimePollTimer->setInterval(m_runtimePollIntervalMs);
    emit runtimePollIntervalMsChanged();
}

bool ComponentMapEditorManager::reconnectRuntime()
{
    if (!m_runtimeClient)
        return false;
    QString error;
    if (!m_runtimeClient->reconnect(&error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }
    return refreshRuntimeStatus();
}

void ComponentMapEditorManager::setRuntimeAddress(const QString &address)
{
    if (!m_runtimeClient || address == m_runtimeClient->address())
        return;

    m_runtimeClient->setAddress(address);
    m_runtimeStatus = QStringLiteral("unknown");
    m_runtimeActiveGraphVersion.clear();
    m_runtimeLastError.clear();

    emit runtimeAddressChanged();
    emit runtimeStatusChanged();
}

bool ComponentMapEditorManager::refreshRuntimeStatus()
{
    if (!m_runtimeClient)
        return false;

    if (m_runtimeClient->address().isEmpty())
    {
        m_runtimeStatus = QStringLiteral("unconfigured");
        m_runtimeActiveGraphVersion.clear();
        m_runtimeLastError = QStringLiteral("Runtime address is not configured.");
        emit runtimeStatusChanged();
        return false;
    }

    cme::runtime::v1::RuntimeStatusResponse response;
    QString error;
    if (!m_runtimeClient->runtimeStatus(&response, &error))
    {
        m_runtimeStatus = QStringLiteral("unreachable");
        m_runtimeActiveGraphVersion.clear();
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }

    m_runtimeStatus = QString::fromStdString(
        cme::runtime::v1::RuntimeStatus_Name(response.status())).toLower();
    m_runtimeActiveGraphVersion = QString::fromStdString(response.graph_version());
    m_runtimeLastError.clear();
    emit runtimeStatusChanged();
    return true;
}

bool ComponentMapEditorManager::deployAndActivate(GraphModel *graph, const QString &graphVersion)
{
    if (!m_runtimeClient)
        return false;

    cme::editor::DeploymentGraphExporter::Options options;
    options.graphVersion = graphVersion;

    cme::runtime::v1::DeploymentGraph deployment;
    QString error;
    if (!cme::editor::DeploymentGraphExporter::exportGraph(graph, options, &deployment, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }

    cme::runtime::v1::ControlResponse response;
    if (!m_runtimeClient->deployGraph(deployment, &response, &error)
        || !m_runtimeClient->activateGraph(graphVersion, &response, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }

    m_runtimeLastError.clear();
    return refreshRuntimeStatus();
}

bool ComponentMapEditorManager::publishRuntimeEvent(const QString &eventId,
                                                    const QString &componentId,
                                                    const QString &messageType,
                                                    const QByteArray &payload)
{
    if (!m_runtimeClient)
        return false;

    cme::runtime::v1::PublishEventRequest request;
    request.set_event_id(eventId.toStdString());
    request.set_target_component_id(componentId.toStdString());
    request.set_message_type(messageType.toStdString());
    request.set_payload(payload.toStdString());

    QString error;
    if (!m_runtimeClient->publishEvent(request, nullptr, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }

    m_runtimeLastError.clear();
    emit runtimeStatusChanged();
    return true;
}

QVariantMap ComponentMapEditorManager::runtimeComponentStatus(const QString &componentId)
{
    QVariantMap result;
    if (!m_runtimeClient)
        return result;

    cme::runtime::v1::ComponentStatusResponse response;
    QString error;
    if (!m_runtimeClient->componentStatus(componentId, &response, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return result;
    }

    result.insert(QStringLiteral("found"), response.found());
    result.insert(QStringLiteral("componentId"), QString::fromStdString(response.component_id()));
    result.insert(QStringLiteral("componentType"), QString::fromStdString(response.component_type()));
    result.insert(QStringLiteral("status"), QString::fromStdString(response.status()));
    result.insert(QStringLiteral("mailboxDepth"), QVariant::fromValue<qlonglong>(response.mailbox_depth()));
    result.insert(QStringLiteral("state"), QByteArray(response.state().data(), static_cast<int>(response.state().size())));
    m_runtimeLastError.clear();
    emit runtimeStatusChanged();
    return result;
}

QVariantList ComponentMapEditorManager::runtimeEventHistory(int limit)
{
    QVariantList result;
    if (!m_runtimeClient)
        return result;

    cme::runtime::v1::EventHistoryResponse response;
    QString error;
    if (!m_runtimeClient->eventHistory(limit, &response, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return result;
    }

    for (const auto &item : response.items())
    {
        QVariantMap entry;
        entry.insert(QStringLiteral("eventId"), QString::fromStdString(item.event_id()));
        entry.insert(QStringLiteral("status"), QString::fromStdString(item.status()));
        entry.insert(QStringLiteral("failureCode"), QString::fromStdString(item.failure_code()));
        entry.insert(QStringLiteral("failureMessage"), QString::fromStdString(item.failure_message()));
        entry.insert(QStringLiteral("createdAt"), QVariant::fromValue<qlonglong>(item.created_at()));
        result.append(entry);
    }
    m_runtimeLastError.clear();
    emit runtimeStatusChanged();
    return result;
}

QVariantList ComponentMapEditorManager::runtimeLogs(int limit)
{
    QVariantList result;
    if (!m_runtimeClient)
        return result;

    cme::runtime::v1::RuntimeLogsResponse response;
    QString error;
    if (!m_runtimeClient->runtimeLogs(limit, &response, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return result;
    }

    for (const auto &item : response.items())
    {
        QVariantMap entry;
        entry.insert(QStringLiteral("level"), QString::fromStdString(item.level()));
        entry.insert(QStringLiteral("code"), QString::fromStdString(item.code()));
        entry.insert(QStringLiteral("message"), QString::fromStdString(item.message()));
        entry.insert(QStringLiteral("createdAt"), QVariant::fromValue<qlonglong>(item.created_at()));
        result.append(entry);
    }
    m_runtimeLastError.clear();
    emit runtimeStatusChanged();
    return result;
}

QVariantList ComponentMapEditorManager::runtimeRejections(int limit)
{
    QVariantList result;
    if (!m_runtimeClient)
        return result;

    cme::runtime::v1::RejectionsResponse response;
    QString error;
    if (!m_runtimeClient->rejections(limit, &response, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return result;
    }

    for (const auto &item : response.items())
    {
        QVariantMap entry;
        entry.insert(QStringLiteral("eventId"), QString::fromStdString(item.event_id()));
        entry.insert(QStringLiteral("idempotencyKey"), QString::fromStdString(item.idempotency_key()));
        entry.insert(QStringLiteral("targetComponentId"), QString::fromStdString(item.target_component_id()));
        entry.insert(QStringLiteral("routeId"), QString::fromStdString(item.route_id()));
        entry.insert(QStringLiteral("reasonCode"), QString::fromStdString(item.reason_code()));
        entry.insert(QStringLiteral("mailboxDepth"), QVariant::fromValue<qlonglong>(item.mailbox_depth()));
        entry.insert(QStringLiteral("createdAt"), QVariant::fromValue<qlonglong>(item.created_at()));
        result.append(entry);
    }
    m_runtimeLastError.clear();
    emit runtimeStatusChanged();
    return result;
}

bool ComponentMapEditorManager::startRuntime()
{
    if (!m_runtimeClient)
        return false;

    QString error;
    if (!m_runtimeClient->startRuntime(nullptr, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }

    return refreshRuntimeStatus();
}

bool ComponentMapEditorManager::stopRuntime()
{
    if (!m_runtimeClient)
        return false;

    QString error;
    if (!m_runtimeClient->stopRuntime(nullptr, &error))
    {
        m_runtimeLastError = error;
        emit runtimeStatusChanged();
        return false;
    }

    return refreshRuntimeStatus();
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
    m_dynamicOptionsRegistry->rebuildFromRegistry(*m_extensionContracts);


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
