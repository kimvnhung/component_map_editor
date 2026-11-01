#ifndef COMPONENTMAPEDITORMANAGER_H
#define COMPONENTMAPEDITORMANAGER_H

#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/runtime/DynamicOptionsRegistry.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"
#include "extensions/runtime/rules/RuleHotReloadService.h"
#include "extensions/runtime/rules/RuleRuntimeRegistry.h"
#include "services/ValidationService.h"
#include "services/GraphExecutionSandbox.h"
#include "services/RuntimeClient.h"
#include <QObject>
#include <QString>

class ExtensionContractRegistry;
class IComponentTypeProvider;
class ExtensionPackBuilder;

class ComponentMapEditorManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TypeRegistry* componentTypeRegistry READ componentTypeRegistry CONSTANT)
    Q_PROPERTY(PropertySchemaRegistry* propertySchemaRegistry READ propertySchemaRegistry CONSTANT)
    Q_PROPERTY(DynamicOptionsRegistry* dynamicOptionsRegistry READ dynamicOptionsRegistry CONSTANT)
    Q_PROPERTY(GraphExecutionSandbox* executionSandbox READ executionSandbox CONSTANT)
    Q_PROPERTY(QString runtimeAddress READ runtimeAddress WRITE setRuntimeAddress NOTIFY runtimeAddressChanged)
    Q_PROPERTY(QString runtimeStatus READ runtimeStatus NOTIFY runtimeStatusChanged)
    Q_PROPERTY(QString runtimeActiveGraphVersion READ runtimeActiveGraphVersion NOTIFY runtimeStatusChanged)
    Q_PROPERTY(QString runtimeLastError READ runtimeLastError NOTIFY runtimeStatusChanged)
public:
    ComponentMapEditorManager(QObject *parent = nullptr);

    ~ComponentMapEditorManager();

    TypeRegistry *componentTypeRegistry() const { return m_componentTypeRegistry; }
    PropertySchemaRegistry *propertySchemaRegistry() const { return m_propertySchemaRegistry; }
    DynamicOptionsRegistry *dynamicOptionsRegistry() const { return m_dynamicOptionsRegistry; }
    GraphExecutionSandbox *executionSandbox() const { return m_executionSandbox; }

    QString runtimeAddress() const;
    void setRuntimeAddress(const QString &address);
    cme::editor::RuntimeClient *runtimeClient() const { return m_runtimeClient.get(); }

    QString runtimeStatus() const { return m_runtimeStatus; }
    QString runtimeActiveGraphVersion() const { return m_runtimeActiveGraphVersion; }
    QString runtimeLastError() const { return m_runtimeLastError; }
public:
    Q_INVOKABLE void reloadComponentTypes();

    Q_INVOKABLE bool refreshRuntimeStatus();
    Q_INVOKABLE bool deployAndActivate(GraphModel *graph, const QString &graphVersion);
    Q_INVOKABLE bool startRuntime();
    Q_INVOKABLE bool stopRuntime();
    Q_INVOKABLE bool publishRuntimeEvent(const QString &eventId, const QString &componentId,
                                         const QString &messageType, const QByteArray &payload);

    void setExtensionContractRegistry(std::unique_ptr<ExtensionContractRegistry> registry);
    void setRuleFilePath(const QString &filePath);
    void setManifestDirectory(const QString &dirPath);
    /**
     * @brief registerPackFactoryEntry: Registers a pack factory entry with the extension startup loader
     * @param entry
     */
    void registerPackFactoryEntry(PackFactoryEntry entry);

signals:
    void runtimeAddressChanged();
    void runtimeStatusChanged();

private:
    std::unique_ptr<ExtensionContractRegistry> m_extensionContracts{nullptr};

    ExtensionStartupLoader *m_extensionStartupLoader = nullptr;

    TypeRegistry *m_componentTypeRegistry = nullptr;
    PropertySchemaRegistry *m_propertySchemaRegistry = nullptr;
    DynamicOptionsRegistry *m_dynamicOptionsRegistry = nullptr;
    GraphExecutionSandbox *m_executionSandbox = nullptr;
    std::unique_ptr<cme::editor::RuntimeClient> m_runtimeClient{};

    QString m_runtimeStatus{QStringLiteral("unknown")};
    QString m_runtimeActiveGraphVersion{};
    QString m_runtimeLastError{};

    QString m_manifestDir;
    QString m_ruleFilePath;
    RuleHotReloadService *m_ruleHotReloadService = nullptr;
    RuleRuntimeRegistry ruleRegistry;
    RuleBackedConnectionPolicyProvider compiledConnectionPolicy{&ruleRegistry};
    RuleBackedValidationProvider compiledValidation{&ruleRegistry};
};


class ComponentMapEditorManagerBuilder
{
public:
    ComponentMapEditorManagerBuilder();
    ~ComponentMapEditorManagerBuilder();

    ComponentMapEditorManagerBuilder &withExtensionContractRegistry(std::unique_ptr<ExtensionContractRegistry> registry);
    ComponentMapEditorManagerBuilder &withRuleFilePath(const QString &filePath);
    ComponentMapEditorManagerBuilder &withManifestDirectory(const QString &dirPath);
    ComponentMapEditorManagerBuilder &withPackFactoryEntry(std::unique_ptr<PackFactoryEntry> entry);


    std::unique_ptr<ComponentMapEditorManager> build();
private:
    std::unique_ptr<ExtensionContractRegistry> m_extensionContracts{nullptr};
    QString m_ruleFilePath{""};
    QString m_manifestDir{""};
    std::vector<std::unique_ptr<PackFactoryEntry>> m_packFactoryEntries;
    std::unique_ptr<ComponentMapEditorManager> m_manager{nullptr};
};


#endif // COMPONENTMAPEDITORMANAGER_H
