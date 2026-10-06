#include "GraphHelper.h"

namespace cme::helper
{
    QStringList getComponentIds(const cme::GraphSnapshot &graph)
    {
        QStringList componentIds;

        for (const auto &component : graph.components())
        {
            componentIds.append(QString::fromStdString(component.id()));
        }

        return componentIds;
    }

    ComponentData getComponentById(const cme::GraphSnapshot &graph, const QString &componentId)
    {
        for (const auto &component : graph.components())
        {
            if (QString::fromStdString(component.id()) == componentId)
            {
                return component;
            }
        }

        return ComponentData(); // Return an empty ComponentData if not found
    }

    int getComponentCount(const cme::GraphSnapshot &graph)
    {
        return graph.components_size();
    }

    QList<ConnectionData> getConnectionsBySourceId(const cme::GraphSnapshot &graph, const QString &sourceId)
    {
        QList<ConnectionData> connections;

        for (const auto &connection : graph.connections())
        {
            if (QString::fromStdString(connection.source_id()) == sourceId)
            {
                connections.append(connection);
            }
        }

        return connections;
    }

    QHash<QString, QList<ConnectionData>> getOutgoingConnectionsBySourceId(const cme::GraphSnapshot &graph)
    {
        QHash<QString, QList<ConnectionData>> outgoingConnections;

        for (const auto &connection : graph.connections())
        {
            QString sourceId = QString::fromStdString(connection.source_id());
            outgoingConnections[sourceId].append(connection);
        }

        return outgoingConnections;
    }

    QList<ConnectionData> getConnectionsByTargetId(const cme::GraphSnapshot &graph, const QString &targetId)
    {
        QList<ConnectionData> connections;

        for (const auto &connection : graph.connections())
        {
            if (QString::fromStdString(connection.target_id()) == targetId)
            {
                connections.append(connection);
            }
        }

        return connections;
    }

    QHash<QString, QList<ConnectionData>> getIncomingConnectionsByTargetId(const cme::GraphSnapshot &graph)
    {
        QHash<QString, QList<ConnectionData>> incomingConnections;

        for (const auto &connection : graph.connections())
        {
            QString targetId = QString::fromStdString(connection.target_id());
            incomingConnections[targetId].append(connection);
        }

        return incomingConnections;
    }

    execution::ExecutionPayload getConnectionPayloadById(const cme::GraphSnapshot &graph, const QString &connectionId)
    {
        for (const auto &connection : graph.connections())
        {
            if (QString::fromStdString(connection.id()) == connectionId)
            {
                return mapToVariantMap(connection.properties());
            }
        }

        return execution::ExecutionPayload(); // Return an empty ExecutionPayload if not found
    }

    bool setPayload(cme::GraphSnapshot &graph, const QString &connectionId, const execution::ExecutionPayload &payload)
    {
        for (auto &connection : *graph.mutable_connections())
        {
            if (QString::fromStdString(connection.id()) == connectionId)
            {
                *connection.mutable_properties() = variantMapToMap(payload);
                return true; // Payload set successfully
            }
        }

        return false; // Connection not found
    }

    QVariantMap mapToVariantMap(const google::protobuf::Map<std::string, std::string> &map)
    {
        QVariantMap variantMap;

        for (const auto &pair : map)
        {
            variantMap.insert(QString::fromStdString(pair.first), QString::fromStdString(pair.second));
        }

        return variantMap;
    }

    google::protobuf::Map<std::string, std::string> variantMapToMap(const QVariantMap &variantMap)
    {
        google::protobuf::Map<std::string, std::string> map;

        for (auto it = variantMap.constBegin(); it != variantMap.constEnd(); ++it)
        {
            map.insert({it.key().toStdString(), it.value().toString().toStdString()});
        }

        return map;
    }

    bool idComparator(const QString &a, const QString &b)
    {
        return a < b;
    }

    QVariantMap mergeIncomingTokens(const cme::execution::IncomingTokens &incomingTokens)
    {
        QVariantMap merged;
        QStringList tokenKeys = incomingTokens.keys();
        std::sort(tokenKeys.begin(), tokenKeys.end());

        for (const QString &tokenKey : tokenKeys)
        {
            merged.insert(incomingTokens.value(tokenKey));
        }

        return merged;
    }

    QVariant redactVariant(const QVariant &value,
                           const QSet<QString> &sensitiveKeys,
                           int *redactedCount)
    {
        if (value.metaType().id() == QMetaType::QVariantMap)
        {
            const QVariantMap map = value.toMap();
            QVariantMap redacted;
            QStringList keys = map.keys();
            std::sort(keys.begin(), keys.end());

            for (const QString &key : keys)
            {
                if (sensitiveKeys.contains(key))
                {
                    redacted.insert(key, QStringLiteral("<redacted>"));

                    if (redactedCount)
                    {
                        ++(*redactedCount);
                    }
                }
                else
                {
                    redacted.insert(key, redactVariant(map.value(key), sensitiveKeys, redactedCount));
                }
            }

            return redacted;
        }

        if (value.metaType().id() == QMetaType::QVariantList)
        {
            const QVariantList list = value.toList();
            QVariantList redacted;
            redacted.reserve(list.size());

            for (const QVariant &item : list)
            {
                redacted.append(redactVariant(item, sensitiveKeys, redactedCount));
            }

            return redacted;
        }

        return value;
    }

    qint64 estimatePayloadBytes(const QVariantMap &payload)
    {
        return QJsonDocument::fromVariant(payload).toJson(QJsonDocument::Compact).size();
    }

} // namespace cme::helper
