#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_TYPES_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_TYPES_H

#include <QByteArray>
#include <QString>

namespace cme::runtime
{

struct RuntimeComponentIdentity
{
    QString instanceId;
    QString componentType;

    bool isValid() const
    {
        return !instanceId.isEmpty() && !componentType.isEmpty();
    }
};

struct RuntimeEvent
{
    QString eventId;
    QString messageType;
    QString sourceComponentId;
    QString targetComponentId;
    QString idempotencyKey;
    QByteArray payload;
};

struct RuntimeFailure
{
    QString code;
    QString message;
    QString eventId;
    QString componentId;

    bool isValid() const
    {
        return !code.isEmpty() && !message.isEmpty();
    }
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_TYPES_H
