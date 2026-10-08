#include "GraphSnapshotActorAdapter.h"

#include "utils/GraphHelper.h"

namespace cme::actor
{

    bool GraphSnapshotActorAdapter::build(const cme::GraphSnapshot &snapshot,
                                          const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                                          ActorScheduler *scheduler,
                                          QStringList *outEntryComponentIds,
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

        QHash<QString, int> pendingInDegree;

        for (const auto &component : snapshot.components())
        {
            pendingInDegree.insert(QString::fromStdString(component.id()), 0);
        }

        for (const auto &connection : snapshot.connections())
        {
            const QString sourceId = QString::fromStdString(connection.source_id());
            const QString targetId = QString::fromStdString(connection.target_id());

            if (!pendingInDegree.contains(sourceId) || !pendingInDegree.contains(targetId))
            {
                if (error)
                {
                    *error = QStringLiteral("Connection '%1' references non-existent source or target component.")
                             .arg(QString::fromStdString(connection.id()));
                }

                return false;
            }

            pendingInDegree[targetId] = pendingInDegree.value(targetId, 0) + 1;
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
                             [scheduler](const ExecutionContext & ctx, const ExecuteResult & result)
            {
                scheduler->handleActorFinished(ctx, result);
            });

            scheduler->registerActor(componentId, std::move(actor));
        }

        scheduler->setRoutingTable(cme::helper::graph::getOutgoingConnectionsBySourceId(snapshot));
        scheduler->setInitialJoinGates(pendingInDegree);

        if (outEntryComponentIds)
        {
            QStringList componentIds = cme::helper::graph::getComponentIds(snapshot);
            std::sort(componentIds.begin(), componentIds.end(), cme::helper::graph::idComparator);

            outEntryComponentIds->clear();

            for (const QString &componentId : componentIds)
            {
                if (pendingInDegree.value(componentId, 0) == 0)
                {
                    outEntryComponentIds->append(componentId);
                }
            }
        }

        return true;
    }

} // namespace cme::actor
