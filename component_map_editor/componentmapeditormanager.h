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
#include "services/RuntimeClient.h"

#include <QObject>
#include <QTimer>
#include <QString>

#include <QVariantList>

#include <QVariantMap>

#include <memory>

#include <vector>

class ExtensionContractRegistry;

class IComponentTypeProvider;

class ExtensionPackBuilder;

class ComponentMapEditorManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TypeRegistry* componentTypeRegistry READ componentTypeRegistry CONSTANT)
    Q_PROPERTY(PropertySchemaRegistry* propertySchemaRegistry READ propertySchemaRegistry CONSTANT)
    Q_PROPERTY(DynamicOptionsRegistry* dynamicOptionsRegistry READ dynamicOptionsRegistry CONSTANT)
    Q_PROPERTY(QString runtimeAddress READ runtimeAddress WRITE setRuntimeAddress NOTIFY runtimeAddressChanged)
    Q_PROPERTY(QString runtimeStatus READ runtimeStatus NOTIFY runtimeStatusChanged)
    Q_PROPERTY(QString runtimeActiveGraphVersion READ runtimeActiveGraphVersion NOTIFY runtimeStatusChanged)
    Q_PROPERTY(QString runtimeLastError READ runtimeLastError NOTIFY runtimeStatusChanged)
    Q_PROPERTY(QString runtimeChannelState READ runtimeChannelState NOTIFY runtimeStatusChanged)
    Q_PROPERTY(bool runtimeConnected READ runtimeConnected NOTIFY runtimeStatusChanged)
    Q_PROPERTY(int runtimePollIntervalMs READ runtimePollIntervalMs WRITE setRuntimePollIntervalMs NOTIFY runtimePollIntervalMsChanged)
public:
    ComponentMapEditorManager(QObject *parent = nullptr);

    ~ComponentMapEditorManager();

    TypeRegistry *componentTypeRegistry() const { return m_componentTypeRegistry; }
    PropertySchemaRegistry *propertySchemaRegistry() const { return m_propertySchemaRegistry; }
    DynamicOptionsRegistry *dynamicOptionsRegistry() const { return m_dynamicOptionsRegistry; }

    QString runtimeAddress() const;
    void setRuntimeAddress(const QString &address);
    cme::editor::RuntimeClient *runtimeClient() const { return m_runtimeClient.get(); }

    QString runtimeStatus() const { return m_runtimeStatus; }
    QString runtimeActiveGraphVersion() const { return m_runtimeActiveGraphVersion; }
    QString runtimeLastError() const { return m_runtimeLastError; }
    QString runtimeChannelState() const;
    bool runtimeConnected() const;
    int runtimePollIntervalMs() const { return m_runtimePollIntervalMs; }
    void setRuntimePollIntervalMs(int intervalMs);

    Q_INVOKABLE bool reconnectRuntime();
public:
    Q_INVOKABLE void reloadComponentTypes();

    Q_INVOKABLE bool refreshRuntimeStatus();
    Q_INVOKABLE bool deployAndActivate(GraphModel *graph, const QString &graphVersion);
    Q_INVOKABLE bool startRuntime();
    Q_INVOKABLE bool stopRuntime();
    Q_INVOKABLE bool publishRuntimeEvent(const QString &eventId, const QString &componentId,
                                         const QString &messageType, const QByteArray &payload);
    Q_INVOKABLE QVariantMap runtimeComponentStatus(const QString &componentId);
    Q_INVOKABLE QVariantList runtimeEventHistory(int limit = 50);
    Q_INVOKABLE QVariantList runtimeLogs(int limit = 50);
    Q_INVOKABLE QVariantList runtimeRejections(int limit = 50);

    void setExtensionContractRegistry(std::unique_ptr<ExtensionContractRegistry> registry);
    void setRuleFilePath(const QString &filePath);
    void setManifestDirectory(const QString &dirPath);
    void registerPackFactoryEntry(PackFactoryEntry entry);

signals:
    void runtimeAddressChanged();
    void runtimeStatusChanged();
    void runtimePollIntervalMsChanged();

private:
    std::unique_ptr<ExtensionContractRegistry> m_extensionContracts{nullptr};

    ExtensionStartupLoader *m_extensionStartupLoader = nullptr;

    TypeRegistry *m_componentTypeRegistry = nullptr;
    PropertySchemaRegistry *m_propertySchemaRegistry = nullptr;
    DynamicOptionsRegistry *m_dynamicOptionsRegistry = nullptr;
    std::unique_ptr<cme::editor::RuntimeClient> m_runtimeClient{};

    QString m_runtimeStatus{QStringLiteral("unknown")};
    QString m_runtimeActiveGraphVersion{};
    QString m_runtimeLastError{};
    int m_runtimePollIntervalMs = 2000;
    QTimer *m_runtimePollTimer = nullptr;

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
