#ifndef CME_EXECUTION_PROVIDERRESULTVALIDATION_H
#define CME_EXECUTION_PROVIDERRESULTVALIDATION_H

#include "ExecutionContext.h"

namespace cme::execution
{

// Shared by SequentialExecutionEngine and ActorMailboxExecutionEngine so both engines apply
// the exact same declared-output-key validation after a provider reports success.
bool validateExecutionResult(const ExecuteResult &result, QString &message);

} // namespace cme::execution

#endif // CME_EXECUTION_PROVIDERRESULTVALIDATION_H
