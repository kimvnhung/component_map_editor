#ifndef IVALIDATIONPROVIDER_H
#define IVALIDATIONPROVIDER_H

#include <QString>
#include <QtPlugin>
#include <QVariantList>
#include <QVariantMap>

class IValidationProvider
{
public:
    virtual ~IValidationProvider() = default;

    virtual QString providerId() const = 0;

    // Returns validation issues. Expected keys per issue:
    // code, severity, message, entityType, entityId.
    virtual QVariantList validateGraph(const QVariantMap &graphSnapshot) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER "ComponentMapEditor.Extensions.IValidationProvider/1.0"
Q_DECLARE_INTERFACE(IValidationProvider, COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER)

#endif // IVALIDATIONPROVIDER_H
