#include "component_map_runtime/DeploymentManager.h"

#include "component_map_runtime/DeploymentValidator.h"
#include "component_map_runtime/FlattenCompiler.h"

namespace
{
void addFlattenIssues(cme::runtime::v1::DeploymentValidationResult *result,
                      const cme::runtime::FlattenResult &flatten)
{
    for (const auto &item : flatten.issues)
    {
        auto *issue = result->add_issues();
        issue->set_code(item.code.toStdString());
        issue->set_severity(cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_ERROR);
        issue->set_field_path(item.fieldPath.toStdString());
        issue->set_message(item.message.toStdString());
    }
}

void addManagerIssue(cme::runtime::v1::DeploymentValidationResult *result,
                     const char *code,
                     const char *path,
                     const char *message)
{
    auto *issue = result->add_issues();
    issue->set_code(code);
    issue->set_severity(cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_ERROR);
    issue->set_field_path(path);
    issue->set_message(message);
}
}

namespace cme::runtime
{

DeploymentManager::DeploymentManager(RuntimeService *runtimeService,
                                     const RuntimeHandlerRegistry *registry)
    : m_runtimeService(runtimeService)
    , m_registry(registry)
{
}

cme::runtime::v1::DeploymentValidationResult DeploymentManager::deploy(
    const cme::runtime::v1::DeploymentGraph &graph)
{
    DeploymentValidator validator(m_registry);
    auto result = validator.validate(graph);
    if (!result.valid())
        return result;

    if (m_hasActiveGraph && graph.graph_version() == m_activeGraph.graph_version())
    {
        addManagerIssue(&result, "GRAPH_VERSION_ALREADY_ACTIVE", "graph_version",
                        "Graph version is already active.");
        result.set_valid(false);
        return result;
    }

    const auto flatten = FlattenCompiler().compile(graph);
    if (!flatten.valid)
    {
        addFlattenIssues(&result, flatten);
        result.set_valid(false);
        return result;
    }

    m_stagedGraph = graph;
    m_stagedPlan = flatten.plan;
    m_hasStagedGraph = true;
    m_hasStagedPlan = true;
    return result;
}

cme::runtime::v1::DeploymentValidationResult DeploymentManager::activate()
{
    cme::runtime::v1::DeploymentValidationResult result;
    if (!m_runtimeService)
    {
        addManagerIssue(&result, "RUNTIME_SERVICE_REQUIRED", "runtime_service",
                        "Runtime service must be provided.");
        result.set_valid(false);
        return result;
    }
    if (!m_hasStagedGraph)
    {
        addManagerIssue(&result, "GRAPH_NOT_STAGED", "graph",
                        "A valid graph must be deployed before activation.");
        result.set_valid(false);
        return result;
    }
    if (m_runtimeService->status() != cme::runtime::v1::RUNTIME_STATUS_STOPPED)
    {
        addManagerIssue(&result, "RUNTIME_MUST_BE_STOPPED", "runtime.status",
                        "Graph activation requires a stopped runtime.");
        result.set_valid(false);
        return result;
    }
    if (!m_hasStagedPlan)
    {
        addManagerIssue(&result, "EXECUTION_PLAN_NOT_READY", "execution_plan",
                        "The staged graph has no valid execution plan.");
        result.set_valid(false);
        return result;
    }

    m_activeGraph = m_stagedGraph;
    m_activePlan = m_stagedPlan;
    m_hasActiveGraph = true;
    m_hasActivePlan = true;
    result.set_valid(true);
    return result;
}

bool DeploymentManager::hasStagedGraph() const
{
    return m_hasStagedGraph;
}

bool DeploymentManager::hasActiveGraph() const
{
    return m_hasActiveGraph;
}

const cme::runtime::v1::DeploymentGraph &DeploymentManager::stagedGraph() const
{
    return m_stagedGraph;
}

const cme::runtime::v1::DeploymentGraph &DeploymentManager::activeGraph() const
{
    return m_activeGraph;
}

const ExecutionPlan &DeploymentManager::activePlan() const
{
    return m_activePlan;
}

} // namespace cme::runtime
