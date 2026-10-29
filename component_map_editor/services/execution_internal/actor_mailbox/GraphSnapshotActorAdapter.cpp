#include "GraphSnapshotActorAdapter.h"

#include "utils/GraphHelper.h"

namespace cme::actor
{

bool GraphSnapshotActorAdapter::build(const cme::GraphSnapshot &snapshot,
                                      const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                                      const QString &resolvedStartComponentId,
                                      ActorScheduler *scheduler,
                                      QString *error)
{
    if (!scheduler)
    {
        if (error)
        {
            *error = QStringLiteral("ActorScheduler is not set.");
        }
        return false;
    }

    if (resolvedStartComponentId.isEmpty())
    {
        if (error)
        {
            *error = QStringLiteral("Resolved start component ID is empty.");
        }
        return false;
    }

    QHash<QString, int> pendingInDegree;
    for (const auto &component : snapshot.components())
    {
        const QString componentId = QString::fromStdString(component.id());
        if (componentId.isEmpty())
        {
            if (error)
            {
                *error = QStringLiteral("Graph snapshot contains a component with an empty ID.");
            }
            return false;
        }

        if (pendingInDegree.contains(componentId))
        {
            if (error)
            {
                *error = QStringLiteral("Graph snapshot contains duplicate component ID '%1'.").arg(componentId);
            }
            return false;
        }

        pendingInDegree.insert(componentId, 0);
    }

    if (!pendingInDegree.contains(resolvedStartComponentId))
    {
        if (error)
        {
            *error = QStringLiteral("Resolved start component '%1' is not in graph snapshot.")
                     .arg(resolvedStartComponentId);
        }
        return false;
    }

    QSet<QString> connectionIds;
    for (const auto &connection : snapshot.connections())
    {
        const QString connectionId = QString::fromStdString(connection.id());
        const QString sourceId = QString::fromStdString(connection.source_id());
        const QString targetId = QString::fromStdString(connection.target_id());

        if (connectionId.isEmpty() || connectionIds.contains(connectionId))
        {
            if (error)
            {
                *error = QStringLiteral("Graph snapshot contains an invalid or duplicate connection ID '%1'.")
                         .arg(connectionId);
            }
            return false;
        }

        if (!pendingInDegree.contains(sourceId) || !pendingInDegree.contains(targetId))
        {
            if (error)
            {
                *error = QStringLiteral("Connection '%1' references non-existent source or target component.")
                         .arg(connectionId);
            }
            return false;
        }

        connectionIds.insert(connectionId);
        pendingInDegree[targetId] = pendingInDegree.value(targetId) + 1;
    }

    if (pendingInDegree.value(resolvedStartComponentId) != 0)
    {
        if (error)
        {
            *error = QStringLiteral("Resolved start component '%1' has incoming connections.")
                     .arg(resolvedStartComponentId);
        }
        return false;
    }

    for (const auto &component : snapshot.components())
    {
        const QString componentId = QString::fromStdString(component.id());
        const QString componentType = QString::fromStdString(component.type_id());
        const IExecutionSemanticsProvider *provider = providersByType.value(componentType, nullptr);

        auto actor = std::make_shared<ComponentActor>(
                         componentId,
                         componentType,
                         component,
                         provider,
                         [scheduler](const ExecutionContext &ctx, const ExecuteResult &result)
        {
            scheduler->handleActorFinished(ctx, result);
        });

        scheduler->registerActor(componentId, std::move(actor));
    }

    scheduler->setRoutingTable(cme::helper::graph::getOutgoingConnectionsBySourceId(snapshot));
    scheduler->setInitialJoinGates(pendingInDegree);
    return true;
}

} // namespace cme::actor
