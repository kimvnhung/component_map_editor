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
} // namespace cme::helper
