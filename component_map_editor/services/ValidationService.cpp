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

    // Collect node ids and check for duplicates
    QSet<QString> nodeIds;
    for (NodeModel *node : graph->nodeList()) {
        if (node->id().isEmpty()) {
            errors << QStringLiteral("Node has an empty id");
            continue;
        }
        if (nodeIds.contains(node->id()))
            errors << QStringLiteral("Duplicate node id: %1").arg(node->id());
        else
            nodeIds.insert(node->id());
    }

    // Collect edge ids and validate references
    QSet<QString> edgeIds;
    for (EdgeModel *edge : graph->edgeList()) {
        if (edge->id().isEmpty())
            errors << QStringLiteral("Edge has an empty id");

        if (!nodeIds.contains(edge->sourceId()))
            errors << QStringLiteral("Edge '%1' references unknown source node '%2'")
                          .arg(edge->id(), edge->sourceId());

        if (!nodeIds.contains(edge->targetId()))
            errors << QStringLiteral("Edge '%1' references unknown target node '%2'")
                          .arg(edge->id(), edge->targetId());

        if (edge->sourceId() == edge->targetId())
            errors << QStringLiteral("Edge '%1' is a self-loop").arg(edge->id());

        if (edgeIds.contains(edge->id()))
            errors << QStringLiteral("Duplicate edge id: %1").arg(edge->id());
        else
            edgeIds.insert(edge->id());
    }

    return errors;
}

bool ValidationService::validate(GraphModel *graph)
{
    return validationErrors(graph).isEmpty();
}
