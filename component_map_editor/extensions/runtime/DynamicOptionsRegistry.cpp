#include "DynamicOptionsRegistry.h"

#include <algorithm>
#include <base_log.h>

#include "extensions/contracts/IExecutionSemanticsProvider.h"
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

void DynamicOptionsRegistry::rebuildFromRegistry(const ExtensionContractRegistry& registry)
{
    m_outputKeysByComponentType.clear();

    for (const IExecutionSemanticsProvider* provider : registry.executionSemanticsProviders())
    {
        if (!provider)
            continue;

        for (const QString& componentType : provider->supportedComponentTypes())
        {
            const QString normalizedType = componentType.trimmed();
            if (normalizedType.isEmpty() || m_outputKeysByComponentType.contains(normalizedType))
                continue;

            m_outputKeysByComponentType.insert(normalizedType,
                                               provider->providedOutputKeys(normalizedType));
        }
    }

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

void DynamicOptionsRegistry::refresh()
{
    if (!m_graph || m_targetComponentId.isEmpty())
    {
        m_targetSourceOptions.clear();
        emit targetSourceOptionsChanged();
        return;
    }

    m_registeredSources.clear();

    if (m_targetComponentId == QStringLiteral("graph"))
    {
        QVariantList options;
        const QList<ComponentModel*> components = m_graph->componentList();
        for (const ComponentModel* component : components)
        {
            if (!component)
                continue;

            const QString componentId = component->id().trimmed();
            const QString componentTitle = component->title();

            options.push_back(QVariantMap{{QStringLiteral("text"), componentTitle},
                                          {QStringLiteral("value"), componentId},
                                          {QStringLiteral("key"), componentId},
                                          {QStringLiteral("title"), componentTitle}});
        }
        if (components.isEmpty())
        {
            options.push_back(QVariantMap{{QStringLiteral("text"), QString("No components found")},
                                          {QStringLiteral("value"), QString()},
                                          {QStringLiteral("key"), QString()},
                                          {QStringLiteral("title"), QString()}});
        }

        updateOptionsForSource(QStringLiteral("components"), options);
    }
    else
    {
        const QVariantList options = tokenRefOptionsForTarget();
        updateOptionsForSource(QStringLiteral("tokenRef"), options);
    }

    QVariantMap sourceOptions;
    for (auto it = m_registeredSources.constBegin(); it != m_registeredSources.constEnd(); ++it)
        sourceOptions.insert(it.key(), it.value());

    setTargetSourceOptions(sourceOptions);
}

QVariantList DynamicOptionsRegistry::tokenRefOptionsForTarget() const
{
    QVariantList options;

    if (!m_graph || m_targetComponentId.isEmpty())
        return options;

    for (const ConnectionModel* connection : m_graph->connectionList())
    {
        if (!connection || connection->targetId().trimmed() != m_targetComponentId)
            continue;

        const QString sourceId = connection->sourceId().trimmed();
        const ComponentModel* source = m_graph->componentById(sourceId);
        if (!source)
            continue;

        const QStringList outputKeys = m_outputKeysByComponentType.value(source->type().trimmed());
        const QString connectionId = connection->id().trimmed();
        for (const QString& rawKey : outputKeys)
        {
            const QString key = rawKey.trimmed();
            if (key.isEmpty())
                continue;

            const QString tokenReference = connectionId.isEmpty()
                                               ? key
                                               : QStringLiteral("%1::%2").arg(connectionId, key);
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

    return sortedOptionsByText(options);
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