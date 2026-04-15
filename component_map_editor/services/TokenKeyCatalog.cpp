#include "TokenKeyCatalog.h"

#include <algorithm>

#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"

TokenKeyCatalog::TokenKeyCatalog(QObject *parent)
    : QObject(parent)
{}

GraphModel *TokenKeyCatalog::graph() const
{
    return m_graph;
}

void TokenKeyCatalog::setGraph(GraphModel *graph)
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

QVariantMap TokenKeyCatalog::providerOutputKeyHints() const
{
    return m_providerOutputKeyHints;
}

void TokenKeyCatalog::setProviderOutputKeyHints(const QVariantMap &hints)
{
    if (m_providerOutputKeyHints == hints)
        return;

    m_providerOutputKeyHints = hints;
    emit providerOutputKeyHintsChanged();
    refresh();
}

QString TokenKeyCatalog::targetComponentId() const
{
    return m_targetComponentId;
}

void TokenKeyCatalog::setTargetComponentId(const QString &targetComponentId)
{
    const QString trimmed = targetComponentId.trimmed();
    if (m_targetComponentId == trimmed)
        return;

    m_targetComponentId = trimmed;
    emit targetComponentIdChanged();
    refresh();
}

QStringList TokenKeyCatalog::tokenKeys() const
{
    return m_tokenKeys;
}

QVariantList TokenKeyCatalog::tokenKeyOptions() const
{
    return m_tokenKeyOptions;
}

void TokenKeyCatalog::refresh()
{
    if (!m_graph || m_targetComponentId.isEmpty()) {
        publishOptions({}, {});
        return;
    }

    QStringList keys;
    QVariantList options;
    QSet<QString> seen;

    const QList<ConnectionModel *> connections = m_graph->connectionList();
    for (const ConnectionModel *connection : connections) {
        if (!connection)
            continue;
        if (connection->targetId().trimmed() != m_targetComponentId)
            continue;

        const QString sourceId = connection->sourceId().trimmed();
        const ComponentModel *source = m_graph->componentById(sourceId);
        if (!source)
            continue;

        const QString sourceType = source->type();
        const QStringList declared = m_providerOutputKeyHints.value(sourceType).toStringList();
        for (const QString &key : declared) {
            if (key.isEmpty() || seen.contains(key))
                continue;
            seen.insert(key);
            keys.append(key);
            options.append(QVariantMap{
                { QStringLiteral("text"),     QStringLiteral("%1 (%2)").arg(key, sourceId) },
                { QStringLiteral("value"),    key },
                { QStringLiteral("key"),      key },
                { QStringLiteral("sourceId"), sourceId }
            });
        }
    }

    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b) {
        return QString::compare(a, b, Qt::CaseInsensitive) < 0;
    });

    publishOptions(keys, sortedOptionsByText(options));
}

QVariantList TokenKeyCatalog::sortedOptionsByText(const QVariantList &rows)
{
    QVariantList sorted = rows;
    std::sort(sorted.begin(), sorted.end(), [](const QVariant &a, const QVariant &b) {
        const QString textA = a.toMap().value(QStringLiteral("text")).toString();
        const QString textB = b.toMap().value(QStringLiteral("text")).toString();
        return QString::compare(textA, textB, Qt::CaseInsensitive) < 0;
    });
    return sorted;
}

void TokenKeyCatalog::publishOptions(const QStringList &keys, const QVariantList &options)
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

void TokenKeyCatalog::reconnectGraphSignals()
{
    if (!m_graph)
        return;

    QObject::connect(m_graph, &QObject::destroyed, this, [this]() {
        m_graph = nullptr;
        refresh();
    });

    QObject::connect(m_graph, &GraphModel::connectionsChanged, this, [this]() {
        refresh();
    });

    QObject::connect(m_graph, &GraphModel::componentsChanged, this, [this]() {
        refresh();
    });
}
