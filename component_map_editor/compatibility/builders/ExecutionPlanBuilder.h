#ifndef EXECUTIONPLANBUILDER_H
#define EXECUTIONPLANBUILDER_H

#include "models/GraphModel.h"
#include "compatibility/execution_internal/ExecutionPlan.h"
#include "compatibility/execution_internal/ExecutionRequest.h"

class ExecutionPlanBuilder
{
public:
    static bool build(GraphModel *graph,
                      const ExecutionRequest &request,
                      ExecutionPlan *plan,
                      QString *error = nullptr);
};

#endif // EXECUTIONPLANBUILDER_H
