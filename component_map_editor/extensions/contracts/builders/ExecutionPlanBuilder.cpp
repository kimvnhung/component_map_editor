#include "ExecutionPlanBuilder.h"

#include <QQueue>

namespace
{
void setError(QString *error, const QString &message)
{
    if (error)
    {
        *error = message;
    }
}
}

bool ExecutionPlanBuilder::build(GraphModel *graph,
                                 const ExecutionRequest &request,
                                 ExecutionPlan *plan,
                                 QString *error)
{
    if (!plan)
    {
        setError(error, QStringLiteral("Execution plan output is null."));
        return false;
    }

    *plan = ExecutionPlan{};

    if (!graph)
    {
        setError(error, QStringLiteral("Graph is not set."));
        return false;
    }

    QHash<QString, ComponentModel *> componentsById;
    for (ComponentModel *component : graph->componentList())
    {
        if (!component)
        {
            setError(error, QStringLiteral("Graph contains a null component."));
            return false;
        }

        const QString id = component->id().trimmed();
        if (id.isEmpty())
        {
            setError(error, QStringLiteral("Graph contains a component with an empty ID."));
            return false;
        }

        if (componentsById.contains(id))
        {
            setError(error, QStringLiteral("Graph contains duplicate component ID '%1'.").arg(id));
            return false;
        }

        componentsById.insert(id, component);
    }

    const QString requestedId = request.requestedStartComponentId.trimmed();
    const QString graphId = graph->startComponentId().trimmed();
    const QString startId = requestedId.isEmpty() ? graphId : requestedId;

    if (startId.isEmpty())
    {
        setError(error, QStringLiteral("Graph has no start component ID."));
        return false;
    }

    if (!componentsById.contains(startId))
    {
        setError(error, QStringLiteral("Start component '%1' does not exist in graph.").arg(startId));
        return false;
    }

    QHash<QString, QList<ConnectionModel *>> outgoing;
    QHash<QString, int> fullInDegree;
    QSet<QString> connectionIds;

    for (const QString &id : componentsById.keys())
    {
        fullInDegree.insert(id, 0);
    }

    for (ConnectionModel *connection : graph->connectionList())
    {
        if (!connection)
        {
            setError(error, QStringLiteral("Graph contains a null connection."));
            return false;
        }

        const QString connectionId = connection->id().trimmed();
        const QString sourceId = connection->sourceId().trimmed();
        const QString targetId = connection->targetId().trimmed();

        if (connectionId.isEmpty())
        {
            setError(error, QStringLiteral("Graph contains a connection with an empty ID."));
            return false;
        }

        if (connectionIds.contains(connectionId))
        {
            setError(error, QStringLiteral("Graph contains duplicate connection ID '%1'.").arg(connectionId));
            return false;
        }

        if (!componentsById.contains(sourceId) || !componentsById.contains(targetId))
        {
            setError(error,
                     QStringLiteral("Connection '%1' references a non-existent source or target component.")
                         .arg(connectionId));
            return false;
        }

        connectionIds.insert(connectionId);
        outgoing[sourceId].append(connection);
        fullInDegree[targetId] = fullInDegree.value(targetId) + 1;
    }

    if (request.requireRootStart && fullInDegree.value(startId) != 0)
    {
        setError(error,
                 QStringLiteral("Start component '%1' must not have incoming connections.").arg(startId));
        return false;
    }

    plan->resolvedStartComponentId = startId;

    QQueue<QString> pending;
    pending.enqueue(startId);
    plan->reachableComponentIds.insert(startId);

    while (!pending.isEmpty())
    {
        const QString sourceId = pending.dequeue();
        const QList<ConnectionModel *> edges = outgoing.value(sourceId);

        for (ConnectionModel *connection : edges)
        {
            const QString targetId = connection->targetId().trimmed();
            plan->activeConnectionIds.insert(connection->id().trimmed());

            if (!plan->reachableComponentIds.contains(targetId))
            {
                plan->reachableComponentIds.insert(targetId);
                pending.enqueue(targetId);
            }
        }
    }

    for (const QString &componentId : componentsById.keys())
    {
        if (!plan->reachableComponentIds.contains(componentId))
        {
            plan->ignoredComponentIds.insert(componentId);
        }
    }

    for (const QString &componentId : plan->reachableComponentIds)
    {
        plan->pendingInDegree.insert(componentId, 0);
    }

    for (ConnectionModel *connection : graph->connectionList())
    {
        const QString sourceId = connection->sourceId().trimmed();
        const QString targetId = connection->targetId().trimmed();

        if (plan->reachableComponentIds.contains(sourceId)
            && plan->reachableComponentIds.contains(targetId))
        {
            plan->pendingInDegree[targetId] = plan->pendingInDegree.value(targetId) + 1;
        }
    }

    plan->diagnostics.append(QStringLiteral("Resolved start component '%1'.").arg(startId));
    if (!plan->ignoredComponentIds.isEmpty())
    {
        plan->diagnostics.append(QStringLiteral("Ignored %1 unreachable component(s).")
                                     .arg(plan->ignoredComponentIds.size()));
    }

    return true;
}
