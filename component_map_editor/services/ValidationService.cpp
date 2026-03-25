#include "ValidationService.h"

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
        if (!component)
            continue;

        const QString componentId = component->id();
        if (componentId.isEmpty()) {
            errors << QStringLiteral("Component has an empty id");
            continue;
        }
        if (componentIds.contains(componentId))
            errors << QStringLiteral("Duplicate component id: %1").arg(componentId);
        else
            componentIds.insert(componentId);

        if (component->width() <= 0.0)
            errors << QStringLiteral("Component '%1' has non-positive width").arg(componentId);

        if (component->height() <= 0.0)
            errors << QStringLiteral("Component '%1' has non-positive height").arg(componentId);
    }

    // Collect connection ids and validate references
    QSet<QString> connectionIds;
    for (ConnectionModel *connection : graph->connectionList()) {
        if (!connection)
            continue;

        const QString connectionId = connection->id();
        const QString sourceId = connection->sourceId();
        const QString targetId = connection->targetId();
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

        const auto sourceSide = connection->sourceSide();
        if (sourceSide != ConnectionModel::SideTop
            && sourceSide != ConnectionModel::SideRight
            && sourceSide != ConnectionModel::SideBottom
            && sourceSide != ConnectionModel::SideLeft
            && sourceSide != ConnectionModel::SideAuto) {
            errors << QStringLiteral("Connection '%1' has invalid sourceSide value %2")
                          .arg(connectionId)
                          .arg(static_cast<int>(sourceSide));
        }

        const auto targetSide = connection->targetSide();
        if (targetSide != ConnectionModel::SideTop
            && targetSide != ConnectionModel::SideRight
            && targetSide != ConnectionModel::SideBottom
            && targetSide != ConnectionModel::SideLeft
            && targetSide != ConnectionModel::SideAuto) {
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
