#ifndef COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_TYPES_H
#define COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_TYPES_H

#include "component_map_runtime/ComponentContract.h"

#include <QDateTime>
#include <QString>
#include <QStringList>

namespace cme::runtime
{

struct ComponentKey
{
    QString identity;
    QString sourceId;
    QString version;

    bool isValid() const;
    QString toString() const;
    friend bool operator==(const ComponentKey &left, const ComponentKey &right)
    {
        return left.identity == right.identity && left.sourceId == right.sourceId
               && left.version == right.version;
    }
};

uint qHash(const ComponentKey &key, uint seed = 0);

struct ComponentDiagnostic
{
    QString code;
    QString message;
    QString fieldPath;
    bool retryable = false;

    bool isValid() const { return !code.isEmpty() && !message.isEmpty(); }
};

struct DiscoveryEvent
{
    enum class Kind
    {
        Registered,
        Refreshed,
        Unavailable,
        Rejected,
        Removed
    };

    QString eventId;
    QDateTime timestamp;
    ComponentKey component;
    Kind kind = Kind::Registered;
    ComponentStatus status = ComponentStatus::Registered;
    QList<ComponentDiagnostic> diagnostics;
};

bool equivalentComponentDescriptor(const ComponentDescriptor &left,
                                   const ComponentDescriptor &right);
ComponentKey componentKey(const ComponentDescriptor &descriptor);
QString discoveryEventKindName(DiscoveryEvent::Kind kind);

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_COMPONENT_DISCOVERY_TYPES_H
