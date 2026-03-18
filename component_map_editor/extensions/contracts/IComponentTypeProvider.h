#ifndef ICOMPONENTTYPEPROVIDER_H
#define ICOMPONENTTYPEPROVIDER_H

#include <QString>
#include <QStringList>
#include <QtPlugin>
#include <QVariantMap>

class IComponentTypeProvider
{
public:
    virtual ~IComponentTypeProvider() = default;

    virtual QString providerId() const = 0;
    virtual QStringList componentTypeIds() const = 0;

    // Returns metadata for a component type. Expected keys include:
    // id, title, category, ports, constraints.
    virtual QVariantMap componentTypeDescriptor(const QString &componentTypeId) const = 0;

    // Returns default property values for new components of this type.
    virtual QVariantMap defaultComponentProperties(const QString &componentTypeId) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_COMPONENT_TYPE_PROVIDER "ComponentMapEditor.Extensions.IComponentTypeProvider/1.0"
Q_DECLARE_INTERFACE(IComponentTypeProvider, COMPONENT_MAP_EDITOR_IID_COMPONENT_TYPE_PROVIDER)

#endif // ICOMPONENTTYPEPROVIDER_H