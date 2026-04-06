#include "TokenKeyCatalog.h"

#include <algorithm>

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

QStringList TokenKeyCatalog::tokenKeys() const
{
    return m_tokenKeys;
}

void TokenKeyCatalog::refresh()
{
    QSet<QString> keySet;

    if (m_graph) {
        const QList<ConnectionModel *> connections = m_graph->connectionList();
        for (const ConnectionModel *connection : connections) {
            if (!connection)
                continue;
            addTokenKeyCandidate(connection->tokenKey(), &keySet);
        }
    }

    for (auto it = m_executionStateSnapshot.constBegin(); it != m_executionStateSnapshot.constEnd(); ++it)
        addTokenKeyCandidate(it.key(), &keySet);

    for (const QVariant &sectionVariant : m_schemaSections) {
        const QVariantMap section = sectionVariant.toMap();
        const QVariantList fields = section.value(QStringLiteral("fields")).toList();
        for (const QVariant &fieldVariant : fields) {
            const QVariantMap field = fieldVariant.toMap();
            const QString fieldKey = field.value(QStringLiteral("key")).toString();
            if (!fieldKey.endsWith(QStringLiteral("Key"), Qt::CaseInsensitive))
                continue;

            addTokenKeyCandidate(field.value(QStringLiteral("defaultValue")), &keySet);

            const QVariantList options = field.value(QStringLiteral("options")).toList();
            for (const QVariant &option : options)
                addTokenKeyCandidate(option, &keySet);
        }
    }

    QStringList sortedKeys = keySet.values();
    std::sort(sortedKeys.begin(), sortedKeys.end(), [](const QString &a, const QString &b) {
        return QString::compare(a, b, Qt::CaseInsensitive) < 0;
    });

    if (m_tokenKeys == sortedKeys)
        return;

    m_tokenKeys = sortedKeys;
    emit tokenKeysChanged();
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
