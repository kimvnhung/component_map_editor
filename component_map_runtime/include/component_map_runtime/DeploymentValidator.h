#ifndef COMPONENT_MAP_RUNTIME_DEPLOYMENT_VALIDATOR_H
#define COMPONENT_MAP_RUNTIME_DEPLOYMENT_VALIDATOR_H

#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"

#include <deployment.pb.h>

namespace cme::runtime
{

class DeploymentValidator
{
public:
    explicit DeploymentValidator(const RuntimeHandlerRegistry *registry,
                                  const ComponentCatalog *catalog = nullptr);

    cme::runtime::v1::DeploymentValidationResult validate(
        const cme::runtime::v1::DeploymentGraph &graph) const;

private:
    const RuntimeHandlerRegistry *m_registry = nullptr;
    const ComponentCatalog *m_catalog = nullptr;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_DEPLOYMENT_VALIDATOR_H
