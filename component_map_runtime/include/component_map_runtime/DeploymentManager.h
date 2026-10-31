#ifndef COMPONENT_MAP_RUNTIME_DEPLOYMENT_MANAGER_H
#define COMPONENT_MAP_RUNTIME_DEPLOYMENT_MANAGER_H

#include "component_map_runtime/RuntimeHandlerRegistry.h"
#include "component_map_runtime/RuntimeService.h"

#include <deployment.pb.h>

namespace cme::runtime
{

class DeploymentManager
{
public:
    explicit DeploymentManager(RuntimeService *runtimeService,
                               const RuntimeHandlerRegistry *registry = nullptr);

    cme::runtime::v1::DeploymentValidationResult deploy(
        const cme::runtime::v1::DeploymentGraph &graph);
    cme::runtime::v1::DeploymentValidationResult activate();

    bool hasStagedGraph() const;
    bool hasActiveGraph() const;
    const cme::runtime::v1::DeploymentGraph &stagedGraph() const;
    const cme::runtime::v1::DeploymentGraph &activeGraph() const;

private:
    RuntimeService *m_runtimeService = nullptr;
    const RuntimeHandlerRegistry *m_registry = nullptr;
    cme::runtime::v1::DeploymentGraph m_stagedGraph;
    cme::runtime::v1::DeploymentGraph m_activeGraph;
    bool m_hasStagedGraph = false;
    bool m_hasActiveGraph = false;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_DEPLOYMENT_MANAGER_H
