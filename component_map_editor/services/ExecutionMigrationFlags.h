#ifndef EXECUTIONMIGRATIONFLAGS_H
#define EXECUTIONMIGRATIONFLAGS_H

namespace cme::execution {

// Phase 0 migration toggle for Design B rollout.
// Phase 6 cutover: default is now ON while retaining explicit override for
// one release-window compatibility rollback.
class MigrationFlags
{
public:
    static bool tokenTransportEnabled();
    static void setTokenTransportEnabled(bool enabled);
    static void resetDefaults();
    static bool compatibilityWindowOpen();

    // Plan 3 actor/mailbox engine cutover toggle. Default OFF - GraphExecutionSandbox keeps
    // using SequentialExecutionEngine until explicitly enabled.
    static bool actorEngineEnabled();
    static void setActorEngineEnabled(bool enabled);
};

} // namespace cme::execution

#endif // EXECUTIONMIGRATIONFLAGS_H
