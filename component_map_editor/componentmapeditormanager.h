#ifndef COMPONENTMAPEDITORMANAGER_H
#define COMPONENTMAPEDITORMANAGER_H

#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"
#include "extensions/runtime/rules/RuleHotReloadService.h"
#include "extensions/runtime/rules/RuleRuntimeRegistry.h"
#include "services/GraphExecutionSandbox.h"
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
    Q_PROPERTY(GraphExecutionSandbox* executionSandbox READ executionSandbox CONSTANT)
public:
    explicit ComponentMapEditorManager(PackFactory extensionPackFactory = nullptr);

    // Extended constructor used by the builder to inject custom services and configuration.
    ComponentMapEditorManager(PackFactory extensionPackFactory,
                              const ExtensionApiVersion &coreApiVersion,
                              const IComponentTypeProvider *customComponentProvider,
                              PropertySchemaRegistry *customPropertySchemaRegistry,
                              GraphExecutionSandbox *customExecutionSandbox,
                              const QString &ruleFilePath,
                              const QString &manifestDir);

    TypeRegistry *componentTypeRegistry() const { return m_componentTypeRegistry; }
    PropertySchemaRegistry *propertySchemaRegistry() const { return m_propertySchemaRegistry; }
    GraphExecutionSandbox *executionSandbox() const { return m_executionSandbox; }

public:
    Q_INVOKABLE void reloadComponentTypes();

    // Setter to change the extension pack factory after construction.
    void setExtensionPackFactory(PackFactory factory);

    // Public setters for runtime replacement (also useful for tests).
    void setExtensionContractVersion(const ExtensionApiVersion &v);
    void setRuleFilePath(const QString &path);
    void setManifestDir(const QString &dir);

private:
    ExtensionContractRegistry *m_extensionContracts = nullptr;
    ExtensionStartupLoader *m_extensionStartupLoader = nullptr;

    RuleRuntimeRegistry *m_ruleRegistry = nullptr;
    RuleBackedConnectionPolicyProvider *m_connectionPolicyProvider = nullptr;
    RuleBackedValidationProvider *m_validationProvider = nullptr;
    RuleHotReloadService *m_ruleHotReloadService = nullptr;

    TypeRegistry *m_componentTypeRegistry = nullptr;
    PropertySchemaRegistry *m_propertySchemaRegistry = nullptr;
    GraphExecutionSandbox *m_executionSandbox = nullptr;

    QString m_manifestDir;
};


class ComponentMapEditorManagerBuilder
{
public:
    ComponentMapEditorManagerBuilder &withExtensionPackFactory(PackFactory factory);
    ComponentMapEditorManagerBuilder &withExtensionPackBuilder(const ExtensionPackBuilder &builder);

    ComponentMapEditorManagerBuilder &withExtensionContractVersion(const ExtensionApiVersion &v);
    ComponentMapEditorManagerBuilder &withRuleFilePath(const QString &path);
    ComponentMapEditorManagerBuilder &withManifestDir(const QString &dir);

    ComponentMapEditorManager *build();
private:
    PackFactory m_packFactory{nullptr};
    ExtensionApiVersion m_coreVersion{1, 0, 0};
    ExtensionPackBuilder *m_extensionPackBuilder{nullptr};
    bool m_hasExtensionPackBuilder{false};
    QString m_ruleFilePath;
    QString m_manifestDir;
    ComponentMapEditorManager *m_manager{nullptr};

    void rebuild();
};


#endif // COMPONENTMAPEDITORMANAGER_H
