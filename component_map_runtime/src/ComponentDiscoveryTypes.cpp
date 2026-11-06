#include "component_map_runtime/ComponentDiscoveryTypes.h"

namespace cme::runtime
{

bool ComponentKey::isValid() const
{
    return !identity.isEmpty() && !sourceId.isEmpty() && !version.isEmpty();
}

QString ComponentKey::toString() const
{
    return identity + QStringLiteral("@") + version + QStringLiteral("#") + sourceId;
}

uint qHash(const ComponentKey &key, uint seed)
{
    return qHash(key.identity, seed) ^ qHash(key.sourceId, seed << 1)
           ^ qHash(key.version, seed << 2);
}

ComponentKey componentKey(const ComponentDescriptor &descriptor)
{
    return {descriptor.identity, descriptor.sourceId, descriptor.version};
}

bool equivalentComponentDescriptor(const ComponentDescriptor &left,
                                   const ComponentDescriptor &right)
{
    return left.identity == right.identity && left.version == right.version
           && left.contractVersion == right.contractVersion && left.source == right.source
           && left.sourceId == right.sourceId && left.status == right.status
           && left.trustState == right.trustState && left.title == right.title
           && left.category == right.category && left.icon == right.icon
           && left.shape == right.shape && left.description == right.description
           && left.defaultWidth == right.defaultWidth && left.defaultHeight == right.defaultHeight
           && left.properties == right.properties
           && left.outputProperties == right.outputProperties && left.ports == right.ports
           && left.execution == right.execution;
}

QString discoveryEventKindName(DiscoveryEvent::Kind kind)
{
    switch (kind)
    {
    case DiscoveryEvent::Kind::Registered: return QStringLiteral("registered");
    case DiscoveryEvent::Kind::Refreshed: return QStringLiteral("refreshed");
    case DiscoveryEvent::Kind::Unavailable: return QStringLiteral("unavailable");
    case DiscoveryEvent::Kind::Rejected: return QStringLiteral("rejected");
    case DiscoveryEvent::Kind::Removed: return QStringLiteral("removed");
    }
    return QStringLiteral("unknown");
}

} // namespace cme::runtime
