#ifndef CME_COMPONENT_DISCOVERY_FIXTURES_H
#define CME_COMPONENT_DISCOVERY_FIXTURES_H

#include "component_map_runtime/ComponentContract.h"

namespace cme::runtime::fixtures
{

inline ComponentDescriptor validBuiltin()
{
    ComponentDescriptor descriptor;
    descriptor.identity = QStringLiteral("fixture.builtin");
    descriptor.sourceId = QStringLiteral("fixture-builtin-source");
    descriptor.status = ComponentStatus::Available;
    descriptor.trustState = TrustState::Trusted;
    descriptor.title = QStringLiteral("Fixture Builtin");
    descriptor.category = QStringLiteral("fixture");
    descriptor.execution.inputRequirements = {QStringLiteral("event")};
    descriptor.execution.outputGuarantees = {QStringLiteral("event")};
    descriptor.execution.errorBehavior = QStringLiteral("structured");
    descriptor.properties.append({QStringLiteral("value"), QStringLiteral("string"), true,
                                  QStringLiteral("default"), {}, QStringLiteral("Value"), {}});
    descriptor.ports.append({QStringLiteral("input"), QStringLiteral("input"), QStringLiteral("event"),
                             QStringLiteral("single"), true, QStringLiteral("Input"), {}});
    descriptor.ports.append({QStringLiteral("output"), QStringLiteral("output"), QStringLiteral("event"),
                             QStringLiteral("single"), false, QStringLiteral("Output"), {}});
    return descriptor;
}

inline ComponentDescriptor validNetwork()
{
    auto descriptor = validBuiltin();
    descriptor.identity = QStringLiteral("fixture.network");
    descriptor.source = ComponentSource::Network;
    descriptor.sourceId = QStringLiteral("fixture-network-source");
    descriptor.execution.mode = ExecutionMode::RemoteWorker;
    descriptor.execution.workerCapabilityReference = QStringLiteral("fixture-worker-capability");
    descriptor.execution.protocol = QStringLiteral("fixture-event-v1");
    descriptor.execution.errorBehavior = QStringLiteral("structured");
    descriptor.trustState = TrustState::Trusted;
    return descriptor;
}

inline ComponentDescriptor invalidDescriptor()
{
    auto descriptor = validBuiltin();
    descriptor.identity.clear();
    return descriptor;
}

inline ComponentDescriptor unavailableDescriptor()
{
    auto descriptor = validBuiltin();
    descriptor.status = ComponentStatus::Unavailable;
    return descriptor;
}

inline ComponentDescriptor conflictingDescriptor()
{
    auto descriptor = validBuiltin();
    descriptor.title = QStringLiteral("Conflicting fixture");
    return descriptor;
}

} // namespace cme::runtime::fixtures

#endif // CME_COMPONENT_DISCOVERY_FIXTURES_H
