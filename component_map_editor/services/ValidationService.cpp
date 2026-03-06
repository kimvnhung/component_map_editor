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
        if (component->id().isEmpty()) {
            errors << QStringLiteral("Component has an empty id");
            continue;
        }
        if (componentIds.contains(component->id()))
            errors << QStringLiteral("Duplicate component id: %1").arg(component->id());
        else
            componentIds.insert(component->id());

        if (component->width() <= 0.0)
            errors << QStringLiteral("Component '%1' has non-positive width").arg(component->id());

        if (component->height() <= 0.0)
            errors << QStringLiteral("Component '%1' has non-positive height").arg(component->id());
    }

    // Collect connection ids and validate references
    QSet<QString> connectionIds;
    for (ConnectionModel *connection : graph->connectionList()) {
        if (connection->id().isEmpty())
            errors << QStringLiteral("Connection has an empty id");

        if (!componentIds.contains(connection->sourceId()))
            errors << QStringLiteral("Connection '%1' references unknown source component '%2'")
                          .arg(connection->id(), connection->sourceId());

        if (!componentIds.contains(connection->targetId()))
            errors << QStringLiteral("Connection '%1' references unknown target component '%2'")
                          .arg(connection->id(), connection->targetId());

        if (connection->sourceId() == connection->targetId())
            errors << QStringLiteral("Connection '%1' is a self-loop").arg(connection->id());

        if (connectionIds.contains(connection->id()))
            errors << QStringLiteral("Duplicate connection id: %1").arg(connection->id());
        else
            connectionIds.insert(connection->id());
    }

    return errors;
}

bool ValidationService::validate(GraphModel *graph)
{
    return validationErrors(graph).isEmpty();
}
