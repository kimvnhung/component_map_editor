#ifndef COMPONENT_MAP_RUNTIME_COMPONENT_CONTRACT_H
#define COMPONENT_MAP_RUNTIME_COMPONENT_CONTRACT_H

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

namespace cme::runtime
{

enum class ComponentSource
{
    Builtin,
    Network
};

enum class ComponentStatus
{
    Registered,
    Available,
    Unavailable,
    Rejected,
    Removed
};

enum class TrustState
{
    Unknown,
    Trusted,
    Rejected
};

enum class ExecutionMode
{
    Builtin,
    RemoteWorker
};

struct PropertySchema
{
    QString id;
    QString type;
    bool required = false;
    QString defaultValue;
    QMap<QString, QString> constraints;
    QString label;
    QString description;

    friend bool operator==(const PropertySchema &left, const PropertySchema &right)
    {
        return left.id == right.id && left.type == right.type && left.required == right.required
               && left.defaultValue == right.defaultValue && left.constraints == right.constraints
               && left.label == right.label && left.description == right.description;
    }
};

using OutputPropertySchema = PropertySchema;

struct DisplayMetadata
{
    QString title;
    QString category;
    QString icon;
    QString shape;
    int defaultWidth = 160;
    int defaultHeight = 80;
    QString description;
};

struct PortContract
{
    QString id;
    QString direction;
    QString payloadType;
    QString cardinality;
    bool required = false;
    QString label;
    QString description;

    friend bool operator==(const PortContract &left, const PortContract &right)
    {
        return left.id == right.id && left.direction == right.direction
               && left.payloadType == right.payloadType && left.cardinality == right.cardinality
               && left.required == right.required && left.label == right.label
               && left.description == right.description;
    }
};

struct ExecutionDescriptor
{
    ExecutionMode mode = ExecutionMode::Builtin;
    QStringList capabilities;
    QStringList inputRequirements;
    QStringList outputGuarantees;
    QString workerCapabilityReference;
    QString protocol;
    int timeoutMs = 0;
    bool supportsCancellation = false;
    bool retryable = false;
    bool idempotent = false;
    QString errorBehavior;

    friend bool operator==(const ExecutionDescriptor &left, const ExecutionDescriptor &right)
    {
        return left.mode == right.mode && left.capabilities == right.capabilities
               && left.inputRequirements == right.inputRequirements
               && left.outputGuarantees == right.outputGuarantees
               && left.workerCapabilityReference == right.workerCapabilityReference
               && left.protocol == right.protocol && left.timeoutMs == right.timeoutMs
               && left.supportsCancellation == right.supportsCancellation
               && left.retryable == right.retryable && left.idempotent == right.idempotent
               && left.errorBehavior == right.errorBehavior;
    }
};

struct ComponentDescriptor
{
    QString identity;
    QString version = QStringLiteral("1.0.0");
    QString contractVersion = QStringLiteral("1.0.0");
    ComponentSource source = ComponentSource::Builtin;
    QString sourceId;
    ComponentStatus status = ComponentStatus::Registered;
    TrustState trustState = TrustState::Unknown;
    QString title;
    QString category;
    QString icon;
    QString shape;
    QString description;
    int defaultWidth = 160;
    int defaultHeight = 80;
    QList<PropertySchema> properties;
    QList<PropertySchema> outputProperties;
    QList<PortContract> ports;
    ExecutionDescriptor execution;
    QStringList diagnostics;
};

QString componentSourceName(ComponentSource source);
QString componentStatusName(ComponentStatus status);
QString trustStateName(TrustState state);
QString executionModeName(ExecutionMode mode);
QStringList validateComponentDescriptor(const ComponentDescriptor &descriptor);

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_COMPONENT_CONTRACT_H
