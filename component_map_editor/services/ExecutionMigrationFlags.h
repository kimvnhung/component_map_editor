#ifndef EXECUTIONMIGRATIONFLAGS_H
#define EXECUTIONMIGRATIONFLAGS_H

namespace cme::execution {

// Phase 0 migration toggle for Design B rollout.
// Default remains false to preserve legacy global-state semantics.
class MigrationFlags
{
public:
    static bool tokenTransportEnabled();
    static void setTokenTransportEnabled(bool enabled);
    static void resetDefaults();
};

} // namespace cme::execution

#endif // EXECUTIONMIGRATIONFLAGS_H
