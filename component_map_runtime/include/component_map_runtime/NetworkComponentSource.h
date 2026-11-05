#ifndef COMPONENT_MAP_RUNTIME_NETWORK_COMPONENT_SOURCE_H
#define COMPONENT_MAP_RUNTIME_NETWORK_COMPONENT_SOURCE_H

#include "component_map_runtime/ComponentContract.h"
#include "component_map_runtime/ComponentDiscovery.h"

#include <QList>
#include <QString>

namespace cme::runtime
{

struct NetworkDiscoveryResult
{
    QList<ComponentDescriptor> descriptors;
    QList<ComponentDiagnostic> diagnostics;
    bool success = false;

    bool hasUsableDescriptors() const;
};

QList<ComponentDiagnostic> validateNetworkComponentDescriptor(
    const ComponentDescriptor &descriptor,
    const DiscoverySourceConfig &sourceConfig);

QList<ComponentDiagnostic> validateNetworkDiscoveryResult(
    const NetworkDiscoveryResult &result,
    const DiscoverySourceConfig &sourceConfig);

inline bool NetworkDiscoveryResult::hasUsableDescriptors() const
{
    return success && !descriptors.isEmpty();
}

class NetworkComponentSource
{
public:
    virtual ~NetworkComponentSource() = default;

    virtual QString sourceId() const = 0;
    virtual DiscoverySourceConfig configuration() const = 0;
    virtual NetworkDiscoveryResult discover() = 0;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_NETWORK_COMPONENT_SOURCE_H
