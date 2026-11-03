#ifndef CME_GRAPHSNAPSHOTACTORADAPTER_H
#define CME_GRAPHSNAPSHOTACTORADAPTER_H

#include <graph.pb.h>

#include "ActorScheduler.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"

namespace cme::actor
{

// Bridges a prepared graph snapshot into the actor scheduler. The snapshot is expected to
// already represent the active execution plan; the adapter only seeds the explicitly resolved
// start component and never infers multiple entry points from in-degree.
class GraphSnapshotActorAdapter
{
public:
    static bool build(const cme::GraphSnapshot &snapshot,
                      const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                      const QString &resolvedStartComponentId,
                      ActorScheduler *scheduler,
                      QString *error);
};

} // namespace cme::actor

#endif // CME_GRAPHSNAPSHOTACTORADAPTER_H
