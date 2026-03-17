#include "ValidationService.h"

#include "GraphSchema.h"

#include <QSet>

ValidationService::ValidationService(QObject *parent)
    : QObject(parent)
{}

QStringList ValidationService::validationErrors(GraphModel *graph)
{
    QStringList errors;
    if (!graph) {
        errors << QStringLiteral("Graph is null");
        return errors;
    }

    // Collect component ids and check for duplicates
    QSet<QString> componentIds;
    for (ComponentModel *component : graph->componentList()) {
        const QJsonObject serialized = GraphSchema::componentToJson(component);
        const QString componentId = serialized[GraphSchema::Keys::componentId()].toString();
        if (componentId.isEmpty()) {
            errors << QStringLiteral("Component has an empty id");
            continue;
        }
        if (componentIds.contains(componentId))
            errors << QStringLiteral("Duplicate component id: %1").arg(componentId);
        else
            componentIds.insert(componentId);

        if (serialized[GraphSchema::Keys::componentWidth()].toDouble() <= 0.0)
            errors << QStringLiteral("Component '%1' has non-positive width").arg(componentId);

        if (serialized[GraphSchema::Keys::componentHeight()].toDouble() <= 0.0)
            errors << QStringLiteral("Component '%1' has non-positive height").arg(componentId);
    }

    // Collect connection ids and validate references
    QSet<QString> connectionIds;
    for (ConnectionModel *connection : graph->connectionList()) {
        const QJsonObject serialized = GraphSchema::connectionToJson(connection);
        const QString connectionId = serialized[GraphSchema::Keys::connectionId()].toString();
        const QString sourceId = serialized[GraphSchema::Keys::connectionSourceId()].toString();
        const QString targetId = serialized[GraphSchema::Keys::connectionTargetId()].toString();
        if (connectionId.isEmpty())
            errors << QStringLiteral("Connection has an empty id");

        if (!componentIds.contains(sourceId))
            errors << QStringLiteral("Connection '%1' references unknown source component '%2'")
                          .arg(connectionId, sourceId);

        if (!componentIds.contains(targetId))
            errors << QStringLiteral("Connection '%1' references unknown target component '%2'")
                          .arg(connectionId, targetId);

        if (sourceId == targetId)
            errors << QStringLiteral("Connection '%1' is a self-loop").arg(connectionId);

        const auto sourceSide = static_cast<ConnectionModel::Side>(
            serialized[GraphSchema::Keys::connectionSourceSide()]
                .toInt(static_cast<int>(ConnectionModel::SideAuto)));
        if (GraphSchema::normalizeConnectionSide(static_cast<int>(sourceSide)) != sourceSide) {
            errors << QStringLiteral("Connection '%1' has invalid sourceSide value %2")
                          .arg(connectionId)
                          .arg(static_cast<int>(sourceSide));
        }

        const auto targetSide = static_cast<ConnectionModel::Side>(
            serialized[GraphSchema::Keys::connectionTargetSide()]
                .toInt(static_cast<int>(ConnectionModel::SideAuto)));
        if (GraphSchema::normalizeConnectionSide(static_cast<int>(targetSide)) != targetSide) {
            errors << QStringLiteral("Connection '%1' has invalid targetSide value %2")
                          .arg(connectionId)
                          .arg(static_cast<int>(targetSide));
        }

        if (connectionIds.contains(connectionId))
            errors << QStringLiteral("Duplicate connection id: %1").arg(connectionId);
        else
            connectionIds.insert(connectionId);
    }

    return errors;
}

bool ValidationService::validate(GraphModel *graph)
{
    return validationErrors(graph).isEmpty();
}
