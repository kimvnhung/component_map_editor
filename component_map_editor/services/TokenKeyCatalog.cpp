#include "TokenKeyCatalog.h"

#include <algorithm>

#include "models/ConnectionModel.h"

struct TokenKeyCatalog::TokenOptionAccumulator {
    QSet<QString> keySet;
    QVariantList optionRows;
    QSet<QString> optionValues;

    static QString makeOptionText(const QString &payloadKey,
                                  const QString &sourceId,
                                  const QString &tokenId)
    {
        if (!sourceId.isEmpty())
            return QStringLiteral("%1 (%2)").arg(payloadKey, sourceId);
        if (!tokenId.isEmpty())
            return QStringLiteral("%1 (%2)").arg(payloadKey, tokenId);
        return payloadKey;
    }

    void appendOption(const QString &payloadKey,
                      const QString &tokenId,
                      const QString &sourceId)
    {
        const QString key = payloadKey.trimmed();
        if (key.isEmpty())
            return;

        keySet.insert(key);

        const QString value = tokenId.isEmpty()
            ? key
            : QStringLiteral("%1::%2").arg(tokenId, key);
        if (optionValues.contains(value))
            return;

        optionValues.insert(value);
        optionRows.append(QVariantMap{
            { QStringLiteral("text"), makeOptionText(key, sourceId, tokenId) },
            { QStringLiteral("value"), value },
            { QStringLiteral("key"), key },
            { QStringLiteral("tokenId"), tokenId },
            { QStringLiteral("sourceId"), sourceId }
        });
    }
};

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

    for (ConnectionModel *connection : m_trackedConnections) {
        if (connection)
            QObject::disconnect(connection, nullptr, this, nullptr);
    }
    m_trackedConnections.clear();

    m_graph = graph;
    reconnectGraphSignals();
    rebuildConnectionTracking();
    emit graphChanged();
    refresh();
}

QVariantMap TokenKeyCatalog::executionStateSnapshot() const
{
    return m_executionStateSnapshot;
}

void TokenKeyCatalog::setExecutionStateSnapshot(const QVariantMap &executionStateSnapshot)
{
    if (m_executionStateSnapshot == executionStateSnapshot)
        return;

    m_executionStateSnapshot = executionStateSnapshot;
    emit executionStateSnapshotChanged();
    refresh();
}

QVariantList TokenKeyCatalog::schemaSections() const
{
    return m_schemaSections;
}

void TokenKeyCatalog::setSchemaSections(const QVariantList &schemaSections)
{
    if (m_schemaSections == schemaSections)
        return;

    m_schemaSections = schemaSections;
    emit schemaSectionsChanged();
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
    TokenOptionAccumulator accumulator;
    const QHash<QString, QString> sourceByIncomingTokenId = incomingSourceByTokenId();

    bool hasRuntimeIncomingTokenKeys = collectRuntimeOptions(sourceByIncomingTokenId, &accumulator);
    collectInputStateFallback(&accumulator, &hasRuntimeIncomingTokenKeys);
    collectConsumedTokenIdFallback(&accumulator, &hasRuntimeIncomingTokenKeys);
    collectConnectionFallback(&accumulator, hasRuntimeIncomingTokenKeys);

    publishOptions(sortedKeys(accumulator.keySet),
                   sortedOptionsByText(accumulator.optionRows));
}

QHash<QString, QString> TokenKeyCatalog::incomingSourceByTokenId() const
{
    QHash<QString, QString> sourceByIncomingTokenId;
    if (!m_graph || m_targetComponentId.isEmpty())
        return sourceByIncomingTokenId;

    const QList<ConnectionModel *> connections = m_graph->connectionList();
    for (const ConnectionModel *connection : connections) {
        if (!connection)
            continue;
        if (connection->targetId().trimmed() != m_targetComponentId)
            continue;
        sourceByIncomingTokenId.insert(connection->id(), connection->sourceId().trimmed());
    }

    return sourceByIncomingTokenId;
}

bool TokenKeyCatalog::collectRuntimeOptions(
    const QHash<QString, QString> &sourceByIncomingTokenId,
    TokenOptionAccumulator *accumulator) const
{
    if (!accumulator)
        return false;

    bool hasRuntimeIncomingTokenKeys = false;
    const QVariantMap incomingTokenPayloads =
        m_executionStateSnapshot.value(QStringLiteral("incomingTokenPayloads")).toMap();
    if (incomingTokenPayloads.isEmpty())
        return false;

    QStringList incomingTokenIds = incomingTokenPayloads.keys();
    std::sort(incomingTokenIds.begin(), incomingTokenIds.end(), [](const QString &a, const QString &b) {
        return QString::compare(a, b, Qt::CaseInsensitive) < 0;
    });

    for (const QString &tokenId : incomingTokenIds) {
        const QVariantMap payload = incomingTokenPayloads.value(tokenId).toMap();
        if (payload.isEmpty())
            continue;

        hasRuntimeIncomingTokenKeys = true;
        QStringList payloadKeys = payload.keys();
        std::sort(payloadKeys.begin(), payloadKeys.end(), [](const QString &a, const QString &b) {
            return QString::compare(a, b, Qt::CaseInsensitive) < 0;
        });

        const QString sourceId = sourceByIncomingTokenId.value(tokenId);
        for (const QString &payloadKey : payloadKeys)
            accumulator->appendOption(payloadKey, tokenId, sourceId);
    }

    return hasRuntimeIncomingTokenKeys;
}

void TokenKeyCatalog::collectInputStateFallback(TokenOptionAccumulator *accumulator,
                                                bool *hasRuntimeIncomingTokenKeys) const
{
    if (!accumulator || !hasRuntimeIncomingTokenKeys || *hasRuntimeIncomingTokenKeys)
        return;

    const QVariantMap inputState = m_executionStateSnapshot.value(QStringLiteral("inputState")).toMap();
    if (inputState.isEmpty())
        return;

    *hasRuntimeIncomingTokenKeys = true;
    QStringList payloadKeys = inputState.keys();
    std::sort(payloadKeys.begin(), payloadKeys.end(), [](const QString &a, const QString &b) {
        return QString::compare(a, b, Qt::CaseInsensitive) < 0;
    });

    for (const QString &payloadKey : payloadKeys)
        accumulator->appendOption(payloadKey, QString(), QString());
}

void TokenKeyCatalog::collectConsumedTokenIdFallback(TokenOptionAccumulator *accumulator,
                                                     bool *hasRuntimeIncomingTokenKeys) const
{
    if (!accumulator || !hasRuntimeIncomingTokenKeys || *hasRuntimeIncomingTokenKeys)
        return;

    const QVariant consumedIncomingTokenIds =
        m_executionStateSnapshot.value(QStringLiteral("consumedIncomingTokenIds"));
    const QVariantList consumedList = consumedIncomingTokenIds.toList();
    for (const QVariant &tokenId : consumedList)
        accumulator->appendOption(tokenId.toString(), QString(), QString());
    *hasRuntimeIncomingTokenKeys = !consumedList.isEmpty();
}

void TokenKeyCatalog::collectConnectionFallback(TokenOptionAccumulator *accumulator,
                                                bool hasRuntimeIncomingTokenKeys) const
{
    if (!accumulator || hasRuntimeIncomingTokenKeys || !m_graph)
        return;

    const QList<ConnectionModel *> connections = m_graph->connectionList();
    for (const ConnectionModel *connection : connections) {
        if (!connection)
            continue;

        if (!m_targetComponentId.isEmpty()
            && connection->targetId().trimmed() != m_targetComponentId) {
            continue;
        }

        accumulator->appendOption(connection->tokenKey(), QString(), connection->sourceId().trimmed());
        addTokenKeyCandidate(connection->tokenKey(), &accumulator->keySet);
    }
}

QStringList TokenKeyCatalog::sortedKeys(const QSet<QString> &keys)
{
    QStringList sorted = keys.values();
    std::sort(sorted.begin(), sorted.end(), [](const QString &a, const QString &b) {
        return QString::compare(a, b, Qt::CaseInsensitive) < 0;
    });
    return sorted;
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
        for (ConnectionModel *connection : m_trackedConnections) {
            if (connection)
                QObject::disconnect(connection, nullptr, this, nullptr);
        }
        m_trackedConnections.clear();
        refresh();
    });

    QObject::connect(m_graph, &GraphModel::connectionsChanged, this, [this]() {
        rebuildConnectionTracking();
        refresh();
    });

    QObject::connect(m_graph, &GraphModel::connectionAdded, this, [this](ConnectionModel *) {
        rebuildConnectionTracking();
        refresh();
    });

    QObject::connect(m_graph, &GraphModel::connectionRemoved, this, [this](const QString &) {
        rebuildConnectionTracking();
        refresh();
    });
}

void TokenKeyCatalog::rebuildConnectionTracking()
{
    for (ConnectionModel *connection : m_trackedConnections) {
        if (connection)
            QObject::disconnect(connection, nullptr, this, nullptr);
    }
    m_trackedConnections.clear();

    if (!m_graph)
        return;

    const QList<ConnectionModel *> connections = m_graph->connectionList();
    for (ConnectionModel *connection : connections) {
        if (!connection)
            continue;

        m_trackedConnections.append(connection);

        QObject::connect(connection, &ConnectionModel::tokenKeyChanged, this, [this]() {
            refresh();
        });
        QObject::connect(connection, &QObject::destroyed, this, [this](QObject *) {
            rebuildConnectionTracking();
            refresh();
        });
    }
}

void TokenKeyCatalog::addTokenKeyCandidate(const QVariant &candidate, QSet<QString> *out)
{
    if (!out)
        return;

    if (candidate.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = candidate.toMap();
        if (map.contains(QStringLiteral("value"))) {
            addTokenKeyCandidate(map.value(QStringLiteral("value")), out);
            return;
        }
    }

    const QString tokenKey = candidate.toString().trimmed();
    if (!tokenKey.isEmpty())
        out->insert(tokenKey);
}
