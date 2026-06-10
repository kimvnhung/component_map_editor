#ifndef COMPONENTMAPEDITORMANAGER_H
#define COMPONENTMAPEDITORMANAGER_H

#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"
#include "extensions/runtime/rules/RuleHotReloadService.h"
#include "extensions/runtime/rules/RuleRuntimeRegistry.h"
#include "services/GraphExecutionSandbox.h"
#include <QObject>

class ComponentMapEditorManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TypeRegistry* componentTypeRegistry READ componentTypeRegistry CONSTANT)
    Q_PROPERTY(PropertySchemaRegistry* propertySchemaRegistry READ propertySchemaRegistry CONSTANT)
    Q_PROPERTY(GraphExecutionSandbox* executionSandbox READ executionSandbox CONSTANT)
public:
    explicit ComponentMapEditorManager(ExtensionStartupLoader::PackFactory extensionPackFactory = nullptr);

    TypeRegistry *componentTypeRegistry() const { return m_componentTypeRegistry; }
    PropertySchemaRegistry *propertySchemaRegistry() const { return m_propertySchemaRegistry; }
    GraphExecutionSandbox *executionSandbox() const { return m_executionSandbox; }

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
};

class ComponentMapEditorBuilder
{
public:
    static std::unique_ptr<ComponentMapEditorManager> build(const QString& extensionId,
            ExtensionStartupLoader::PackFactory extensionPackFactory);

};

#endif // COMPONENTMAPEDITORMANAGER_H
