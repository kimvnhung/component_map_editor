#ifndef COMPONENTMAPEDITORMANAGER_H
#define COMPONENTMAPEDITORMANAGER_H

#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"
#include "extensions/runtime/rules/RuleHotReloadService.h"
#include "extensions/runtime/rules/RuleRuntimeRegistry.h"
#include "services/ValidationService.h"
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
    Q_PROPERTY(ValidationService* validationService READ validationService CONSTANT)
public:
    ComponentMapEditorManager(QObject *parent = nullptr);

    ~ComponentMapEditorManager();

    TypeRegistry *componentTypeRegistry() const { return m_componentTypeRegistry; }
    PropertySchemaRegistry *propertySchemaRegistry() const { return m_propertySchemaRegistry; }
    GraphExecutionSandbox *executionSandbox() const { return m_executionSandbox; }
    ValidationService *validationService() const { return m_validationService; }

public:
    Q_INVOKABLE void reloadComponentTypes();


    void setExtensionContractRegistry(ExtensionContractRegistry *registry);
    void setRuleFilePath(const QString &filePath);
    void setManifestDirectory(const QString &dirPath);

private:
    ExtensionContractRegistry *m_extensionContracts = nullptr;
    ExtensionStartupLoader *m_extensionStartupLoader = nullptr;

    TypeRegistry *m_componentTypeRegistry = nullptr;
    PropertySchemaRegistry *m_propertySchemaRegistry = nullptr;
    GraphExecutionSandbox *m_executionSandbox = nullptr;
    ValidationService *m_validationService = nullptr;

    QString m_manifestDir;
    QString m_ruleFilePath;
    RuleHotReloadService *m_ruleHotReloadService = nullptr;
    RuleRuntimeRegistry ruleRegistry;
};


class ComponentMapEditorManagerBuilder
{
public:
    ComponentMapEditorManagerBuilder &withExtensionContractRegistry(ExtensionContractRegistry *registry);

    ComponentMapEditorManager *build();
private:
    ExtensionContractRegistry *m_extensionContracts{nullptr};
    QString m_ruleFilePath{""};
    QString m_manifestDir{""};
    ComponentMapEditorManager *m_manager{nullptr};
};


#endif // COMPONENTMAPEDITORMANAGER_H
