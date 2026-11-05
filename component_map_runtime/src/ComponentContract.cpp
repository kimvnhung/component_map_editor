#include "component_map_runtime/ComponentContract.h"

#include <QSet>
#include <QRegularExpression>

namespace cme::runtime
{
namespace
{
QStringList validateProperties(const QList<PropertySchema> &properties, const QString &scope)
{
    QStringList errors;
    QSet<QString> ids;
    for (const auto &property : properties)
    {
        if (property.id.isEmpty()) errors.append(scope + QStringLiteral(".id must not be empty"));
        if (ids.contains(property.id)) errors.append(scope + QStringLiteral(" contains duplicate id: ") + property.id);
        ids.insert(property.id);
        if (property.type.isEmpty()) errors.append(scope + QStringLiteral(".") + property.id + QStringLiteral(".type must not be empty"));
        if (property.constraints.contains(QStringLiteral("enum"))
            && !property.defaultValue.isEmpty()
            && !property.constraints.value(QStringLiteral("enum")).split(',', Qt::SkipEmptyParts)
                    .contains(property.defaultValue))
        {
            errors.append(scope + QStringLiteral(".") + property.id
                          + QStringLiteral(".default is not in enum constraint"));
        }
        if (property.constraints.contains(QStringLiteral("min_length"))
            && property.defaultValue.size() < property.constraints.value(QStringLiteral("min_length")).toInt())
        {
            errors.append(scope + QStringLiteral(".") + property.id
                          + QStringLiteral(".default violates min_length constraint"));
        }
    }
    return errors;
}

bool validSemanticVersion(const QString &version)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9]+\\.[0-9]+\\.[0-9]+(?:[-+][0-9A-Za-z.-]+)?$"));
    return pattern.match(version).hasMatch();
}

bool validPortDirection(const QString &direction)
{
    return direction == QStringLiteral("input") || direction == QStringLiteral("output");
}

bool validPortCardinality(const QString &cardinality)
{
    return cardinality.isEmpty() || cardinality == QStringLiteral("single")
           || cardinality == QStringLiteral("optional") || cardinality == QStringLiteral("repeated");
}
}

QString componentSourceName(ComponentSource source)
{
    return source == ComponentSource::Network ? QStringLiteral("network") : QStringLiteral("builtin");
}

QString componentStatusName(ComponentStatus status)
{
    switch (status)
    {
    case ComponentStatus::Registered: return QStringLiteral("registered");
    case ComponentStatus::Available: return QStringLiteral("available");
    case ComponentStatus::Unavailable: return QStringLiteral("unavailable");
    case ComponentStatus::Rejected: return QStringLiteral("rejected");
    case ComponentStatus::Removed: return QStringLiteral("removed");
    }
    return QStringLiteral("unknown");
}

QString trustStateName(TrustState state)
{
    switch (state)
    {
    case TrustState::Trusted: return QStringLiteral("trusted");
    case TrustState::Rejected: return QStringLiteral("rejected");
    case TrustState::Unknown: return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

QString executionModeName(ExecutionMode mode)
{
    return mode == ExecutionMode::RemoteWorker ? QStringLiteral("remote_worker") : QStringLiteral("builtin");
}

QStringList validateComponentDescriptor(const ComponentDescriptor &descriptor)
{
    QStringList errors;
    if (descriptor.identity.isEmpty()) errors.append(QStringLiteral("identity must not be empty"));
    if (descriptor.version.isEmpty()) errors.append(QStringLiteral("version must not be empty"));
    else if (!validSemanticVersion(descriptor.version)) errors.append(QStringLiteral("version must be semantic version"));
    if (descriptor.contractVersion.isEmpty()) errors.append(QStringLiteral("contract_version must not be empty"));
    else if (!validSemanticVersion(descriptor.contractVersion)) errors.append(QStringLiteral("contract_version must be semantic version"));
    if (descriptor.sourceId.isEmpty()) errors.append(QStringLiteral("source_id must not be empty"));
    if (descriptor.defaultWidth <= 0 || descriptor.defaultHeight <= 0)
        errors.append(QStringLiteral("default dimensions must be positive"));
    if (descriptor.source == ComponentSource::Builtin && descriptor.execution.mode != ExecutionMode::Builtin)
        errors.append(QStringLiteral("builtin components must use builtin execution"));
    if (descriptor.execution.timeoutMs < 0)
        errors.append(QStringLiteral("execution timeout must not be negative"));
    if (descriptor.execution.mode == ExecutionMode::RemoteWorker
        && descriptor.execution.workerCapabilityReference.isEmpty())
        errors.append(QStringLiteral("remote worker capability reference must not be empty"));
    if (descriptor.execution.mode == ExecutionMode::RemoteWorker && descriptor.execution.protocol.isEmpty())
        errors.append(QStringLiteral("remote worker protocol must not be empty"));
    if (descriptor.execution.mode == ExecutionMode::RemoteWorker && descriptor.execution.errorBehavior.isEmpty())
        errors.append(QStringLiteral("remote worker error behavior must not be empty"));
    if (descriptor.execution.mode == ExecutionMode::RemoteWorker
        && descriptor.execution.inputRequirements.isEmpty())
        errors.append(QStringLiteral("remote worker input requirements must not be empty"));
    if (descriptor.execution.mode == ExecutionMode::RemoteWorker
        && descriptor.execution.outputGuarantees.isEmpty())
        errors.append(QStringLiteral("remote worker output guarantees must not be empty"));
    if (descriptor.status == ComponentStatus::Available && descriptor.source == ComponentSource::Network
        && descriptor.trustState != TrustState::Trusted)
        errors.append(QStringLiteral("available network components must be trusted"));
    if (descriptor.status == ComponentStatus::Available && descriptor.execution.mode == ExecutionMode::Builtin
        && descriptor.source != ComponentSource::Builtin)
        errors.append(QStringLiteral("only builtin sources may expose builtin execution"));
    errors.append(validateProperties(descriptor.properties, QStringLiteral("properties")));
    errors.append(validateProperties(descriptor.outputProperties, QStringLiteral("output_properties")));

    QSet<QString> portIds;
    for (const auto &port : descriptor.ports)
    {
        if (port.id.isEmpty()) errors.append(QStringLiteral("ports.id must not be empty"));
        if (portIds.contains(port.id)) errors.append(QStringLiteral("ports contains duplicate id: ") + port.id);
        portIds.insert(port.id);
        if (port.payloadType.isEmpty()) errors.append(QStringLiteral("ports.") + port.id + QStringLiteral(".payload_type must not be empty"));
        if (!validPortDirection(port.direction))
            errors.append(QStringLiteral("ports.") + port.id + QStringLiteral(".direction must be input or output"));
        if (!validPortCardinality(port.cardinality))
            errors.append(QStringLiteral("ports.") + port.id + QStringLiteral(".cardinality is invalid"));
    }

    if (descriptor.source == ComponentSource::Network)
    {
        if (descriptor.execution.mode != ExecutionMode::RemoteWorker)
            errors.append(QStringLiteral("network components must use remote_worker execution"));
        if (descriptor.trustState != TrustState::Trusted)
            errors.append(QStringLiteral("network component is not trusted"));
        if (descriptor.execution.workerCapabilityReference.isEmpty())
            errors.append(QStringLiteral("remote worker capability reference must not be empty"));
        if (descriptor.execution.protocol.isEmpty())
            errors.append(QStringLiteral("remote worker protocol must not be empty"));
    }
    return errors;
}

} // namespace cme::runtime
