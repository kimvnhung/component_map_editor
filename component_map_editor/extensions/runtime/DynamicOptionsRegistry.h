#ifndef DYNAMICOPTIONSREGISTRY_H
#define DYNAMICOPTIONSREGISTRY_H

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>

#include "extensions/runtime/PropertySchemaRegistry.h"
#include "models/GraphModel.h"

class DynamicOptionsRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel* graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(PropertySchemaRegistry* propertySchemaRegistry WRITE setPropertySchemaRegistry FINAL)
    Q_PROPERTY(QVariantMap providerOutputKeyHints READ providerOutputKeyHints WRITE setProviderOutputKeyHints NOTIFY
                   providerOutputKeyHintsChanged FINAL)
    Q_PROPERTY(QString targetComponentId READ targetComponentId WRITE setTargetComponentId NOTIFY
                   targetComponentIdChanged FINAL)
    Q_PROPERTY(QStringList tokenKeys READ tokenKeys NOTIFY tokenKeysChanged FINAL)
    Q_PROPERTY(QVariantList tokenKeyOptions READ tokenKeyOptions NOTIFY tokenKeyOptionsChanged FINAL)
    Q_PROPERTY(QMap<QString, QVariantList> targetSourceOptions READ targetSourceOptions WRITE setTargetSourceOptions
                   NOTIFY targetSourceOptionsChanged FINAL)

public:
    explicit DynamicOptionsRegistry(QObject* parent = nullptr);

    GraphModel* graph() const;
    void setGraph(GraphModel* graph);

    void setPropertySchemaRegistry(PropertySchemaRegistry* registry);

    QVariantMap providerOutputKeyHints() const;
    void setProviderOutputKeyHints(const QVariantMap& hints);

    QString targetComponentId() const;
    void setTargetComponentId(const QString& targetComponentId);

    QStringList tokenKeys() const;
    QVariantList tokenKeyOptions() const;

    Q_INVOKABLE void refresh();

    /**
     * @brief registerSource : Registers a source key with its corresponding options for dynamic option generation.
     * @param sourceKey : The unique identifier for the source (e.g., a component or connection).
     * @param options : A list of options associated with the source, where each option is a QVariantMap containing
     * "text" and "value" keys.
     * @return true if the source was successfully registered, false if the sourceKey is empty, duplicated or options
     * are invalid.
     */
    bool registerSource(const QString& sourceKey, const QVariantList& options);

    QString sourceKey() const;
    void setSourceKey(const QString& newSourceKey);

    QMap<QString, QVariantList> targetSourceOptions() const;
    void setTargetSourceOptions(const QMap<QString, QVariantList>& newTargetSourceOptions);

signals:
    void graphChanged();
    void providerOutputKeyHintsChanged();
    void targetComponentIdChanged();
    void tokenKeysChanged();
    void tokenKeyOptionsChanged();

    void sourceKeyChanged();

    void targetSourceOptionsChanged();

private:
    void reconnectGraphSignals();
    static QVariantList sortedOptionsByText(const QVariantList& rows);
    void publishOptions(const QStringList& keys, const QVariantList& options);

    void updateOptionsForSource(const QString& sourceKey, const QVariantList& options);

private:
    QPointer<GraphModel> m_graph;
    QPointer<PropertySchemaRegistry> m_propertySchemaRegistry;
    QVariantMap m_providerOutputKeyHints;
    QString m_targetComponentId;
    QStringList m_tokenKeys;
    QVariantList m_tokenKeyOptions;

    // NEW: Using sourceKey, textRole, and valueRole for dynamic options
    QString m_sourceKey;
    QMap<QString, QVariantList> m_targetSourceOptions; // Maps targetComponentId to its sourceKey and options
    QMap<QString, QVariantList> m_registeredSources;   // Maps sourceKey to its options
};

#endif // DYNAMICOPTIONSREGISTRY_H
