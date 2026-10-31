#include "component_map_runtime/DeploymentManager.h"

#include "component_map_runtime/DeploymentValidator.h"

namespace
{
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
    {
        return result;
    }
    if (m_hasActiveGraph && graph.graph_version() == m_activeGraph.graph_version())
    {
        addManagerIssue(&result, "GRAPH_VERSION_ALREADY_ACTIVE", "graph_version",
                        "Graph version is already active.");
        result.set_valid(false);
        return result;
    }

    m_stagedGraph = graph;
    m_hasStagedGraph = true;
    return result;
}

cme::runtime::v1::DeploymentValidationResult DeploymentManager::activate()
{
    cme::runtime::v1::DeploymentValidationResult result;
    if (!m_runtimeService)
    {
        auto *issue = result.add_issues();
        issue->set_code("RUNTIME_SERVICE_REQUIRED");
        issue->set_severity(cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_ERROR);
        issue->set_field_path("runtime_service");
        issue->set_message("Runtime service must be provided.");
        result.set_valid(false);
        return result;
    }
    if (!m_hasStagedGraph)
    {
        auto *issue = result.add_issues();
        issue->set_code("GRAPH_NOT_STAGED");
        issue->set_severity(cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_ERROR);
        issue->set_field_path("graph");
        issue->set_message("A valid graph must be deployed before activation.");
        result.set_valid(false);
        return result;
    }
    if (m_runtimeService->status() != cme::runtime::v1::RUNTIME_STATUS_STOPPED)
    {
        auto *issue = result.add_issues();
        issue->set_code("RUNTIME_MUST_BE_STOPPED");
        issue->set_severity(cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_ERROR);
        issue->set_field_path("runtime.status");
        issue->set_message("Graph activation requires a stopped runtime.");
        result.set_valid(false);
        return result;
    }

    m_activeGraph = m_stagedGraph;
    m_hasActiveGraph = true;
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

} // namespace cme::runtime
