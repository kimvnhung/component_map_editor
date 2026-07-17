#ifndef COMPONENTMAPEDITORMANAGER_H
#define COMPONENTMAPEDITORMANAGER_H

#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
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
public:
    ComponentMapEditorManager(QObject *parent = nullptr);

    ~ComponentMapEditorManager();

    TypeRegistry *componentTypeRegistry() const { return m_componentTypeRegistry; }
    PropertySchemaRegistry *propertySchemaRegistry() const { return m_propertySchemaRegistry; }
    GraphExecutionSandbox *executionSandbox() const { return m_executionSandbox; }
public:
    Q_INVOKABLE void reloadComponentTypes();


    void setExtensionContractRegistry(std::unique_ptr<ExtensionContractRegistry> registry);
    void setRuleFilePath(const QString &filePath);
    void setManifestDirectory(const QString &dirPath);
    /**
     * @brief registerPackFactoryEntry: Registers a pack factory entry with the extension startup loader
     * @param entry
     */
    void registerPackFactoryEntry(PackFactoryEntry entry);

private:
    std::unique_ptr<ExtensionContractRegistry> m_extensionContracts{nullptr};
    ExtensionStartupLoader *m_extensionStartupLoader = nullptr;

    TypeRegistry *m_componentTypeRegistry = nullptr;
    PropertySchemaRegistry *m_propertySchemaRegistry = nullptr;
    GraphExecutionSandbox *m_executionSandbox = nullptr;

    QString m_manifestDir;
    QString m_ruleFilePath;
    RuleHotReloadService *m_ruleHotReloadService = nullptr;
    RuleRuntimeRegistry ruleRegistry;
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
