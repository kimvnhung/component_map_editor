#ifndef DYNAMICOPTIONSREGISTRY_H
#define DYNAMICOPTIONSREGISTRY_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "models/GraphModel.h"

class DynamicOptionsRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel* graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(PropertySchemaRegistry* propertySchemaRegistry WRITE setPropertySchemaRegistry FINAL)
    Q_PROPERTY(QString targetComponentId READ targetComponentId WRITE setTargetComponentId NOTIFY
                   targetComponentIdChanged FINAL)
    Q_PROPERTY(QStringList tokenKeys READ tokenKeys NOTIFY tokenKeysChanged FINAL)
    Q_PROPERTY(QVariantList tokenKeyOptions READ tokenKeyOptions NOTIFY tokenKeyOptionsChanged FINAL)
    Q_PROPERTY(QVariantMap targetSourceOptions READ targetSourceOptions NOTIFY targetSourceOptionsChanged FINAL)

public:
    explicit DynamicOptionsRegistry(QObject* parent = nullptr);

    GraphModel* graph() const;
    void setGraph(GraphModel* graph);

    void setPropertySchemaRegistry(PropertySchemaRegistry* registry);

    QString targetComponentId() const;
    void setTargetComponentId(const QString& targetComponentId);

    QStringList tokenKeys() const;
    QVariantList tokenKeyOptions() const;

    Q_INVOKABLE void refresh();

    // Rebuilds the output-key cache from registered execution semantics
    // providers. Call once after all extension packs are loaded.
    void rebuildFromRegistry(const ExtensionContractRegistry& registry);

    /**
     * @brief registerSource : Registers a source key with its corresponding options for dynamic option generation.
     * @param sourceKey : The unique identifier for the source (e.g., a component or connection).
     * @param options : A list of options associated with the source, where each option is a QVariantMap containing
     * "text" and "value" keys.
     * @return true if the source was successfully registered, false if the sourceKey is empty, duplicated or options
     * are invalid.
     */
    bool registerSource(const QString& sourceKey, const QVariantList& options);

    QVariantMap targetSourceOptions() const;
    void setTargetSourceOptions(const QVariantMap& newTargetSourceOptions);

signals:
    void graphChanged();
    void targetComponentIdChanged();
    void tokenKeysChanged();
    void tokenKeyOptionsChanged();

    void targetSourceOptionsChanged();

private:
    void reconnectGraphSignals();
    static QVariantList sortedOptionsByText(const QVariantList& rows);
    void publishOptions(const QStringList& keys, const QVariantList& options);

    void updateOptionsForSource(const QString& sourceKey, const QVariantList& options);

    // Builds the option list for the "tokenRef" source from the direct
    // sources of the current target component.
    QVariantList tokenRefOptionsForTarget() const;

private:
    QPointer<GraphModel> m_graph;
    QPointer<PropertySchemaRegistry> m_propertySchemaRegistry;
    QString m_targetComponentId;
    QStringList m_tokenKeys;
    QVariantList m_tokenKeyOptions;

    // Output keys declared by execution semantics providers, keyed by
    // component type. Source of truth for tokenRef dropdown options.
    QHash<QString, QStringList> m_outputKeysByComponentType;

    QVariantMap m_targetSourceOptions;               // Maps targetComponentId to its sourceKey and options
    QMap<QString, QVariantList> m_registeredSources; // Maps sourceKey to its options
};

#endif // DYNAMICOPTIONSREGISTRY_H
