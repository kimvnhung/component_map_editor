#ifndef CME_GRAPHSNAPSHOTACTORADAPTER_H
#define CME_GRAPHSNAPSHOTACTORADAPTER_H

#include <graph.pb.h>

#include "ActorScheduler.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"

namespace cme::actor
{

// Bridges the domain graph (cme::GraphSnapshot + real IExecutionSemanticsProvider) into an
// ActorScheduler: builds one ComponentActor per component, wires the routing table and
// initial join-gate in-degree counts, and reports which components are ready to fire first
// (in-degree 0). Mirrors SequentialExecutionEngine::prepare()'s graph-capture semantics so
// both engines schedule the same DAG identically.
class GraphSnapshotActorAdapter
{
public:
    static bool build(const cme::GraphSnapshot &snapshot,
                      const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                      ActorScheduler *scheduler,
                      QStringList *outEntryComponentIds,
                      QString *error);
};

} // namespace cme::actor

#endif // CME_GRAPHSNAPSHOTACTORADAPTER_H
