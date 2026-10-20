#include "DynamicOptionsRegistry.h"

#include <algorithm>
#include <base_log.h>

#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"

DynamicOptionsRegistry::DynamicOptionsRegistry(QObject* parent) : QObject(parent) {}

GraphModel* DynamicOptionsRegistry::graph() const
{
    return m_graph;
}

void DynamicOptionsRegistry::setGraph(GraphModel* graph)
{
    if (m_graph == graph)
        return;

    if (m_graph)
        QObject::disconnect(m_graph, nullptr, this, nullptr);

    m_graph = graph;
    reconnectGraphSignals();
    emit graphChanged();
    refresh();
}

void DynamicOptionsRegistry::setPropertySchemaRegistry(PropertySchemaRegistry* registry)
{
    if (m_propertySchemaRegistry == registry)
        return;

    m_propertySchemaRegistry = registry;
}

QVariantMap DynamicOptionsRegistry::providerOutputKeyHints() const
{
    return m_providerOutputKeyHints;
}

void DynamicOptionsRegistry::setProviderOutputKeyHints(const QVariantMap& hints)
{
    if (m_providerOutputKeyHints == hints)
        return;

    m_providerOutputKeyHints = hints;
    emit providerOutputKeyHintsChanged();
    refresh();
}

QString DynamicOptionsRegistry::targetComponentId() const
{
    return m_targetComponentId;
}

void DynamicOptionsRegistry::setTargetComponentId(const QString& targetComponentId)
{
    const QString trimmed = targetComponentId.trimmed();
    if (m_targetComponentId == trimmed)
        return;

    m_targetComponentId = trimmed;
    emit targetComponentIdChanged();
    refresh();
}

QStringList DynamicOptionsRegistry::tokenKeys() const
{
    return m_tokenKeys;
}

QVariantList DynamicOptionsRegistry::tokenKeyOptions() const
{
    return m_tokenKeyOptions;
}

void DynamicOptionsRegistry::refresh()
{
    if (!m_graph || m_targetComponentId.isEmpty())
    {
        publishOptions({}, {});
        return;
    }

    QStringList keys;
    QVariantList options;
    QSet<QString> seenKeys;

    if (m_targetComponentId == "graph")
    {
        const QList<ComponentModel*> components = m_graph->componentList();
        for (const ComponentModel* component : components)
        {
            if (!component)
                continue;

            const QString componentId = component->id().trimmed();
            const QString componentTitle = component->title();

            options.push_back(QVariantMap{{QStringLiteral("text"), QString("%1 - %2").arg(componentTitle, componentId)},
                                          {QStringLiteral("value"), componentId},
                                          {QStringLiteral("key"), componentId},
                                          {QStringLiteral("title"), componentTitle}});
        }
        updateOptionsForSource(QStringLiteral("components"), options);
    }
    else
    {
        const QList<ConnectionModel*> connections = m_graph->connectionList();
        for (const ConnectionModel* connection : connections)
        {
            if (!connection)
                continue;
            if (connection->targetId().trimmed() != m_targetComponentId)
                continue;

            const QString sourceId = connection->sourceId().trimmed();
            const ComponentModel* source = m_graph->componentById(sourceId);
            if (!source)
                continue;

            const QString sourceType = source->type();
            const QStringList declared = m_providerOutputKeyHints.value(sourceType).toStringList();
            const QString connectionId = connection->id().trimmed();
            for (const QString& key : declared)
            {
                if (key.isEmpty())
                    continue;

                if (!seenKeys.contains(key))
                {
                    seenKeys.insert(key);
                    keys.append(key);
                }

                const QString tokenReference =
                    connectionId.isEmpty() ? key : QStringLiteral("%1::%2").arg(connectionId, key);
                const QString optionText = connectionId.isEmpty()
                                               ? QStringLiteral("%1 (%2)").arg(key, sourceId)
                                               : QStringLiteral("%1 (%2 via %3)").arg(key, sourceId, connectionId);
                options.append(QVariantMap{{QStringLiteral("text"), optionText},
                                           {QStringLiteral("value"), tokenReference},
                                           {QStringLiteral("key"), key},
                                           {QStringLiteral("tokenId"), connectionId},
                                           {QStringLiteral("sourceId"), sourceId}});
            }
        }
    }

    for (const QString& sourceKey : m_registeredSources.keys())
    {
        m_targetSourceOptions[sourceKey] = m_registeredSources.value(sourceKey);
    }
    // TODO: Need to copy more source options which is dynamicly with special targetComponent later
    emit targetSourceOptionsChanged();
}

QVariantList DynamicOptionsRegistry::sortedOptionsByText(const QVariantList& rows)
{
    QVariantList sorted = rows;
    std::sort(sorted.begin(), sorted.end(),
              [](const QVariant& a, const QVariant& b)
              {
                  const QString textA = a.toMap().value(QStringLiteral("text")).toString();
                  const QString textB = b.toMap().value(QStringLiteral("text")).toString();
                  return QString::compare(textA, textB, Qt::CaseInsensitive) < 0;
              });
    return sorted;
}

void DynamicOptionsRegistry::publishOptions(const QStringList& keys, const QVariantList& options)
{
    const bool keysChanged = (m_tokenKeys != keys);
    const bool optionsChanged = (m_tokenKeyOptions != options);
    if (!keysChanged && !optionsChanged)
        return;

    m_tokenKeys = keys;
    m_tokenKeyOptions = options;
    if (keysChanged)
        emit tokenKeysChanged();
    if (optionsChanged)
        emit tokenKeyOptionsChanged();
}

void DynamicOptionsRegistry::updateOptionsForSource(const QString& sourceKey, const QVariantList& options)
{
    if (sourceKey.trimmed().isEmpty())
        return;

    if (m_registeredSources.contains(sourceKey))
    {
        m_registeredSources[sourceKey] = sortedOptionsByText(options);
    }
    else
    {
        m_registeredSources.insert(sourceKey, sortedOptionsByText(options));
    }
}

QVariantMap DynamicOptionsRegistry::targetSourceOptions() const
{
    return m_targetSourceOptions;
}

void DynamicOptionsRegistry::setTargetSourceOptions(const QVariantMap& newTargetSourceOptions)
{
    if (m_targetSourceOptions == newTargetSourceOptions)
        return;
    m_targetSourceOptions = newTargetSourceOptions;
    emit targetSourceOptionsChanged();
}

QString DynamicOptionsRegistry::sourceKey() const
{
    return m_sourceKey;
}

void DynamicOptionsRegistry::setSourceKey(const QString& newSourceKey)
{
    if (m_sourceKey == newSourceKey)
        return;
    m_sourceKey = newSourceKey;
    emit sourceKeyChanged();
}

void DynamicOptionsRegistry::reconnectGraphSignals()
{
    if (!m_graph)
        return;

    QObject::connect(m_graph, &QObject::destroyed, this,
                     [this]()
                     {
                         m_graph = nullptr;
                         refresh();
                     });

    QObject::connect(m_graph, &GraphModel::connectionsChanged, this, [this]() { refresh(); });

    QObject::connect(m_graph, &GraphModel::componentsChanged, this, [this]() { refresh(); });
}

bool DynamicOptionsRegistry::registerSource(const QString& sourceKey, const QVariantList& options)
{
    if (sourceKey.trimmed().isEmpty())
        return false;

    if (m_registeredSources.contains(sourceKey))
        return false;

    m_registeredSources.insert(sourceKey, sortedOptionsByText(options));
    return true;
}